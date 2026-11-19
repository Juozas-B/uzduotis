#ifndef MQTT_CALLBACKS_H
#define MQTT_CALLBACKS_H

#include <signal.h>

#include <mosquitto.h>

extern volatile sig_atomic_t running;

void on_message(struct mosquitto *mosq, void *userdata,
                const struct mosquitto_message *message);
void on_connect(struct mosquitto *mosq, void *userdata, int rc);
void on_disconnect(struct mosquitto *mosq, void *userdata, int rc);
void on_subscribe(struct mosquitto *mosq, void *userdata, int mid,
                  int qos_count, const int *granted_qos);

#endif
