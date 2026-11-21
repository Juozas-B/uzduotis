# MQTT IoT Device Monitor

## Project overview

MQTT IoT Device Monitor is a C application that receives telemetry from
multiple simulated IoT devices, displays the latest device readings, and
publishes alerts when sensor values exceed the configured thresholds.

## Install required dependencies

```bash
sudo apt update
sudo apt install build-essential libmosquitto-dev libcjson-dev mosquitto mosquitto-clients python3-venv
```

## Starting and checking Mosquitto

Start Mosquitto:

```bash
sudo systemctl start mosquitto
```

Check Mosquitto:

```bash
systemctl status mosquitto
```

## Building the C application

```bash
make
```

Clean build artifacts:

```bash
make clean
```

## Running the Python simulator

Create and activate the Python virtual environment:

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

Start the Python simulator:

```bash
python3 mqtt_simulator.py
```

## MQTT topics and message format

Telemetry is published to:

```text
iot/devices/<device_id>/telemetry
```

The application subscribes to all device telemetry with the `+` wildcard.
Alerts are published to:

```text
iot/devices/<device_id>/alert
```

Telemetry messages must contain the following JSON fields:

```json
{
  "device_id": "sensor_01",
  "temperature": 24.5,
  "humidity": 56.2,
  "battery": 85,
  "timestamp": 1794301200
}
```

## Running the C application

Build with `make`, then start the C application:

```bash
./iot_monitor
```

The C application connects to `localhost` on port `1883` by default. You can override the broker and logging settings with these command-line options:

| Option | Description | Default |
| --- | --- | --- |
| `-H <host>` | MQTT broker hostname or IP address | `localhost` |
| `-P <port>` | MQTT broker port | `1883` |
| `-o <file>` | Write application logs to a file | Standard output |
| `-t <temperature>` | High-temperature alert threshold in degrees C | `30.0` |
| `-u <humidity>` | High-humidity alert threshold in percent | `80.0` |
| `-b <battery>` | Low-battery alert threshold in percent | `20` |
| `-h` | Display the command-line help message | N/A |

For example, connect to a broker at `192.168.1.10:1883` and save logs to
`monitor.log`:

```bash
./iot_monitor -H 192.168.1.10 -P 1883 -o monitor.log
```

To use custom alert thresholds:

```bash
./iot_monitor -t 35 -u 85 -b 15
```

To display all available options:

```bash
./iot_monitor -h
```

## Test MQTT communication

The tests below were run with the local Mosquitto broker. Start it before
testing:

```bash
sudo systemctl start mosquitto
```

The broker status command produced:

```text
active
```

The commands below can be run in separate terminals as indicated.

### 1. Connect to the broker

**Command:**

```bash
./iot_monitor
```

**Expected:** The application connects to the local broker.

**Produced:**

```text
Connected to MQTT broker
```

### 2. Receive telemetry from multiple devices

**Command:**

```bash
source .venv/bin/activate
python3 -u mqtt_simulator.py
```

**Expected:** Telemetry is published for multiple device IDs and displayed by
the C application.

**Produced by the simulator:**

```text
MQTT simulator started
Published: iot/devices/sensor_01/telemetry -> {'device_id': 'sensor_01', 'temperature': 25.12, 'humidity': 54.75, 'battery': 85, 'timestamp': 1795170244}
Published: iot/devices/sensor_02/telemetry -> {'device_id': 'sensor_02', 'temperature': 30.18, 'humidity': 58.17, 'battery': 45, 'timestamp': 1795170244}
Published: iot/devices/sensor_03/telemetry -> {'device_id': 'sensor_03', 'temperature': 22.38, 'humidity': 89.41, 'battery': 17, 'timestamp': 1795170244}
```

**Produced by the C application:**

```text
[12:24:04] Device: sensor_01
Temperature: 25.12°C
Humidity:    54.75%
Battery:     85%

[12:24:04] Device: sensor_02
Temperature: 30.18°C
Humidity:    58.17%
Battery:     45%

[12:24:04] Device: sensor_03
Temperature: 22.38°C
Humidity:    89.41%
Battery:     17%
```

### 3. Parse and display a telemetry message

**Command:**

```bash
mosquitto_pub -h localhost -p 1883 \
  -t 'iot/devices/test_sensor/telemetry' \
  -m '{"device_id":"test_sensor","temperature":24.5,"humidity":56.2,"battery":85,"timestamp":1794301200}'
```

**Expected:** The application parses the JSON and displays all telemetry
fields.

**Produced:**

```text
[11:00:00] Device: test_sensor
Temperature: 24.50°C
Humidity:    56.20%
Battery:     85%
```

### 4. Detect high temperature

**Command:**

```bash
mosquitto_pub -h localhost -p 1883 \
  -t 'iot/devices/test_sensor/telemetry' \
  -m '{"device_id":"test_sensor","temperature":35.0,"humidity":50.0,"battery":85,"timestamp":1794301200}'
```

