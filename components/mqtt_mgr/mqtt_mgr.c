
#include "mqtt_mgr.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include <string.h>
#include <stdio.h>
#include "secrets.h"


#include "catalog.h"
#include "cJSON.h"
#include <stdlib.h>

#if __has_include("secrets.h")
  #include "secrets.h"
#else
  #include "secrets_example.h"
#endif

#define TOPIC_CATALOG_SET    "home/grocery/catalog/set"
#define TOPIC_CATALOG_DELETE "home/grocery/catalog/delete"

extern const uint8_t hivemq_ca_pem_start[] asm("_binary_hivemq_ca_pem_start");
extern const uint8_t hivemq_ca_pem_end[]   asm("_binary_hivemq_ca_pem_end");

static const char *TAG = "MQTT_MGR";
static esp_mqtt_client_handle_t s_client = NULL;
static bool s_connected = false;


#define MQTT_DEVICE_NAME  "kitchen-esp32"
// =================================================

static bool parse_category_string(const char *s, category_t *out_cat)
{
    if (!s || !out_cat) return false;

    if (strcmp(s, "Pantry") == 0) {
        *out_cat = CAT_PANTRY;
        return true;
    }
    if (strcmp(s, "Fridge") == 0) {
        *out_cat = CAT_FRIDGE;
        return true;
    }
    if (strcmp(s, "Cleaning") == 0) {
        *out_cat = CAT_CLEANING;
        return true;
    }

    return false;
}

static void handle_catalog_set_message(const char *payload)
{
    cJSON *root = cJSON_Parse(payload);
    if (!root) {
        ESP_LOGW("MQTT_MGR", "Invalid JSON for catalog/set");
        return;
    }

    cJSON *id_json = cJSON_GetObjectItemCaseSensitive(root, "id");
    cJSON *code_json = cJSON_GetObjectItemCaseSensitive(root, "code");
    cJSON *name_json = cJSON_GetObjectItemCaseSensitive(root, "name");
    cJSON *cat_json = cJSON_GetObjectItemCaseSensitive(root, "category");

    if (!cJSON_IsNumber(id_json) ||
        !cJSON_IsString(name_json) ||
        !cJSON_IsString(cat_json)) {
        ESP_LOGW("MQTT_MGR", "catalog/set missing required fields");
        cJSON_Delete(root);
        return;
    }

    uint8_t id = (uint8_t)id_json->valueint;
    int16_t code = -1;
    if (cJSON_IsNumber(code_json)) {
        code = (int16_t)code_json->valueint;
    }

    category_t cat;
    if (!parse_category_string(cat_json->valuestring, &cat)) {
        ESP_LOGW("MQTT_MGR", "Invalid category: %s", cat_json->valuestring);
        cJSON_Delete(root);
        return;
    }

    bool ok = catalog_add_or_update_item(id, cat, name_json->valuestring, code);
    if (ok) {
        ESP_LOGI("MQTT_MGR", "Catalog set OK: id=%u name=%s code=%d",
                 id, name_json->valuestring, code);
    } else {
        ESP_LOGW("MQTT_MGR", "Catalog set FAILED: id=%u", id);
    }

    cJSON_Delete(root);
}
static void handle_catalog_delete_message(const char *payload)
{
    cJSON *root = cJSON_Parse(payload);
    if (!root) {
        ESP_LOGW("MQTT_MGR", "Invalid JSON for catalog/delete");
        return;
    }

    cJSON *id_json = cJSON_GetObjectItemCaseSensitive(root, "id");
    if (!cJSON_IsNumber(id_json)) {
        ESP_LOGW("MQTT_MGR", "catalog/delete missing id");
        cJSON_Delete(root);
        return;
    }

    uint8_t id = (uint8_t)id_json->valueint;

    bool ok = catalog_archive_item(id);
    if (ok) {
        ESP_LOGI("MQTT_MGR", "Catalog delete OK: id=%u", id);
    } else {
        ESP_LOGW("MQTT_MGR", "Catalog delete FAILED: id=%u", id);
    }

    cJSON_Delete(root);
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        s_connected = true;
        ESP_LOGI(TAG, "Connected ✅");
        esp_mqtt_client_subscribe(event->client, TOPIC_CATALOG_SET, 1);
       esp_mqtt_client_subscribe(event->client, TOPIC_CATALOG_DELETE, 1);

       ESP_LOGI("MQTT_MGR", "Subscribed to catalog topics");
        break;

    case MQTT_EVENT_DISCONNECTED:
        s_connected = false;
        ESP_LOGW(TAG, "Disconnected");
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
        break;

  case MQTT_EVENT_DATA: {
    char topic[event->topic_len + 1];
    char data[event->data_len + 1];

    memcpy(topic, event->topic, event->topic_len);
    topic[event->topic_len] = '\0';

    memcpy(data, event->data, event->data_len);
    data[event->data_len] = '\0';

    ESP_LOGI("MQTT_MGR", "Received topic=%s data=%s", topic, data);

    if (strcmp(topic, TOPIC_CATALOG_SET) == 0) {
        handle_catalog_set_message(data);
    } else if (strcmp(topic, TOPIC_CATALOG_DELETE) == 0) {
        handle_catalog_delete_message(data);
    }

    break;
}

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