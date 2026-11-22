#ifndef TELEMETRY_PARSER_H
#define TELEMETRY_PARSER_H

#include <stddef.h>

struct telemetry {
    char device_id[64];
    double temperature;
    double humidity;
    int battery;
    long timestamp;
};

enum telemetry_result {
    TELEMETRY_OK,
    TELEMETRY_INVALID_JSON,
    TELEMETRY_MISSING_FIELD,
    TELEMETRY_INVALID_TYPE,
    TELEMETRY_INVALID_ID,
    TELEMETRY_INVALID_VALUE
};

enum telemetry_result parse_telemetry(const void *payload, size_t length, struct telemetry *out);

#endif
