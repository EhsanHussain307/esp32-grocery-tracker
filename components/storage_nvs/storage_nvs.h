#pragma once
#include <stdint.h>
#include "esp_err.h"

esp_err_t storage_nvs_load_missing_bits(uint64_t *out_bits);
esp_err_t storage_nvs_save_missing_bits(uint64_t bits);
