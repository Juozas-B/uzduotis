#ifndef DEVICE_STORE_H
#define DEVICE_STORE_H

struct device {
    char device_id[64];
    double temperature;
    double humidity;
    int battery;
    long timestamp;
};

struct device_list {
    struct device *arr;
    int count;
    int capacity;
};

struct device *find_or_add_device(struct device_list *list, const char *device_id);

#endif
