#include "storage_nvs.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <string.h>

#define NVS_NS       "grocery"

// Old format (v1)
#define KEYBITS_U64  "missing_bits"   // uint64_t

// New format (v2)
#define KEYBITS_BLOB "missing_blob"   // blob of LIST_BYTES
#define KEYVER       "ver"            // uint32_t (optional)

#define STORE_VER    2

// Load blob (new). If missing, try old u64 and migrate.
esp_err_t storage_nvs_load_missing_blob(void *out_blob, size_t blob_size, bool *out_migrated)
{
    if (out_migrated) *out_migrated = false;

    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READONLY, &h);
    if (err != ESP_OK) return err;

    // 1) Try new blob first
    size_t required = 0;
    err = nvs_get_blob(h, KEYBITS_BLOB, NULL, &required);
    if (err == ESP_OK && required > 0) {
        // Zero then copy what we have (safe if older smaller blob)
        memset(out_blob, 0, blob_size);

        size_t copy_len = required;
        if (copy_len > blob_size) copy_len = blob_size;

        err = nvs_get_blob(h, KEYBITS_BLOB, out_blob, &copy_len);
        nvs_close(h);
        return err;
    }

    // 2) If blob not found, try old u64
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        uint64_t old_bits = 0;
        esp_err_t err2 = nvs_get_u64(h, KEYBITS_U64, &old_bits);
        nvs_close(h);

        if (err2 == ESP_OK) {
           
            memset(out_blob, 0, blob_size);
            memcpy(out_blob, &old_bits, sizeof(old_bits));
            if (out_migrated) *out_migrated = true;
            return ESP_OK;
        }
        return err2; // could be NOT_FOUND
    }

    nvs_close(h);
    return err;
}

esp_err_t storage_nvs_save_missing_blob(const void *blob, size_t blob_size)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;

    // Save version
    (void)nvs_set_u32(h, KEYVER, STORE_VER);

    // Save blob
    err = nvs_set_blob(h, KEYBITS_BLOB, blob, blob_size);
    if (err == ESP_OK) err = nvs_commit(h);

    nvs_close(h);
    return err;
}


esp_err_t storage_nvs_erase_old_u64(void)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;

    err = nvs_erase_key(h, KEYBITS_U64);
    if (err == ESP_OK) err = nvs_commit(h);

    nvs_close(h);
    return err;
}