**Expected:** A `HIGH_TEMPERATURE` warning is printed.

**Produced:**

```text
WARNING: HIGH_TEMPERATURE for device test_sensor: 35.00°C
```

### 5. Detect high humidity

**Command:**

```bash
mosquitto_pub -h localhost -p 1883 \
  -t 'iot/devices/test_sensor/telemetry' \
  -m '{"device_id":"test_sensor","temperature":25.0,"humidity":85.0,"battery":85,"timestamp":1794301200}'
```

**Expected:** A `HIGH_HUMIDITY` warning is printed.

**Produced:**

```text
WARNING: HIGH_HUMIDITY for device test_sensor: 85.00%
```

### 6. Detect low battery

**Command:**

```bash
mosquitto_pub -h localhost -p 1883 \
  -t 'iot/devices/test_sensor/telemetry' \
  -m '{"device_id":"test_sensor","temperature":25.0,"humidity":50.0,"battery":15,"timestamp":1794301200}'
```

**Expected:** A `LOW_BATTERY` warning is printed.

**Produced:**

```text
WARNING: LOW_BATTERY for device test_sensor: 15%
```

### 7. Publish alerts to the correct topic

Run this subscriber in another terminal:

```bash
mosquitto_sub -h localhost -p 1883 \
  -t 'iot/devices/+/alert' -v
```

Publish one message that triggers all three alerts:

```bash
mosquitto_pub -h localhost -p 1883 -t 'iot/devices/test_sensor/telemetry' \
  -m '{"device_id":"test_sensor","temperature":35.0,"humidity":85.0,"battery":15,"timestamp":1794301200}'
```

**Expected:** Three alert messages are published to
`iot/devices/test_sensor/alert`.

**Produced by the alert subscriber:**

```text
iot/devices/test_sensor/alert {"device_id":"test_sensor","alert":"HIGH_TEMPERATURE","value":35,"timestamp":1794301200}
iot/devices/test_sensor/alert {"device_id":"test_sensor","alert":"HIGH_HUMIDITY","value":85,"timestamp":1794301200}
iot/devices/test_sensor/alert {"device_id":"test_sensor","alert":"LOW_BATTERY","value":15,"timestamp":1794301200}
```

### 8. Handle malformed JSON

**Command:**

```bash
mosquitto_pub -h localhost -p 1883 \
  -t 'iot/devices/test_sensor/telemetry' \
  -m '{"device_id":"test_sensor","temperature":35.0'
```

**Expected:** The application reports the malformed message and keeps
running.

**Produced:**

```text
Invalid JSON
```

Publish a valid message afterward to confirm that message processing resumes:

```bash
mosquitto_pub -h localhost -p 1883 \
  -t 'iot/devices/test_sensor/telemetry' \
  -m '{"device_id":"test_sensor","temperature":24.0,"humidity":55.0,"battery":85,"timestamp":1794301200}'
```

**Produced afterward:**

```text
[11:00:00] Device: test_sensor
Temperature: 24.00°C
Humidity:    55.00%
Battery:     85%
```

### 9. Handle broker disconnection and reconnection

While `iot_monitor` is running, stop and restart Mosquitto:

```bash
sudo systemctl stop mosquitto
sleep 3
sudo systemctl start mosquitto
sleep 8
```

**Expected:** The application reports the lost connection, reconnects, and
receives telemetry again.

**Produced by the application during the broker restart:**

```text
Connected to MQTT broker

Disconnected: The connection was lost.
Connected to MQTT broker
```

After the broker is active again, publish the following message four times:

```bash
mosquitto_pub -h localhost -p 1883 \
  -t 'iot/devices/test_sensor/telemetry' \
  -m '{"device_id":"test_sensor","temperature":24.0,"humidity":55.0,"battery":85,"timestamp":1794301200}'
```

**Produced after reconnection:**

```text
[11:00:00] Device: test_sensor
Temperature: 24.00°C
Humidity:    55.00%
Battery:     85%

[11:00:00] Device: test_sensor
Temperature: 24.00°C
Humidity:    55.00%
Battery:     85%

[11:00:00] Device: test_sensor
Temperature: 24.00°C
Humidity:    55.00%
Battery:     85%

[11:00:00] Device: test_sensor
Temperature: 24.00°C
Humidity:    55.00%
Battery:     85%
```

### 10. Terminate gracefully

**Command:**

```text
Ctrl+C
```

**Expected:** The application exits cleanly.

**Produced:**

```text
Connected to MQTT broker

Disconnected: No error.
```

## Known limitations

- The application uses an unauthenticated local Mosquitto broker by default.
- Broker credentials and TLS settings are not configurable through the CLI.
- Device telemetry is stored in memory and is not persisted between runs.
