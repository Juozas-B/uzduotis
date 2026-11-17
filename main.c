#include <stdio.h>
#include <mosquitto.h>
#include <cjson/cJSON.h>

void on_message(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message){
    (void)userdata;
    (void)mosq;

    printf("Received message: %s\n", (char *)message->payload);
    fflush(stdout);
}


int main(void){
    mosquitto_lib_init();

    struct mosquitto *mosq = mosquitto_new("iot_monitor", true, NULL);

    mosquitto_connect(mosq, "localhost", 1883, 60);

    mosquitto_message_callback_set(mosq, on_message);

    mosquitto_subscribe(mosq, NULL, "iot/devices/+/telemetry", 0);

    mosquitto_loop_start(mosq);

    while (true){
    }
    
    return 0;
}