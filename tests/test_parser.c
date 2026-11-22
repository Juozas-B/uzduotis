#include "telemetry_parser.h"

#include <stdio.h>
#include <string.h>

#define VALID "{\"device_id\":\"sensor_01\",\"temperature\":24.5,\"humidity\":56.2,\"battery\":85,\"timestamp\":1794301200}"

static int total = 0;
static int passed = 0;

static void check(const char *name, const char *json, enum telemetry_result expected)
{
    struct telemetry value = {0};
    enum telemetry_result actual = parse_telemetry(json, strlen(json), &value);
    int success = (actual == expected);
    if (success && actual == TELEMETRY_OK && strcmp(json, VALID) == 0) {
        success = strcmp(value.device_id, "sensor_01") == 0 &&
        value.temperature == 24.5 && 
        value.humidity == 56.2 && 
        value.battery == 85 && 
        value.timestamp == 1794301200L;
    }
    total++;
    passed += success;
    printf("[%s] %s\n", success ? "PASS" : "FAIL", name);
    if (!success) {
        printf("       expected result %d, got %d\n", (int)expected, (int)actual);
    }
}

int main(void)
{
    check("Valid telemetry", VALID, TELEMETRY_OK);
    check("Malformed JSON", "{invalid", TELEMETRY_INVALID_JSON);
    check("Empty JSON", "", TELEMETRY_INVALID_JSON);
    check("JSON array instead of object", "[]", TELEMETRY_INVALID_TYPE);
    check("Missing temperature", "{\"device_id\":\"x\",\"humidity\":50,\"battery\":70,\"timestamp\":100}", TELEMETRY_MISSING_FIELD);
    check("Wrong battery type", "{\"device_id\":\"x\",\"temperature\":20,\"humidity\":50,\"battery\":\"hello\",\"timestamp\":100}", TELEMETRY_INVALID_TYPE);
    check("Empty device ID", "{\"device_id\":\"\",\"temperature\":20,\"humidity\":50,\"battery\":70,\"timestamp\":100}", TELEMETRY_INVALID_ID);
    check("Device ID with slash", "{\"device_id\":\"foo/bar\",\"temperature\":20,\"humidity\":50,\"battery\":70,\"timestamp\":100}", TELEMETRY_INVALID_ID);
    check("Humidity over 100", "{\"device_id\":\"x\",\"temperature\":20,\"humidity\":150,\"battery\":70,\"timestamp\":100}", TELEMETRY_INVALID_VALUE);
    check("Negative humidity", "{\"device_id\":\"x\",\"temperature\":20,\"humidity\":-1,\"battery\":70,\"timestamp\":100}", TELEMETRY_INVALID_VALUE);
    check("Battery over 100", "{\"device_id\":\"x\",\"temperature\":20,\"humidity\":50,\"battery\":101,\"timestamp\":100}", TELEMETRY_INVALID_VALUE);
    check("Fractional battery", "{\"device_id\":\"x\",\"temperature\":20,\"humidity\":50,\"battery\":12.5,\"timestamp\":100}", TELEMETRY_INVALID_VALUE);
    check("Negative timestamp", "{\"device_id\":\"x\",\"temperature\":20,\"humidity\":50,\"battery\":70,\"timestamp\":-1}", TELEMETRY_INVALID_VALUE);
    check("Fractional timestamp", "{\"device_id\":\"x\",\"temperature\":20,\"humidity\":50,\"battery\":70,\"timestamp\":1.5}", TELEMETRY_INVALID_VALUE);

    check("High temperature is valid telemetry", "{\"device_id\":\"sensor_01\",\"temperature\":31.5,\"humidity\":56.2,\"battery\":85,\"timestamp\":1794301200}", TELEMETRY_OK);

    printf("\n%d/%d tests passed\n", passed, total);
    return passed == total ? 0 : 1;
}
