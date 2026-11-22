#include "telemetry_parser.h"

#include <cjson/cJSON.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include <time.h>

static int valid_device_id(const char *id)
{
    if (id == NULL || id[0] == '\0') {
        return 0;
    }

    size_t length = strlen(id);
    if (length >= sizeof(((struct telemetry *)0)->device_id)) {
        return 0;
    }

    for (size_t i = 0; i < length; i++) {
        char c = id[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-')) {
            return 0;
        }
    }
    return 1;
}

enum telemetry_result parse_telemetry(const void *payload, size_t length, struct telemetry *out)
{
    if (payload == NULL || out == NULL || length == 0) {
        return TELEMETRY_INVALID_JSON;
    }

    cJSON *json = cJSON_ParseWithLengthOpts(payload, length, NULL, 0);
    if (json == NULL) {
        return TELEMETRY_INVALID_JSON;
    }

    enum telemetry_result result = TELEMETRY_OK;
    if (!cJSON_IsObject(json)) {
        result = TELEMETRY_INVALID_TYPE;
        goto cleanup;
    }

    const cJSON *id = cJSON_GetObjectItem(json, "device_id");
    const cJSON *temp = cJSON_GetObjectItem(json, "temperature");
    const cJSON *hum = cJSON_GetObjectItem(json, "humidity");
    const cJSON *battery = cJSON_GetObjectItem(json, "battery");
    const cJSON *timestamp = cJSON_GetObjectItem(json, "timestamp");

    if (!id || !temp || !hum || !battery || !timestamp) {
        result = TELEMETRY_MISSING_FIELD;
        goto cleanup;
    }
    if (!cJSON_IsString(id) || !cJSON_IsNumber(temp) || !cJSON_IsNumber(hum) ||
        !cJSON_IsNumber(battery) || !cJSON_IsNumber(timestamp)) {
        result = TELEMETRY_INVALID_TYPE;
        goto cleanup;
    }
    if (!valid_device_id(id->valuestring)) {
        result = TELEMETRY_INVALID_ID;
        goto cleanup;
    }

    double t = temp->valuedouble;
    double h = hum->valuedouble;
    double b = battery->valuedouble;
    double ts = timestamp->valuedouble;
    if (!isfinite(t) || !isfinite(h) || h < 0.0 || h > 100.0 ||
        !isfinite(b) || b < 0.0 || b > 100.0 || floor(b) != b ||
        !isfinite(ts) || ts < 0.0 || ts >= (double)LONG_MAX || floor(ts) != ts) {
        result = TELEMETRY_INVALID_VALUE;
        goto cleanup;
    }

    long parsed_ts = (long)ts;
    time_t clock_ts = (time_t)parsed_ts;
    if (localtime(&clock_ts) == NULL) {
        result = TELEMETRY_INVALID_VALUE;
        goto cleanup;
    }

    struct telemetry parsed = {0};
    strcpy(parsed.device_id, id->valuestring);
    parsed.temperature = t;
    parsed.humidity = h;
    parsed.battery = (int)b;
    parsed.timestamp = parsed_ts;
    *out = parsed;

cleanup:
    cJSON_Delete(json);
    return result;
}
