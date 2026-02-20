#include "mqtt_mgr.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include <string.h>
#include <stdio.h>

extern const uint8_t hivemq_ca_pem_start[] asm("_binary_hivemq_ca_pem_start");
extern const uint8_t hivemq_ca_pem_end[]   asm("_binary_hivemq_ca_pem_end");

static const char *TAG = "MQTT_MGR";
static esp_mqtt_client_handle_t s_client = NULL;
static bool s_connected = false;

// ====== TODO: put these in menuconfig later ======
#define MQTT_BROKER_URI "mqtts://5b03d33a134c4eb9b36309bf9052b9a8.s1.eu.hivemq.cloud:8883"

#define MQTT_USERNAME     "ehsan"
#define MQTT_PASSWORD     "Sunasuna307@"
#define MQTT_DEVICE_NAME  "kitchen-esp32"
// =================================================



static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        s_connected = true;
        ESP_LOGI(TAG, "Connected ✅");
        // You can subscribe later if you want voice commands from phone:
        // esp_mqtt_client_subscribe(s_client, "home/grocery/cmd", 1);
        break;

    case MQTT_EVENT_DISCONNECTED:
        s_connected = false;
        ESP_LOGW(TAG, "Disconnected");
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "DATA topic=%.*s data=%.*s",
                 event->topic_len, event->topic,
                 event->data_len, event->data);
        break;

    default:
        break;
    }
}

esp_err_t mqtt_mgr_start(void)
{
       esp_mqtt_client_config_t cfg = {
    .broker.address.uri = MQTT_BROKER_URI,
    .broker.verification.certificate = (const char *)hivemq_ca_pem_start,
    
    .credentials.username = MQTT_USERNAME,
    .credentials.authentication.password = MQTT_PASSWORD,
    .session.keepalive = 120,
};


    s_client = esp_mqtt_client_init(&cfg);
    if (!s_client) return ESP_FAIL;

    esp_mqtt_client_register_event(s_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(s_client);

    ESP_LOGI(TAG, "MQTT start requested...");
    return ESP_OK;
}

bool mqtt_mgr_is_connected(void)
{
    return s_connected;
}
esp_err_t mqtt_mgr_publish_item(uint8_t item_id, const char *item_name, const char *status)
{
    if (!s_client || !s_connected) return ESP_ERR_INVALID_STATE;

    // 1) event JSON (non-retained) - optional, good for debugging
    char payload[160];
    snprintf(payload, sizeof(payload),
             "{\"id\":%u,\"item\":\"%s\",\"status\":\"%s\"}",
             item_id, item_name, status);

    esp_mqtt_client_publish(s_client, "home/grocery/item/set", payload, 0, 1, 0);

    // 2) retained state per item id (for apps sync)
    char topic2[64];
    snprintf(topic2, sizeof(topic2), "home/grocery/state/%u", item_id);
    esp_mqtt_client_publish(s_client, topic2, status, 0, 1, 1);

    ESP_LOGI(TAG, "Published: %s => %s", topic2, status);
    return ESP_OK;
}