#include <stdio.h>
#include <mosquitto.h>
#include <cjson/cJSON.h>
#include <time.h>

void on_message(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message){
    (void)userdata;
    (void)mosq;

    cJSON *json = cJSON_Parse((char *)message->payload);

    if (json == NULL) {
        printf("nera json\n");
        return;
    }

    cJSON *device_id = cJSON_GetObjectItem(json, "device_id");
    cJSON *temperature = cJSON_GetObjectItem(json, "temperature");
    cJSON *humidity = cJSON_GetObjectItem(json, "humidity");
    cJSON *battery = cJSON_GetObjectItem(json, "battery");
    cJSON *timestamp = cJSON_GetObjectItem(json, "timestamp");

    if (!device_id || !temperature || !humidity || !battery || !timestamp) {
        printf("Truksta json dalies\n");
        cJSON_Delete(json);
        return;
    }

    if (!cJSON_IsString(device_id) || !cJSON_IsNumber(temperature) || !cJSON_IsNumber(humidity) || !cJSON_IsNumber(battery) || !cJSON_IsNumber(timestamp)) {
        printf("Blogas dalies formatas\n");
        cJSON_Delete(json);
        return;
    }

    char *id = device_id->valuestring;
    double temp = temperature->valuedouble;
    double hum = humidity->valuedouble;
    int bat = battery->valueint;
    long ts = timestamp->valueint;

    struct tm *laikas = localtime(&ts);

    printf("[%02d:%02d:%02d] Device: %s\nTemperature: %.2f°C\nHumidity: %.2f%%\nBattery: %d%%\n\n", laikas->tm_hour, laikas->tm_min, laikas->tm_sec, id, temp, hum, bat);


    //printf("Received message: %s\n", (char *)message->payload);
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