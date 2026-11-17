#include <stdio.h>
#include <mosquitto.h>
#include <cjson/cJSON.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

struct device{
    char device_id[64]; // reik pridet patikrinima ar ne per ilgas gal?
    double temperature;
    double humidity;
    int battery;
    long timestamp;
};

struct device_list{
    struct device *arr;
    int count;
    int capacity;
};


struct device *find_or_add_device(struct device_list *list, const char *device_id){
    
    for (int i = 0; i < list->count; i++){
        if (strcmp(list->arr[i].device_id, device_id) == 0){
            return &list->arr[i];
        }
    }

    if (list->count == list->capacity){ //prapleciam
        list->capacity = list->capacity == 0 ? 1 : list->capacity * 2;
        list->arr = realloc(list->arr, list->capacity * sizeof(struct device)); // list->arr visada rodo i pirma
    }

    strcpy(list->arr[list->count].device_id, device_id);
    return &list->arr[list->count++];
}



void on_message(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message){
    (void)mosq;

    struct device_list *list = userdata;

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

    struct device *pridetas_device = find_or_add_device(list, id);

    pridetas_device->temperature = temp;
    pridetas_device->humidity = hum;
    pridetas_device->battery = bat;
    pridetas_device->timestamp = ts;


    struct tm *laikas = localtime(&ts);

    printf("[%02d:%02d:%02d] Device: %s\nTemperature: %.2f°C\nHumidity: %.2f%%\nBattery: %d%%\n\n", laikas->tm_hour, laikas->tm_min, laikas->tm_sec, id, temp, hum, bat);


    //printf("Received message: %s\n", (char *)message->payload);
    fflush(stdout);
}


int main(void){
    struct device_list device_list = {0};

    mosquitto_lib_init();

    struct mosquitto *mosq = mosquitto_new("iot_monitor", true, &device_list);

    mosquitto_connect(mosq, "localhost", 1883, 60);

    mosquitto_message_callback_set(mosq, on_message);

    mosquitto_subscribe(mosq, NULL, "iot/devices/+/telemetry", 0);

    mosquitto_loop_start(mosq);

    while (true){
    }
    
    return 0;
}