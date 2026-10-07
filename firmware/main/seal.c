#include "seal.h"

#include <string.h>

#include "esp_log.h"
#include "esp_ota_ops.h"
#include "fwmeta.h"

static const char *TAG = "seal";

static const esp_partition_t *meta_partition(void)
{
    return esp_partition_find_first(ESP_PARTITION_TYPE_DATA, (esp_partition_subtype_t)FWMETA_SUBTYPE, "fwmeta");
}

static int slot_of(const esp_partition_t *partition)
{
    int slot = (int)partition->subtype - (int)ESP_PARTITION_SUBTYPE_APP_OTA_MIN;
    if (slot < 0 || slot >= FWMETA_SLOTS) {
        return -1;
    }
    return slot;
}

static esp_err_t update_slot(const esp_partition_t *app_partition, bool seal)
{
    int slot = slot_of(app_partition);
    const esp_partition_t *meta = meta_partition();
    if (slot < 0 || meta == NULL) {
        return ESP_ERR_NOT_FOUND;
    }

    fwmeta_record_t records[FWMETA_SLOTS];
    esp_err_t err = esp_partition_read(meta, 0, records, sizeof(records));
    if (err != ESP_OK) {
        return err;
    }

    memset(&records[slot], 0xFF, sizeof(records[slot]));
    if (seal) {
        records[slot].magic = FWMETA_MAGIC;
        err = esp_partition_get_sha256(app_partition, records[slot].sha256);
        if (err != ESP_OK) {
            return err;
        }
    }

    err = esp_partition_erase_range(meta, 0, FWMETA_SIZE);
    if (err != ESP_OK) {
        return err;
    }
    err = esp_partition_write(meta, 0, records, sizeof(records));
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "slot %d %s", slot, seal ? "sealed" : "cleared");
    }
    return err;
}

esp_err_t seal_clear(const esp_partition_t *app_partition)
{
    return update_slot(app_partition, false);
}

esp_err_t seal_write(const esp_partition_t *app_partition)
{
    return update_slot(app_partition, true);
}
