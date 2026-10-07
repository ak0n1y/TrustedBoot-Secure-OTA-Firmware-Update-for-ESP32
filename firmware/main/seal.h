#pragma once

#include "esp_err.h"
#include "esp_partition.h"

esp_err_t seal_clear(const esp_partition_t *app_partition);
esp_err_t seal_write(const esp_partition_t *app_partition);
