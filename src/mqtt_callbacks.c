#include "mqtt_callbacks.h"

#include "device_store.h"
#include "logger.h"
#include "telemetry_parser.h"

#include <cjson/cJSON.h>
#include <math.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void publish_alert(struct mosquitto *mosq, const char *id, const char *alert_name, double value, long ts)
{
    cJSON *alert = cJSON_CreateObject();
    char *alert_str;
    char topic[128];
    int topic_length;
    int publish_result;

    if (alert == NULL) {
        log_line("Failed to create alert JSON for device %s\n", id);
        return;
    }

    if (cJSON_AddStringToObject(alert, "device_id", id) == NULL ||
        cJSON_AddStringToObject(alert, "alert", alert_name) == NULL ||
        cJSON_AddNumberToObject(alert, "value", value) == NULL ||
        cJSON_AddNumberToObject(alert, "timestamp", ts) == NULL) {

        log_line("Failed to build alert JSON for device %s\n", id);
        cJSON_Delete(alert);
        return;
    }

    alert_str = cJSON_PrintUnformatted(alert);
    if (alert_str == NULL) {
        log_line("Failed to serialize alert for device %s\n", id);
        cJSON_Delete(alert);
        return;
    }

    topic_length = snprintf(topic, sizeof(topic), "iot/devices/%s/alert", id);
    if (topic_length < 0 || (size_t)topic_length >= sizeof(topic)) {
        log_line("Alert topic is too long for device ID: %s\n", id);
        free(alert_str);
        cJSON_Delete(alert);
        return;
    }

    publish_result = mosquitto_publish(mosq, NULL, topic, strlen(alert_str), alert_str, 1, false);
    if (publish_result != MOSQ_ERR_SUCCESS) {
        log_line("Failed to publish alert for device %s: %s\n", id, mosquitto_strerror(publish_result));
    }

    free(alert_str);
    cJSON_Delete(alert);
}

void on_message(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message)
{
    struct app_context *context = userdata;
    struct device_list *list = context->devices;
    struct telemetry parsed;

    if (message == NULL || message->payload == NULL || message->payloadlen < 0) {
        log_line("Invalid MQTT message\n");
        return;
    }

    enum telemetry_result result = parse_telemetry(message->payload, (size_t)message->payloadlen, &parsed);
    if (result != TELEMETRY_OK) {
        switch (result) {
        case TELEMETRY_INVALID_JSON:
            log_line("Invalid JSON\n");
            break;
        case TELEMETRY_MISSING_FIELD:
            log_line("Missing required JSON fields\n");
            break;
        case TELEMETRY_INVALID_TYPE:
            log_line("Invalid JSON field types\n");
            break;
        case TELEMETRY_INVALID_ID:
            log_line("Invalid device ID\n");
            break;
        case TELEMETRY_INVALID_VALUE:
            log_line("Invalid telemetry values\n");
            break;
        case TELEMETRY_OK:
            break;
        }
        return;
    }

    const char *id = parsed.device_id;
    double temp = parsed.temperature;
    double hum = parsed.humidity;
    int bat = parsed.battery;
    long ts = parsed.timestamp;
    time_t display_ts = (time_t)ts;
    struct tm *laikas = localtime(&display_ts);
    if (laikas == NULL) {
        log_line("Invalid timestamp for device %s: %ld\n", id, ts);
        return;
    }

    if (pthread_mutex_lock(&list->mutex) != 0) {
        log_line("Failed to lock device list\n");
        return;
    }

    struct device *pridetas_device = find_or_add_device(list, id);
    if (pridetas_device == NULL) {
        pthread_mutex_unlock(&list->mutex);
        log_line("Failed to store telemetry for device %s\n", id);
        return;
    }

    int was_inactive = pridetas_device->inactive;

    pridetas_device->temperature = temp;
    pridetas_device->humidity = hum;
    pridetas_device->battery = bat;
    pridetas_device->timestamp = ts;
    pridetas_device->last_received = time(NULL);
    pridetas_device->inactive = 0;
    pthread_mutex_unlock(&list->mutex);

    if (was_inactive) {
        log_line("Device %s telemetry recovered\n", id);
    }

    int alert_printed = 0;
    if (temp > context->thresholds.max_temperature) {
        log_line("WARNING: HIGH_TEMPERATURE for device %s: %.2f°C\n", id, temp);
        publish_alert(mosq, id, "HIGH_TEMPERATURE", temp, ts);
        alert_printed++;
    }
    if (hum > context->thresholds.max_humidity) {
        log_line("WARNING: HIGH_HUMIDITY for device %s: %.2f%%\n", id, hum);
        publish_alert(mosq, id, "HIGH_HUMIDITY", hum, ts);
        alert_printed++;
    }
    if (bat < context->thresholds.min_battery) {
        log_line("WARNING: LOW_BATTERY for device %s: %d%%\n", id, bat);
        publish_alert(mosq, id, "LOW_BATTERY", bat, ts);
        alert_printed++;
    }
    if (alert_printed) {
        log_line("\n");
    }

    log_line("[%02d:%02d:%02d] Device: %s\n"
             "%-12s %.2f°C\n"
             "%-12s %.2f%%\n"
             "%-12s %d%%\n\n",
             laikas->tm_hour, laikas->tm_min, laikas->tm_sec, id,
             "Temperature:", temp,
             "Humidity:", hum,
             "Battery:", bat);

}

void on_connect(struct mosquitto *mosq, void *userdata, int rc)
{
    (void)userdata;

    if (rc == 0) {
        int subscribe_result;

        log_line("Connected to MQTT broker\n");
        subscribe_result = mosquitto_subscribe(mosq, NULL, "iot/devices/+/telemetry", 1);
        if (subscribe_result != MOSQ_ERR_SUCCESS) {
            log_line("Subscription failed: %s\n", mosquitto_strerror(subscribe_result));
            running = 0;
        }
    } else {
        log_line("Failed to connect to MQTT broker, error code: %d\n", rc);
    }
}

void on_disconnect(struct mosquitto *mosq, void *userdata, int rc)
{
    (void)mosq;
    (void)userdata;

    log_line("\nDisconnected: %s\n", mosquitto_strerror(rc));
}

void on_subscribe(struct mosquitto *mosq, void *userdata, int mid,
                  int qos_count, const int *granted_qos)
{
    (void)mid;
    (void)qos_count;
    (void)mosq;
    (void)userdata;

    if (granted_qos == NULL || qos_count <= 0 || granted_qos[0] == 0x80) {
        log_line("Subscribe failed\n");
        running = 0;
    }
}
