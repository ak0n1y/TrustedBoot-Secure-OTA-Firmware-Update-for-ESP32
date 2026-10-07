#pragma once

typedef enum {
    OTA_RESULT_NO_UPDATE,
    OTA_RESULT_UPDATED,
    OTA_RESULT_REJECTED,
    OTA_RESULT_ERROR,
} ota_result_t;

ota_result_t ota_client_check_and_update(void);
