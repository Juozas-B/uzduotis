#ifndef MQTT_CALLBACKS_H
#define MQTT_CALLBACKS_H

#include <signal.h>

#include <mosquitto.h>
#include "device_store.h"

extern volatile sig_atomic_t running;

struct alert_thresholds {
    double max_temperature;
    double max_humidity;
    int min_battery;
};

struct app_context {
    struct device_list *devices;
    struct alert_thresholds thresholds;
};

void on_message(struct mosquitto *mosq, void *userdata,
                const struct mosquitto_message *message);
void on_connect(struct mosquitto *mosq, void *userdata, int rc);
void on_disconnect(struct mosquitto *mosq, void *userdata, int rc);
void on_subscribe(struct mosquitto *mosq, void *userdata, int mid,
                  int qos_count, const int *granted_qos);

#endif
