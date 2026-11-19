# MQTT IoT Device Monitor

## Install dependencies

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

## Running the C application

Build with `make`, then start the C application:

```bash
./iot_monitor
```

Custom broker settings:

```bash
./iot_monitor -H localhost -P 1883
```

## Test MQTT communication

Keep the Mosquitto service running.

Subscribe to telemetry messages (-v for verbose):

```bash
mosquitto_sub -h localhost -p 1883 -t 'iot/devices/+/telemetry' -v
```

Subscribe to alert messages (-v for verbose):

```bash
mosquitto_sub -h localhost -p 1883 -t 'iot/devices/+/alert' -v
```

Start the C application:

```bash
./iot_monitor
```

Send a telemetry message manually:

```bash
mosquitto_pub -h localhost -p 1883 -t 'iot/devices/test_sensor/telemetry' \
  -m '{"device_id":"test_sensor","temperature":35.0,"humidity":85.0,"battery":15,"timestamp":1794301200}'
```

This example message should trigger all three alert types.

The Python simulator can also be used for testing:

```bash
source .venv/bin/activate
pip install -r requirements.txt
python3 mqtt_simulator.py
```
