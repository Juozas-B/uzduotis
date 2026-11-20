#ifndef DEVICE_STORE_H
#define DEVICE_STORE_H

#include <pthread.h>
#include <time.h>

struct device {
    char device_id[64];
    double temperature;
    double humidity;
    int battery;
    long timestamp;
    time_t last_received;
};

struct device_list {
    struct device *arr;
    int count;
    int capacity;
    pthread_mutex_t mutex;
};

struct device *find_or_add_device(struct device_list *list, const char *device_id);
void print_device_summary(struct device_list *list);

#endif
