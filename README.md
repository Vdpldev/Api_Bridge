# OTA Middleware

ESP8266 firmware that connects a Tuya MCU to an OEM controller and an MQTT
backend. The middleware translates frames in both directions, reports device
state, provisions Wi-Fi credentials on first boot, and checks for firmware
updates over HTTPS.

## What it does

- Reads and validates Tuya serial frames (`55 AA ... checksum`).
- Converts Tuya device updates into OEM frames (`7B ... 7D`).
- Converts OEM control messages back into Tuya commands.
- Publishes converted device updates to MQTT.
- Receives MQTT commands and forwards them to the OEM controller.
- Sends Tuya heartbeats and reports Wi-Fi/MQTT state to the MCU.
- Stores Wi-Fi credentials in ESP8266 EEPROM.
- Provides a temporary Wi-Fi access point and TCP configuration endpoint when
  no credentials are stored.
- Checks the OTA service daily and downloads firmware after a checksum check.

The current device profile supports one fan and four switches, plus fan speed,
backlight, child lock, and restart status data points.

## Repository layout

```text
.
├── README.md
└── main/
    ├── main.ino                 # Arduino setup/loop and message dispatch
    ├── StateMachine.ino         # Startup, Wi-Fi, MQTT, and OTA states
    ├── SerialHandler.ino        # Tuya UART framing and validation
    ├── ProtocolConversion.ino   # Tuya <-> OEM conversion
    ├── MqttHandler.ino          # MQTT connection and message handling
    ├── network.ino              # Wi-Fi, AP mode, and TCP provisioning
    ├── storage.ino              # EEPROM credential storage
    ├── OtaManager.ino           # HTTPS update and SHA-256 verification
    └── constant.h               # Protocol IDs and deployment constants
```

## Requirements

- Arduino IDE 1.8.x or 2.x.
- ESP8266 Arduino core.
- An ESP8266 board with enough flash space for OTA updates. The checked-in
  build output targets **NodeMCU v2 (ESP8266)**.
- A Tuya MCU connected to the ESP8266 UART.
- Access to the configured MQTT broker and OTA service.

Install these libraries through the Arduino Library Manager:

- **ArduinoJson**
- **PubSubClient**

The remaining dependencies come from the ESP8266 core or the Arduino
environment: `ESP8266WiFi`, `ESP8266HTTPClient`, `ESP8266httpUpdate`,
`WiFiClientSecure`, `EEPROM`, `time`, and `deque`.

## Build and flash

1. Open `main/main.ino` in Arduino IDE. Keep all files in the `main` folder
   together; Arduino automatically combines the `.ino` files during
   compilation.
2. Select an ESP8266 board, typically **NodeMCU 1.0 (ESP-12E Module)**.
3. Select the correct COM port and a flash layout that leaves room for OTA.
4. Review the deployment constants in `main/constant.h` before flashing:
   - `MQTT_BROKER` and `MQTT_PORT`
   - `mqtt_username` and `mqtt_password`
   - `OTA_URL`
   - `CURRENT_VERSION`
   - `TCP_PORT`
5. Upload the sketch.

The serial speed is **9600 baud**. The serial port is also used for Tuya
communication, so leave `DEBUG` disabled in `main/main.ino` when the UART is
connected to the MCU. Only enable it while debugging a setup that can tolerate
debug output on that serial link.

## First-boot Wi-Fi provisioning

If no Wi-Fi credentials are found in EEPROM, the device starts an access point:

| Setting | Value |
|---|---|
| SSID | `vdpltcp_<MAC-without-colons>` |
| Password | `12345678` |
| AP address | `192.168.4.1` |
| TCP port | `1234` |

Connect to that access point and send a JSON object to `192.168.4.1:1234`:

```json
{
  "ssid": "your-wifi-name",
  "password": "your-wifi-password"
}
```

The firmware stores both values in EEPROM, generates the device MQTT topic,
requests device information from the Tuya MCU, returns the response over the
TCP connection, and restarts its state machine. SSIDs and passwords are
limited to 31 characters by the current EEPROM layout.

To clear saved credentials, send the Tuya reset frame handled by
`SerialHandler.ino`, or erase the EEPROM using a suitable development
procedure. Clearing EEPROM removes the saved Wi-Fi settings.

## Runtime flow

After startup, the state machine follows this general path:

1. Initialize EEPROM, MQTT, and protocol state.
2. Load credentials, or enter AP provisioning mode.
3. Connect to Wi-Fi. The connection attempt retries after 60 seconds.
4. Connect to MQTT. MQTT retries after 5 seconds when necessary.
5. Query the Tuya MCU for initial device status.
6. Convert and publish device changes while forwarding MQTT control messages.
7. Check for OTA updates around midnight, or when the internal OTA trigger
   message `{@}` is received.

The built-in LED is used as a connection indicator: it is off while starting
or connecting to Wi-Fi and on after Wi-Fi is connected.

## MQTT topics

Topics are generated from the ESP8266 MAC address:

1. Remove colons and convert the MAC to lowercase.
2. Move the final four hexadecimal characters to the beginning.
3. Append `s`.

For example, a normalized MAC of `aabbccddeeff` becomes
`eeffaabbccdds`. The current implementation uses this generated topic for
both publishing and subscribing. MQTT credentials and broker settings are
compiled into `main/constant.h`; replace the example values before deploying
to a real environment.

## OTA updates

The firmware checks the configured service at:

```text
<OTA_URL>/api/ota/checkupdate?mac_address=<MAC>&current_version=<VERSION>
```

When an update is available, it downloads:

```text
<OTA_URL>/api/firmware/<firmware_id>/download
```

The response must provide the firmware ID, version, and SHA-256 checksum. The
device compares the calculated checksum with the server value before ending
the update and rebooting. It also reports download, success, and failure
status to:

```text
<OTA_URL>/api/ota/update-status
```

The current HTTPS client deliberately skips certificate verification with
`setInsecure()`. Use certificate validation before deploying this firmware in
an environment where transport authenticity is required.

## Serial protocols

### Tuya UART

Tuya frames begin with `55 AA` and include a command, a two-byte payload
length, payload data, and an additive checksum. Invalid checksums are
discarded. The firmware periodically sends a heartbeat and can query the MCU
for status.

### OEM frames

OEM frames begin with `{` (`0x7B`) and end with `}` (`0x7D`). The conversion
logic currently maps:

- Four relay switches
- Fan power
- Fan speed
- Backlight
- Child lock
- Restart mode

Protocol command IDs, DPID mappings, frame sizes, and device-specific values
are defined in `main/constant.h`.

## Troubleshooting

- **The AP does not appear:** clear EEPROM and reboot the device, then wait for
  the AP state to start.
- **Wi-Fi repeatedly reconnects:** verify the credentials fit the 31-character
  storage limit and confirm the network is available.
- **MQTT does not connect:** check the broker address, port, credentials, and
  generated MAC-based topic.
- **No Tuya data is visible:** confirm the UART wiring, baud rate, shared
  serial expectations, and that debug output is disabled.
- **OTA fails:** verify the firmware URL, response fields, HTTP status, image
  size, and SHA-256 checksum. The selected flash layout must also leave room
  for the new image.

## Security notes

- Do not commit production MQTT credentials or Wi-Fi passwords.
- Change the default AP password before using the provisioning flow in an
  untrusted environment.
- Replace `WiFiClientSecure::setInsecure()` with certificate validation for
  production OTA and status requests.
