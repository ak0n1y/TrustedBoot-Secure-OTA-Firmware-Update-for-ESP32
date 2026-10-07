#include <stdbool.h>
#include <string.h>

#include "bootloader_common.h"
#include "bootloader_flash_priv.h"
#include "bootloader_hooks.h"
#include "esp_flash_partitions.h"
#include "esp_log.h"
#include "fwmeta.h"

static const char *TAG = "custom_verify";

void bootloader_hooks_include(void)
{
}

void bootloader_before_init(void)
{
}

static void check_slot(int slot, const esp_partition_pos_t *pos, const fwmeta_record_t *record)
{
    if (record->magic != FWMETA_MAGIC) {
        ESP_LOGW(TAG, "slot %d: no seal, skipped", slot);
        return;
    }

    uint8_t digest[32];
    esp_err_t err = bootloader_common_get_sha256_of_partition(pos->offset, pos->size, PART_TYPE_APP, digest);
    if (err == ESP_OK && memcmp(digest, record->sha256, sizeof(digest)) == 0) {
        ESP_LOGI(TAG, "slot %d: seal verified", slot);
        return;
    }

    ESP_LOGE(TAG, "slot %d: seal mismatch, invalidating image", slot);
    bootloader_flash_erase_range(pos->offset, 0x1000);
}

void bootloader_after_init(void)
{
    const esp_partition_info_t *table = bootloader_mmap(ESP_PARTITION_TABLE_OFFSET, ESP_PARTITION_TABLE_MAX_LEN);
    if (table == NULL) {
        ESP_LOGE(TAG, "cannot map partition table");
        return;
    }

    int count = 0;
    esp_partition_pos_t slots[FWMETA_SLOTS];
    bool found[FWMETA_SLOTS] = { false };

    if (esp_partition_table_verify(table, false, &count) == ESP_OK) {
        for (int i = 0; i < count; i++) {
            if (table[i].type != PART_TYPE_APP) {
                continue;
            }
            int slot = (int)table[i].subtype - PART_SUBTYPE_OTA_FLAG;
            if (slot < 0 || slot >= FWMETA_SLOTS) {
                continue;
            }
            slots[slot] = table[i].pos;
            found[slot] = true;
        }
    }
    bootloader_munmap(table);

    fwmeta_record_t records[FWMETA_SLOTS];
    if (bootloader_flash_read(FWMETA_OFFSET, records, sizeof(records), false) != ESP_OK) {
        ESP_LOGE(TAG, "cannot read seals");
        return;
    }

    for (int slot = 0; slot < FWMETA_SLOTS; slot++) {
        if (found[slot]) {
            check_slot(slot, &slots[slot], &records[slot]);
        }
    }
}
