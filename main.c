#include <stdio.h>
#include <stdarg.h>
#include <mosquitto.h>
#include <cjson/cJSON.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

static volatile sig_atomic_t running = 1;
static FILE *log_fp = NULL;

static void log_line(const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    vprintf(fmt, args);
    fflush(stdout);
    va_end(args);

    if (log_fp) {
        va_start(args, fmt);
        vfprintf(log_fp, fmt, args);
        fflush(log_fp);
        va_end(args);
    }
}

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


static void handle_sigint(int sig) { (void)sig; running = 0; }

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

static void publish_alert(struct mosquitto *mosq, const char *id, const char *alert_name, double value, long ts) {
    cJSON *alert = cJSON_CreateObject();
    cJSON_AddStringToObject(alert, "device_id", id);
    cJSON_AddStringToObject(alert, "alert", alert_name);
    cJSON_AddNumberToObject(alert, "value", value);
    cJSON_AddNumberToObject(alert, "timestamp", ts);
    char *alert_str = cJSON_PrintUnformatted(alert);
    char topic[128];
    snprintf(topic, sizeof(topic), "iot/devices/%s/alert", id);
    mosquitto_publish(mosq, NULL, topic, strlen(alert_str), alert_str, 0, false);
    free(alert_str);
    cJSON_Delete(alert);
}

void on_message(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message){
    (void)mosq;

    struct device_list *list = userdata;

    cJSON *json = cJSON_Parse((char *)message->payload);

    if (json == NULL) {
        log_line("nera json\n");
        return;
    }

    cJSON *device_id = cJSON_GetObjectItem(json, "device_id");
    cJSON *temperature = cJSON_GetObjectItem(json, "temperature");
    cJSON *humidity = cJSON_GetObjectItem(json, "humidity");
    cJSON *battery = cJSON_GetObjectItem(json, "battery");
    cJSON *timestamp = cJSON_GetObjectItem(json, "timestamp");

    if (!device_id || !temperature || !humidity || !battery || !timestamp) {
        log_line("Truksta json dalies\n");
        cJSON_Delete(json);
        return;
    }

    if (!cJSON_IsString(device_id) || !cJSON_IsNumber(temperature) || !cJSON_IsNumber(humidity) || !cJSON_IsNumber(battery) || !cJSON_IsNumber(timestamp)) {
        log_line("Blogas dalies formatas\n");
        cJSON_Delete(json);
        return;
    }

    
    char *id = device_id->valuestring;
    double temp = temperature->valuedouble;
    double hum = humidity->valuedouble;
    int bat = battery->valueint;
    long ts = timestamp->valueint;

    //4.3
    struct device *pridetas_device = find_or_add_device(list, id);

    pridetas_device->temperature = temp;
    pridetas_device->humidity = hum;
    pridetas_device->battery = bat;
    pridetas_device->timestamp = ts;

    //4.5
    int alert_printed = 0;
    if (temp > 30.0) {
        log_line("WARNING: HIGH_TEMPERATURE for device %s: %.2f°C\n", id, temp);
        publish_alert(mosq, id, "HIGH_TEMPERATURE", temp, ts);
        alert_printed++;
    }
    if (hum > 80.0) {
        log_line("WARNING: HIGH_HUMIDITY for device %s: %.2f%%\n", id, hum);
        publish_alert(mosq, id, "HIGH_HUMIDITY", hum, ts);
        alert_printed++;
    }
    if (bat < 20) {
        log_line("WARNING: LOW_BATTERY for device %s: %d%%\n", id, bat);
        publish_alert(mosq, id, "LOW_BATTERY", bat, ts);
        alert_printed++;
    }
    if (alert_printed) {
        log_line("\n");
    }


    //4.4
    struct tm *laikas = localtime(&ts);

    log_line("[%02d:%02d:%02d] Device: %s\nTemperature: %.2f°C\nHumidity: %.2f%%\nBattery: %d%%\n\n", laikas->tm_hour, laikas->tm_min, laikas->tm_sec, id, temp, hum, bat);


    //printf("Received message: %s\n", (char *)message->payload);
    fflush(stdout);
}

void on_connect(struct mosquitto *mosq, void *userdata, int rc){
    (void)userdata;

    if (rc == 0){
        log_line("Prisijungta prie MQTT brokerio\n");
        mosquitto_subscribe(mosq, NULL, "iot/devices/+/telemetry", 0);
    } else {
        log_line("Nepavyko prisijungti prie MQTT brokerio, klaidos kodas: %d\n", rc);
    }
}

void on_disconnect(struct mosquitto *mosq, void *userdata, int rc){
    (void)mosq;
    (void)userdata;

    log_line("Disconnected: %s\n", mosquitto_strerror(rc));
}

void on_subscribe(struct mosquitto *mosq, void *userdata, int mid, int qos_count, const int *granted_qos){
    (void)mid;
    (void)qos_count;
    (void)mosq;
    (void)userdata;

    if(granted_qos[0] == 0x80){
        log_line("Subcribe failed\n");
        running = 0;
    }

}

int main(int argc, char *argv[]){

    // 4.7
    char *host = "localhost"; 
    int port = 1883; 
    char *logfile = NULL;
    int opt;
    while ((opt = getopt(argc, argv, "H:P:o:")) != -1) {
        switch (opt) {
            case 'H': host = optarg; break;
            case 'P': port = atoi(optarg); break;
            case 'o': logfile = optarg; break;
            case 'h':
                printf("Usage: %s -H host -P port -o logfile\n", argv[0]);
                printf("  -H  MQTT broker host (default: localhost)\n");
                printf("  -P  MQTT broker port (default: 1883)\n");
                printf("  -o  Log file (default: stdout)\n");
                printf("  -h  Show this help\n");
                return 0;
            default:
                fprintf(stderr, "Usage: %s [-H host] [-P port] [-o file]\n", argv[0]);
                return 1;
        }
    }

    if (logfile) {
        log_fp = fopen(logfile, "a");
        if (!log_fp) {
            perror("Failed to open log file");
            return 1;
        }
    }

    struct device_list device_list = {0};

    //4.8
    signal(SIGINT, handle_sigint);

    //4.1
    mosquitto_lib_init();

    struct mosquitto *mosq = mosquitto_new("iot_monitor", true, &device_list);

    // 4.6
    mosquitto_disconnect_callback_set(mosq, on_disconnect);
    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_subscribe_callback_set(mosq, on_subscribe);
    mosquitto_reconnect_delay_set(mosq, 1, 10, true);

    // 4.2
    mosquitto_message_callback_set(mosq, on_message);
    mosquitto_connect(mosq, host, port, 60);

    mosquitto_loop_start(mosq);

    while (running) {
        sleep(1);
    }

    mosquitto_loop_stop(mosq, true);
    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    free(device_list.arr);
    if (log_fp) fclose(log_fp);
    
    return 0;
}