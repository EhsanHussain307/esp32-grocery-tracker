#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "keypad.h"
#include "display.h"
#include "catalog.h"
#include "list_store.h"
#include "ui.h"

#include "nvs_flash.h"
#include "storage_nvs.h"

#include "wifi_mgr.h"
#include "mqtt_mgr.h"


static const char *TAG = "APP";

void app_main(void)
{
    ESP_LOGI(TAG, "Grocery Terminal starting...");
   
    ESP_ERROR_CHECK(nvs_flash_init());

ESP_ERROR_CHECK(wifi_mgr_init());
ESP_ERROR_CHECK(wifi_mgr_connect_sta("Sunahome2.4GHZ", "AHHAM000", 15000));
ESP_ERROR_CHECK(mqtt_mgr_start());

    catalog_init_defaults();

    list_store_t store;
    list_store_init(&store);

    //nvs load 
    #include "storage_nvs.h"

uint64_t bits = 0;
if (storage_nvs_load_missing_bits(&bits) == ESP_OK) {
    list_store_set_missing_bits(&store, bits);
}

    // I2C OLED (found at 0x3C)
    display_cfg_t dcfg = {
        .pin_sda = 21,
        .pin_scl = 22,
        .i2c_port = 0,
        .i2c_addr = 0x3C,
    };
    display_init(&dcfg);

    keypad_cfg_t kcfg = {
        .rows = {13,12,14,27},
        .cols = {26,25,33,32},
        .scan_delay_ms = 2,
        .debounce_ms = 80,
    };
    keypad_init(&kcfg);

    ui_ctx_t ui;
    ui_init(&ui, &store);

    display_show_boot();

    while (1) {
        char key = 0;
        if (keypad_get_key(&key)) {
            ESP_LOGI("KEYPAD", "Pressed: %c", key);
            ui_handle_key(&ui, key);
        }

        ui_render(&ui);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
