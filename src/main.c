#include "device_store.h"
#include "logger.h"
#include "mqtt_callbacks.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <mosquitto.h>

volatile sig_atomic_t running = 1;

static void handle_sigint(int sig)
{
    (void)sig;
    running = 0;
}

static int parse_port(const char *text, int *port)
{
    char *end;
    long parsed;

    errno = 0;
    parsed = strtol(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' ||
        parsed < 1 || parsed > 65535 || parsed > INT_MAX) {
        return -1;
    }

    *port = (int)parsed;
    return 0;
}

int main(int argc, char *argv[])
{
    char *host = "localhost";
    int port = 1883;
    char *logfile = NULL;
    int opt;

    while ((opt = getopt(argc, argv, "H:P:o:h")) != -1) {
        switch (opt) {
        case 'H':
            host = optarg;
            break;
        case 'P':
            if (parse_port(optarg, &port) != 0) {
                fprintf(stderr, "Invalid MQTT port: %s\n", optarg);
                return 1;
            }
            break;
        case 'o':
            logfile = optarg;
            break;
        case 'h':
            printf("Usage: %s [-H host] [-P port] [-o logfile]\n", argv[0]);
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

    if (logfile && logger_open(logfile) != 0) {
        perror("Failed to open log file");
        return 1;
    }

    struct device_list device_list = {0};
    if (pthread_mutex_init(&device_list.mutex, NULL) != 0) {
        fprintf(stderr, "Failed to initialize device list mutex\n");
        logger_close();
        return 1;
    }

    int mosquitto_result;

    signal(SIGINT, handle_sigint);
    mosquitto_result = mosquitto_lib_init();
    if (mosquitto_result != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "Failed to initialize Mosquitto: %s\n", mosquitto_strerror(mosquitto_result));
        pthread_mutex_destroy(&device_list.mutex);
        logger_close();
        return 1;
    }

    struct mosquitto *mosq = mosquitto_new("iot_monitor", true, &device_list);
    if (mosq == NULL) {
        fprintf(stderr, "Failed to create Mosquitto client\n");
        mosquitto_lib_cleanup();
        pthread_mutex_destroy(&device_list.mutex);
        logger_close();
        return 1;
    }

    mosquitto_disconnect_callback_set(mosq, on_disconnect);
    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_subscribe_callback_set(mosq, on_subscribe);
    mosquitto_result = mosquitto_reconnect_delay_set(mosq, 1, 10, true);

    if (mosquitto_result != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "Failed to configure reconnect: %s\n", mosquitto_strerror(mosquitto_result));
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        pthread_mutex_destroy(&device_list.mutex);
        logger_close();
        return 1;
    }

    mosquitto_message_callback_set(mosq, on_message);
    mosquitto_result = mosquitto_connect(mosq, host, port, 60);
    if (mosquitto_result != MOSQ_ERR_SUCCESS) {
        log_line("Initial MQTT connection failed: %s\n", mosquitto_strerror(mosquitto_result));
    }

    mosquitto_result = mosquitto_loop_start(mosq);
    if (mosquitto_result != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "Failed to start MQTT loop: %s\n", mosquitto_strerror(mosquitto_result));
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        free(device_list.arr);
        pthread_mutex_destroy(&device_list.mutex);
        logger_close();
        return 1;
    }

    time_t last_summary = time(NULL);

    while (running) {
        time_t now;

        sleep(1);
        now = time(NULL);

        if (now - last_summary >= 10) {
            print_device_summary(&device_list);
            last_summary = now;
        }
    }

    mosquitto_loop_stop(mosq, true);
    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    free(device_list.arr);
    pthread_mutex_destroy(&device_list.mutex);
    logger_close();

    return 0;
}
