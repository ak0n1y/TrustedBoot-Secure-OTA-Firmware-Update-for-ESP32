#include "ota_client.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include "esp_app_desc.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "mbedtls/base64.h"
#include "mbedtls/pk.h"
#include "mbedtls/sha256.h"
#include "sdkconfig.h"
#include "seal.h"
#include "version.h"

#define MANIFEST_MAX_LEN   2048
#define SIGNATURE_MAX_LEN  256
#define CHUNK_SIZE         4096
#define DIGEST_LEN         32

extern const uint8_t server_ca_pem_start[] asm("_binary_server_ca_pem_start");
extern const uint8_t ota_signing_pub_pem_start[] asm("_binary_ota_signing_pub_pem_start");
extern const uint8_t ota_signing_pub_pem_end[] asm("_binary_ota_signing_pub_pem_end");

static const char *TAG = "ota_client";

typedef struct {
    char version[32];
    char url[256];
    char sha256_hex[2 * DIGEST_LEN + 1];
    uint8_t signature[SIGNATURE_MAX_LEN];
    size_t signature_len;
    size_t size;
} manifest_t;

static esp_http_client_handle_t client_init(const char *url)
{
    esp_http_client_config_t config = {
        .url = url,
        .cert_pem = (const char *)server_ca_pem_start,
        .timeout_ms = CONFIG_SOTA_HTTP_TIMEOUT_MS,
    };
    return esp_http_client_init(&config);
}

static void client_close(esp_http_client_handle_t client)
{
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
}

static bool url_allowed(const char *url)
{
    if (strncmp(url, "https://", 8) == 0) {
        return true;
    }
#if CONFIG_SOTA_ALLOW_HTTP
    return strncmp(url, "http://", 7) == 0;
#else
    return false;
#endif
}

static bool hex_decode(const char *hex, uint8_t *out, size_t out_len)
{
    if (strlen(hex) != out_len * 2) {
        return false;
    }
    for (size_t i = 0; i < out_len; i++) {
        unsigned value = 0;
        for (int j = 0; j < 2; j++) {
            char c = hex[2 * i + j];
            unsigned nibble;
            if (c >= '0' && c <= '9') {
                nibble = (unsigned)(c - '0');
            } else if (c >= 'a' && c <= 'f') {
                nibble = (unsigned)(c - 'a' + 10);
            } else if (c >= 'A' && c <= 'F') {
                nibble = (unsigned)(c - 'A' + 10);
            } else {
                return false;
            }
            value = (value << 4) | nibble;
        }
        out[i] = (uint8_t)value;
    }
    return true;
}

static esp_err_t fetch_manifest(char *buffer, size_t capacity)
{
    esp_http_client_handle_t client = client_init(CONFIG_SOTA_MANIFEST_URL);
    if (client == NULL) {
        return ESP_FAIL;
    }

    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        esp_http_client_cleanup(client);
        return err;
    }

    esp_http_client_fetch_headers(client);
    if (esp_http_client_get_status_code(client) != 200) {
        client_close(client);
        return ESP_FAIL;
    }

    size_t total = 0;
    while (total < capacity - 1) {
        int read = esp_http_client_read(client, buffer + total, (int)(capacity - 1 - total));
        if (read < 0) {
            err = ESP_FAIL;
            break;
        }
        if (read == 0) {
            break;
        }
        total += (size_t)read;
    }
    buffer[total] = '\0';

    client_close(client);
    return err;
}

static bool parse_manifest(const char *json, manifest_t *manifest)
{
    cJSON *root = cJSON_Parse(json);
    if (root == NULL) {
        return false;
    }

    bool ok = false;
    const cJSON *version = cJSON_GetObjectItemCaseSensitive(root, "version");
    const cJSON *url = cJSON_GetObjectItemCaseSensitive(root, "url");
    const cJSON *sha256 = cJSON_GetObjectItemCaseSensitive(root, "sha256");
    const cJSON *size = cJSON_GetObjectItemCaseSensitive(root, "size");
    const cJSON *signature = cJSON_GetObjectItemCaseSensitive(root, "signature");

    if (!cJSON_IsString(version) || !cJSON_IsString(url) || !cJSON_IsString(sha256) ||
        !cJSON_IsNumber(size) || !cJSON_IsString(signature)) {
        goto done;
    }
    if (strlen(version->valuestring) >= sizeof(manifest->version) ||
        strlen(url->valuestring) >= sizeof(manifest->url) ||
        strlen(sha256->valuestring) != 2 * DIGEST_LEN || size->valuedouble <= 0) {
        goto done;
    }

    strcpy(manifest->version, version->valuestring);
    strcpy(manifest->url, url->valuestring);
    strcpy(manifest->sha256_hex, sha256->valuestring);
    manifest->size = (size_t)size->valuedouble;

    if (mbedtls_base64_decode(manifest->signature, sizeof(manifest->signature), &manifest->signature_len,
                              (const unsigned char *)signature->valuestring,
                              strlen(signature->valuestring)) != 0) {
        goto done;
    }
    ok = true;

done:
    cJSON_Delete(root);
    return ok;
}

static bool signature_valid(const uint8_t *digest, const uint8_t *signature, size_t signature_len)
{
    mbedtls_pk_context pk;
    mbedtls_pk_init(&pk);

    size_t key_len = (size_t)(ota_signing_pub_pem_end - ota_signing_pub_pem_start);
    int rc = mbedtls_pk_parse_public_key(&pk, ota_signing_pub_pem_start, key_len);
    if (rc == 0) {
        rc = mbedtls_pk_verify(&pk, MBEDTLS_MD_SHA256, digest, DIGEST_LEN, signature, signature_len);
    }
    mbedtls_pk_free(&pk);
    return rc == 0;
}

