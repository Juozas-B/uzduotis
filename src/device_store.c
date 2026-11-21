#include "device_store.h"
#include "logger.h"

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct device *find_or_add_device(struct device_list *list, const char *device_id)
{
    for (int i = 0; i < list->count; i++) {
        if (strcmp(list->arr[i].device_id, device_id) == 0) {
            return &list->arr[i];
        }
    }

    if (list->count == list->capacity) {
        int new_capacity;
        struct device *new_arr;

        if (list->capacity > INT_MAX / 2 || (size_t)list->capacity > SIZE_MAX / sizeof(struct device) / 2) {
            return NULL;
        }

        new_capacity = list->capacity == 0 ? 1 : list->capacity * 2;
        new_arr = realloc(list->arr, (size_t)new_capacity * sizeof(struct device));
        if (new_arr == NULL) {
            return NULL;
        }

        list->capacity = new_capacity;
        list->arr = new_arr;
    }

    strcpy(list->arr[list->count].device_id, device_id);
    return &list->arr[list->count++];
}

void print_device_summary(struct device_list *list)
{
    time_t now = time(NULL);

    if (pthread_mutex_lock(&list->mutex) != 0) {
        log_line("Failed to lock device list\n");
        return;
    }

    log_line("\n--- Device summary ---\n");

    if (list->count == 0) {
        log_line("No devices have sent telemetry yet\n");
    }

    for (int i = 0; i < list->count; i++) {
        const struct device *device = &list->arr[i];
        time_t elapsed = now - device->last_received;

        log_line("%s | temperature: %.2f C | humidity: %.2f%% | battery: %d%% | "
                 "last message: %ld seconds ago\n",
                 device->device_id,
                 device->temperature,
                 device->humidity,
                 device->battery,
                 (long)elapsed);
    }

    log_line("----------------------\n\n\n");
    pthread_mutex_unlock(&list->mutex);
}

void check_stopped_devices(struct device_list *list, time_t timeout)
{
    time_t now = time(NULL);

    if (pthread_mutex_lock(&list->mutex) != 0) {
        log_line("Failed to lock device list\n");
        return;
    }

    for (int i = 0; i < list->count; i++) {
        struct device *device = &list->arr[i];
        time_t elapsed = now - device->last_received;

        if (elapsed >= timeout && !device->inactive) {
            log_line("WARNING: Device %s stopped sending telemetry. " "(last message %ld seconds ago)\n",
                     device->device_id, (long)elapsed);
            device->inactive = 1;
        }
    }

    pthread_mutex_unlock(&list->mutex);
}
