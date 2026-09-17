# edge_parser plugin for MADS

This is a Source plugin for [MADS](https://github.com/MADS-NET/MADS). 

<provide here some introductory info>

*Required MADS version: 2.4.0.*


## Supported platforms

Currently, the supported platforms are:

* **Linux** 
* **MacOS**
* **Windows**


## Installation

Linux and MacOS:

```bash
cmake -Bbuild -DCMAKE_INSTALL_PREFIX="$(mads -p)"
cmake --build build -j4
sudo cmake --install build
```

Windows:

```powershell
cmake -Bbuild -DCMAKE_INSTALL_PREFIX="$(mads -p)"
cmake --build build --config Release
cmake --install build --config Release
```


## INI settings

The plugin supports the following settings in the INI file:

```ini
[edge_parser]
broker_host = "127.0.0.1"
broker_port = 1883
silent = false
QoS = 2
topic = "#"
pub_topic = "edge"
```

All settings are optional; if omitted, the default values are used.

## Usage
This plugin is intended to receive data streamed from an Edge controller through MQTT and then parse it so it can be processed by MADS. To make this work correctly, the network must be configured properly before starting the data stream.

The Edge controller and the PC running the MQTT broker must be directly connected with an Ethernet cable. The PC must be configured with a static IPv4 address on the network mask `255.255.255.0`, for example `192.168.1.100`. This IP is the one already expected by the Edge controller as the MQTT broker address. If a different IP is used, the MQTT client configuration on the Edge web interface must be changed accordingly.

The following items are required:
* MADS installed on the system where the plugin will run
* An MQTT broker running on the user's PC (Docker is a convenient option)
* Access to the Edge web interface

### 1 - Network setup
The PC hosting the MQTT broker must be connected to the Edge device using Ethernet. The network connector on the PC must use a static IPv4 address such as `192.168.1.100` with the netmask `255.255.255.0`.

This is important because the Edge controller is configured to communicate with the broker at that address by default. If the broker runs on another IP address, the MQTT configuration on the Edge has to be updated to match it.

### 2 - Start the MQTT broker
The MQTT broker must be running and reachable from the Edge controller before any streaming is enabled. In most cases, this is done on the user's PC, either by installing a broker directly or by running a broker container such as Docker-based Mosquitto.

Once the broker is active, it should be listening on the expected port, usually `1883` unless otherwise configured.

### 3 - Configure the Edge job for streaming
After logging in to the Edge web interface, create or open a job. In that job, select the machine data that should be transmitted. To activate the MQTT output, go to `Advanced` and enable `Enable data streaming for job`.

At this point, the Edge device starts publishing the selected data to the configured MQTT broker.

### 4 - MQTT while recording
During recording, the Edge capture tool transmits data packets continuously. This means that the MQTT broker must remain enabled and reachable while the recording session is active. If the broker is stopped or the connection is misconfigured, the data stream will not be delivered correctly.

### 5 - Parsing with this plugin
Run this plugin to receive, parse, and unpack the MQTT payload. The plugin reads the data packets published by the Edge and makes them available to MADS as arrays separated by the configured time interval.

This allows the data to be visualized in MADS with minimal delay, while preserving the same acquisition speed as the machine data being captured.