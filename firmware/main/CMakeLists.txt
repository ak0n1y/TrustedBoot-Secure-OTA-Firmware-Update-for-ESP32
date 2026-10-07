set(SERVER_CA "${CMAKE_CURRENT_LIST_DIR}/certs/server_ca.pem")
set(OTA_PUB "${CMAKE_CURRENT_LIST_DIR}/certs/ota_signing_pub.pem")

if(NOT EXISTS "${SERVER_CA}" OR NOT EXISTS "${OTA_PUB}")
    message(FATAL_ERROR "Missing certificates in main/certs. Run tools/gen_keys.sh first.")
endif()

idf_component_register(
    SRCS "main.c" "ota_client.c" "version.c" "self_test.c" "seal.c"
    INCLUDE_DIRS "." "../bootloader_components/custom_verify/include"
    REQUIRES esp_wifi esp_event esp_netif nvs_flash esp_http_client esp_partition app_update esp_app_format json mbedtls
    EMBED_TXTFILES "certs/server_ca.pem" "certs/ota_signing_pub.pem"
)
