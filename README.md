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
In order to make Edge communicate with MADS, there are some mandatory dependencies:
* MADS installed
* MQTT broker installed (better if dockerized)
* Access to Edge webpage

### 1 - Configure MQTT transmission
After acccessing the Edge webpage, a new job can configured: in this job the user can select the interested data coming from the machine and in order to activate the MQTT transmission, the user must perform `Advanced` --> `Enable data streaming for job`.

### 2 - MQTT while recording
Basically, the Edge capture tool transmit sets of data while recording them, so the MQTT broker should be enabled and then the MQTT transmission can happen

### 3 - Parsing
Run this plugin in order to parse and unpack the data coming from MQTT and each single array will be provided in MADS network separated by the set time period. In this way, the visualization will be just delayed from the machine, but the velocity will be the same