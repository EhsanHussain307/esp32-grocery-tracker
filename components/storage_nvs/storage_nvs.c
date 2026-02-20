#include "storage_nvs.h"
#include "nvs.h"
#include "nvs_flash.h"

#define NVS_NS  "grocery"
#define KEYBITS "missing_bits"

esp_err_t storage_nvs_load_missing_bits(uint64_t *out_bits)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READONLY, &h);
    if (err != ESP_OK) return err;

    uint64_t v = 0;
    err = nvs_get_u64(h, KEYBITS, &v);
    nvs_close(h);

    if (err == ESP_OK) *out_bits = v;
    return err;
}

esp_err_t storage_nvs_save_missing_bits(uint64_t bits)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;

    err = nvs_set_u64(h, KEYBITS, bits);
    if (err == ESP_OK) err = nvs_commit(h);

    nvs_close(h);
    return err;
}