static ota_result_t download_and_install(const manifest_t *manifest, const esp_partition_t *target)
{
    ota_result_t result = OTA_RESULT_ERROR;
    esp_ota_handle_t ota_handle = 0;
    bool ota_started = false;
    mbedtls_sha256_context sha;
    uint8_t *chunk = NULL;
    esp_http_client_handle_t client = NULL;
    bool client_opened = false;

    mbedtls_sha256_init(&sha);

    seal_clear(target);

    if (esp_ota_begin(target, OTA_WITH_SEQUENTIAL_WRITES, &ota_handle) != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed");
        goto cleanup;
    }
    ota_started = true;

    chunk = malloc(CHUNK_SIZE);
    client = client_init(manifest->url);
    if (chunk == NULL || client == NULL) {
        goto cleanup;
    }

    if (esp_http_client_open(client, 0) != ESP_OK) {
        ESP_LOGE(TAG, "cannot open firmware URL");
        goto cleanup;
    }
    client_opened = true;

    int64_t content_length = esp_http_client_fetch_headers(client);
    if (esp_http_client_get_status_code(client) != 200 || content_length != (int64_t)manifest->size) {
        ESP_LOGE(TAG, "unexpected HTTP response");
        goto cleanup;
    }

    if (mbedtls_sha256_starts(&sha, 0) != 0) {
        goto cleanup;
    }

    size_t received = 0;
    while (true) {
        int read = esp_http_client_read(client, (char *)chunk, CHUNK_SIZE);
        if (read < 0) {
            ESP_LOGE(TAG, "download error");
            goto cleanup;
        }
        if (read == 0) {
            break;
        }
        received += (size_t)read;
        if (received > manifest->size) {
            ESP_LOGE(TAG, "image larger than announced");
            goto cleanup;
        }
        if (mbedtls_sha256_update(&sha, chunk, (size_t)read) != 0 ||
            esp_ota_write(ota_handle, chunk, (size_t)read) != ESP_OK) {
            goto cleanup;
        }
    }

    if (received != manifest->size) {
        ESP_LOGE(TAG, "incomplete download");
        goto cleanup;
    }

    uint8_t digest[DIGEST_LEN];
    uint8_t expected[DIGEST_LEN];
    if (mbedtls_sha256_finish(&sha, digest) != 0 || !hex_decode(manifest->sha256_hex, expected, DIGEST_LEN)) {
        goto cleanup;
    }

    if (memcmp(digest, expected, DIGEST_LEN) != 0) {
        ESP_LOGE(TAG, "SHA-256 mismatch, image rejected");
        result = OTA_RESULT_REJECTED;
        goto cleanup;
    }

    if (!signature_valid(digest, manifest->signature, manifest->signature_len)) {
        ESP_LOGE(TAG, "invalid signature, image rejected");
        result = OTA_RESULT_REJECTED;
        goto cleanup;
    }

    ota_started = false;
    if (esp_ota_end(ota_handle) != ESP_OK) {
        ESP_LOGE(TAG, "image validation failed, image rejected");
        result = OTA_RESULT_REJECTED;
        goto cleanup;
    }

    if (seal_write(target) != ESP_OK) {
        ESP_LOGE(TAG, "cannot write seal");
        goto cleanup;
    }

    if (esp_ota_set_boot_partition(target) != ESP_OK) {
        ESP_LOGE(TAG, "cannot set boot partition");
        goto cleanup;
    }

    ESP_LOGI(TAG, "update to %s installed", manifest->version);
    result = OTA_RESULT_UPDATED;

cleanup:
    if (ota_started) {
        esp_ota_abort(ota_handle);
    }
    if (client != NULL) {
        if (client_opened) {
            client_close(client);
        } else {
            esp_http_client_cleanup(client);
        }
    }
    free(chunk);
    mbedtls_sha256_free(&sha);
    return result;
}

ota_result_t ota_client_check_and_update(void)
{
    char *buffer = calloc(1, MANIFEST_MAX_LEN);
    manifest_t *manifest = calloc(1, sizeof(manifest_t));
    ota_result_t result = OTA_RESULT_ERROR;

    if (buffer == NULL || manifest == NULL) {
        goto done;
    }

    if (fetch_manifest(buffer, MANIFEST_MAX_LEN) != ESP_OK) {
        ESP_LOGW(TAG, "manifest unreachable");
        goto done;
    }
    if (!parse_manifest(buffer, manifest)) {
        ESP_LOGE(TAG, "invalid manifest");
        goto done;
    }

    version_t current;
    version_t offered;
    const esp_app_desc_t *description = esp_app_get_description();
    if (!version_parse(description->version, &current) || !version_parse(manifest->version, &offered)) {
        ESP_LOGE(TAG, "cannot parse versions");
        goto done;
    }

    if (version_compare(&offered, &current) <= 0) {
        ESP_LOGI(TAG, "up to date (%s)", description->version);
        result = OTA_RESULT_NO_UPDATE;
        goto done;
    }

    if (!url_allowed(manifest->url)) {
        ESP_LOGE(TAG, "firmware URL scheme refused");
        result = OTA_RESULT_REJECTED;
        goto done;
    }

    const esp_partition_t *target = esp_ota_get_next_update_partition(NULL);
    if (target == NULL || manifest->size > target->size) {
        ESP_LOGE(TAG, "no suitable partition");
        goto done;
    }

    ESP_LOGI(TAG, "update %s -> %s", description->version, manifest->version);
    result = download_and_install(manifest, target);

done:
    free(buffer);
    free(manifest);
    return result;
}
