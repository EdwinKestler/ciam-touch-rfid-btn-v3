# FSM Test Reference -- CIAM Touch RFID Button v3

Checklist for real-life evaluator to verify every finite state machine flow path.
Open serial monitor at **115200 baud** before starting each test.

---

## Test Environment Setup

| Item | Requirement |
| --- | --- |
| Serial Monitor | 115200 baud, connected via USB |
| MQTT Broker | Running and reachable at configured address |
| WiFi | Available network with known credentials |
| RFID Card(s) | At least 2 different cards for duplicate-read testing |
| Stopwatch | For timing 30-min and 60-min periodic events (or modify `Universal_1_sec_Interval` to speed up) |

**Tip:** For accelerated testing, temporarily set `Universal_1_sec_Interval = 100` in `src/Settings.cpp`. This makes 30-min timers fire in 3 minutes and 60-min timers in 6 minutes.

---

## Flow 1: Normal Startup (no button held)

**Precondition:** Device powered off, WiFi configured, MQTT broker reachable.

| Step | Action | Expected Serial Output | LED | Buzzer | Next State |
| --- | --- | --- | --- | --- | --- |
| 1 | Power on / reset | `inicio exitosamnte el puerto Serial` | Green ON | -- | -- |
| 2 | Wait 2s (don't press button) | `estado del Boton: 0` | Green OFF, Red ON | -- | -- |
| 3 | Wait 2s (don't press button) | -- | Red OFF | -- | -- |
| 4 | WiFi auto-connects | `Wifi conectado, Direccion de IP Asignado: x.x.x.x` | -- | -- | -- |
| 5 | NTP sync (up to 5 attempts) | `servidor de NTP: <server>`, `NTP sync attempt #1`, `.....` (polling dots), `Receive NTP Response` | -- | -- | -- |
| 6 | MQTT connect (up to 4 attempts) | `Conectando al servidor MQTT: <server>`, `MQTT attempt #1`, `MQTT connected` then `Mqtt Connection Done!, sending Device Data` | -- | -- | -- |
| 7 | Subscribe topics | `se ha subscrito al Topico de respuestas` / `...Reincio Remoto` / `...Actulizaciones Remotas` / `...Control RGB` | -- | -- | -- |
| 8 | Device info summary | `CHIPID:`, `HARDWARE:`, `FIRMWARE:`, `Servidor de NTP:`, `Servidor de MQTT:`, `Puerto:`, `Usuario de MQTT:`, `Client ID:` | White OFF | -- | IDLE (0) |

**PASS criteria:** All serial messages appear in order. Device enters IDLE. No restart loops.

---

## Flow 1b: Startup -- MQTT Connect Failure (WiFiManager fallback)

**Precondition:** Device powered off, WiFi configured, MQTT broker **unreachable** (wrong server/port/credentials or firewall blocking port 1883).

| Step | Action | Expected Serial Output | LED | Buzzer | Next State |
| --- | --- | --- | --- | --- | --- |
| 1 | Power on, WiFi connects, NTP syncs | Normal startup messages up to `Time Sync, Connecting to mqtt sevrer` | -- | -- | -- |
| 2 | MQTT attempt #1 | `Conectando al servidor MQTT: <server>`, `MQTT attempt #1`, `failed, rc=<code>` | White flash (short) | -- | -- |
| 3 | MQTT attempt #2 | `MQTT attempt #2`, `failed, rc=<code>` | White flash (short) | -- | -- |
| 4 | MQTT attempt #3 | `MQTT attempt #3`, `failed, rc=<code>` | White flash (short) | -- | -- |
| 5 | MQTT attempt #4 | `MQTT attempt #4`, `failed, rc=<code>` | White flash (short) | -- | -- |
| 6a | Fallback WiFi (if configured) | `MQTT connect failed after 4 attempts`, `Intentando red WiFi de respaldo: <ssid>`, `Conectado a red WiFi de respaldo` | -- | -- | Retries MQTT |
| 6b | Fallback WiFi not configured or fails | `Opening WiFiManager...` | Purple ON | Medium beep x2 | WiFi portal active |
| 7 | Connect to `flatwifi` AP, correct settings | WiFiManager portal (includes Location, Fallback SSID/Pass fields) | -- | -- | -- |
| 8 | Save and restart | -- | -- | -- | ESP.restart() |

**PubSubClient return codes (rc= value):**

| rc | Meaning |
| --- | --- |
| -4 | Connection timeout |
| -3 | Connection lost |
| -2 | Connect failed (network unreachable) |
| -1 | Disconnected |
| 1 | Bad protocol |
| 4 | Bad credentials |
| 5 | Not authorized |

**PASS criteria:** Exactly 4 MQTT attempts logged with rc= codes. If fallback WiFi is configured, device tries it first. If fallback succeeds, MQTT is retried on new network. If fallback fails or is not configured, WiFiManager config portal opens. After saving corrected settings, device restarts and connects successfully (Flow 1).

---

## Flow 2: Startup -- WiFi Configuration Portal (button held in first 2s window)

**Precondition:** Device powered off.

| Step | Action | Expected Serial Output | LED | Buzzer | Next State |
| --- | --- | --- | --- | --- | --- |
| 1 | Power on, hold button immediately | `estado del Boton: 1` | Green ON | -- | -- |
| 2 | Keep holding through 2s window | WiFiManager portal starts | -- | -- | WiFi portal active |
| 3 | Connect to AP, configure WiFi + MQTT + NTP + Device_ID | WiFiManager logs | -- | -- | Continues to NTP/MQTT |

**PASS criteria:** WiFiManager configuration portal is accessible. Portal shows fields for: MQTT Server, Port, User, Password, NTP Server, NTP Interval, Device_ID, Location, Fallback_SSID, and Fallback_Pass. After saving, device connects to WiFi and proceeds to MQTT setup.

---

## Flow 3: Startup -- OTA Update Mode (button held in second 2s window)

**Precondition:** Device powered off. Do NOT hold button during first 2s.

| Step | Action | Expected Serial Output | LED | Buzzer | Next State |
| --- | --- | --- | --- | --- | --- |
| 1 | Power on, wait for green LED to turn off | `estado del Boton: 0` | Green ON then OFF | -- | -- |
| 2 | Hold button when red LED turns on | `Starting OTA` then `Ready` | Blue flash (medium) | Medium, Short, Medium beep pattern | RDY_TO_UPDATE_OTA (7) |
| 3 | Device creates AP: `RFID_OTA` | -- | -- | -- | Stays in state 7 |
| 4a | Upload firmware via ArduinoOTA (password: `FLATB0X_OTA`) | `OTA: update starting...`, `OTA progress: 0%`...`100%`, `OTA: update complete, rebooting` | Blue ON during upload | 3x short beep on complete | Reboots |
| 4b | Upload with wrong password | `OTA error[4]: Auth Failed` | Red flash (medium) | 2x medium beep | Stays in state 7 |
| 4c | Upload network error | `OTA error[<code>]: <type>` | Red flash (medium) | 2x medium beep | Stays in state 7 |

**OTA error codes:**

| Code | Meaning |
| --- | --- |
| 0 | Auth Failed |
| 1 | Begin Failed |
| 2 | Connect Failed |
| 3 | Receive Failed |
| 4 | End Failed |

**PASS criteria:** AP `RFID_OTA` appears. OTA upload requires password (`FLATB0X_OTA`). Upload without password shows "Auth Failed" with red flash + beeps. Successful upload shows progress percentage, blue LED solid during transfer, triple beep on completion, then device reboots.

---

## Flow 4: IDLE -- RFID Card Read (new card)

**Precondition:** Device in IDLE state, MQTT connected. Card has NOT been read before (or 5s cooldown has passed).

| Step | Action | Expected Serial Output | LED | Buzzer | Next State |
| --- | --- | --- | --- | --- | --- |
| 1 | Present RFID card to reader | Frame validated (non-empty card ID check), then `RFID CARD ID IS: <card_id>` | Green flash (short) | Short beep | TRANSMIT_SENSOR_DATA (1) |
| 2 | FSM enters state 1 | `SENSOR DATA SENT` | -- | -- | -- |
| 3 | MQTT connection check | (if disconnected: `Attempting MQTT connection...` `connected`) | White flash (if reconnecting) | Short beep (if reconnecting) | -- |
| 4 | CheckTime() runs | (if time not synced: `Time not Sync, Syncronizing time`) | -- | -- | -- |
| 5 | publishSensorEvent(rfidSensor) | Builds payload via `rfidSensor.buildPayload()` | -- | -- | -- |
| 6a | Publish SUCCESS | `enviado data de RFID: OK` | Green flash (short) | Short beep | IDLE (0) |
| 6b | Publish FAIL | `enviado data de RFID: FAILED` | Red flash (short) | -- | IDLE (0) |

**MQTT Payload (success):**

```json
{"d":{"tagdata":{"ChipID":"<NodeID>","IDeventoTag":"<NodeID>-1","Tstamp":"<ISO8601>","Tag":"<card_id>"}}}
```

**PASS criteria:** Card ID printed correctly. JSON published to `iot-2/evt/status/fmt/json`. Green LED + beep on success. State returns to IDLE.

---

## Flow 5: IDLE -- RFID Duplicate Card Read (same card within 5s)

**Precondition:** Device in IDLE state. Same card just read (OldTagRead matches).

| Step | Action | Expected Serial Output | LED | Buzzer | Next State |
| --- | --- | --- | --- | --- | --- |
| 1 | Present same RFID card again | `RFID CARD ID IS: <card_id>` then `Duplicate read, ignoring` | NO flash | NO beep | Stays IDLE (0) |

**PASS criteria:** No state transition. No LED. No buzzer. No MQTT publish. Serial shows "Duplicate read, ignoring". State remains IDLE.

---

## Flow 6: IDLE -- RFID Duplicate Card Read (same card after 5s cooldown)

**Precondition:** Device in IDLE state. Same card read > 5 seconds ago (`resetStaleTag()` timer resets internal duplicate guard).

| Step | Action | Expected Serial Output | LED | Buzzer | Next State |
| --- | --- | --- | --- | --- | --- |
| 1 | Wait > 5 seconds after last read | -- | -- | -- | IDLE (stale tag reset) |
| 2 | Present same card again | `RFID CARD ID IS: <card_id>` | Green flash (short) | Short beep | TRANSMIT_SENSOR_DATA (1) |
| 3 | Normal card publish flow | Same as Flow 4 steps 2-6 | -- | -- | IDLE (0) |

**PASS criteria:** After 5s cooldown (`resetStaleTag()`), same card is accepted as a new read.

---

## Flow 6b: IDLE -- RFID Invalid Frame (noise rejection)

**Precondition:** Device in IDLE state. RFID reader sends garbled/incomplete data that produces no valid card ID bytes at positions 4-7.

| Step | Action | Expected Serial Output | LED | Buzzer | Next State |
| --- | --- | --- | --- | --- | --- |
| 1 | Noisy/incomplete data arrives on RFID serial | `RFID: invalid frame, discarding` | NO flash | NO beep | Stays IDLE (0) |

**PASS criteria:** No state transition. No LED. No buzzer. No MQTT publish. Frame discarded with serial warning. Buffer cleared.

---

## Flow 7: IDLE -- Button Press

**Precondition:** Device in IDLE state, MQTT connected. No RFID card presented simultaneously.

| Step | Action | Expected Serial Output | LED | Buzzer | Next State |
| --- | --- | --- | --- | --- | --- |
| 1 | Touch the capacitive button | `Pressed` | Blue flash (short) | Short beep | TRANSMIT_SENSOR_DATA (1) |
| 2 | FSM enters state 1 | `SENSOR DATA SENT` | -- | -- | -- |
| 3 | MQTT connection check | (if disconnected: `Attempting MQTT connection...` `connected`) | White flash (if reconnecting) | Short beep (if reconnecting) | -- |
| 4 | CheckTime() runs | (if time not synced: `Time not Sync, Syncronizing time`) | -- | -- | -- |
| 5 | publishSensorEvent(buttonSensor) | Builds payload via `buttonSensor.buildPayload()` | -- | -- | -- |
| 6a | Publish SUCCESS | `enviado data de boton: OK` | Green flash (short) + White OFF | Short beep | IDLE (0) |
| 6b | Publish FAIL | `enviado data de boton: FAILED` | Red flash (short) + White OFF | -- | IDLE (0) |

**MQTT Payload (success):**

```json
{"d":{"botondata":{"ChipID":"<NodeID>","IDEventoBoton":"<NodeID>-1","Tstamp":"<ISO8601>"}}}
```

**PASS criteria:** Button event number increments on each press. JSON published to `iot-2/evt/status/fmt/json`. Returns to IDLE.

---

## Flow 8: IDLE -- Event Priority (card + button simultaneously)

**Precondition:** Device in IDLE state. RFID card data arrives AND button is pressed in same loop iteration.

| Step | Action | Expected Serial Output | LED | Buzzer | Next State |
| --- | --- | --- | --- | --- | --- |
| 1 | Present RFID card AND press button at same time | `RFID CARD ID IS: <card_id>` | Green flash (short) | Short beep | TRANSMIT_SENSOR_DATA (1) |
| 2 | Button sensor poll is SKIPPED (RFID is first in `sensors[]` array) | NO `Pressed` message | -- | -- | -- |
| 3 | Card flow completes normally | Same as Flow 4 steps 2-6 | -- | -- | IDLE (0) |
| 4 | Next loop: button detected (if still held) | `Pressed` | Blue flash | Short beep | TRANSMIT_SENSOR_DATA (1) |

**PASS criteria:** Sensor array order determines priority (RFID first). Button is NOT processed in the same loop iteration when RFID has an event. Button event appears in subsequent loop.

---

## Flow 9: 30-Minute Device Update (RSSI normal)

**Precondition:** Device in IDLE state for > 30 minutes. WiFi RSSI >= `rssi_low_threshold` (default -75 dBm).

| Step | Action | Expected Serial Output | LED | Buzzer | Next State |
| --- | --- | --- | --- | --- | --- |
| 1 | 30-min timer expires | -- | -- | -- | UPDATE (3) |
| 2 | updateDeviceInfo() runs | -- (msg set to "on") | -- | -- | -- |
| 3 | FSM transitions | `STATE_UPDATE` | -- | -- | TRANSMIT_DEVICE_UPDATE (5) |
| 4 | MQTT check | `STATE_TRANSMIT_DEVICE_UPDATE` (if disconnected: reconnect log) | -- | -- | -- |
| 5 | CheckTime() | -- | -- | -- | -- |
| 6 | publishRF_ID_Manejo() | `publishing device data to manageTopic:` then JSON payload | -- | -- | -- |
| 7a | Publish SUCCESS | `enviado data de dispositivo:OK` | -- | -- | IDLE (0) |
| 7b | Publish FAIL | `enviado data de dispositivo:FAILED` | -- | -- | IDLE (0) |

**MQTT Payload (success, to manageTopic):**

```json
{"d":{"Ddata":{"ChipID":"<NodeID>","Msg":"on","batt":<voltage>,"RSSI":<rssi>,"publicados":<n>,"enviados":<n>,"fallidos":<n>,"Tstamp":"<ISO8601>","Mac":"<mac>","Ip":"<ip>"}}}
```

**PASS criteria:** Device data published to `iotdevice-1/mgmt/manage`. Msg field is `"on"`. Returns to IDLE.

---

## Flow 10: 30-Minute Device Update (RSSI low -- alarm)

**Precondition:** Device in IDLE state for > 30 minutes. WiFi RSSI < `rssi_low_threshold` (default -75 dBm) (weak signal).

| Step | Action | Expected Serial Output | LED | Buzzer | Next State |
| --- | --- | --- | --- | --- | --- |
| 1 | 30-min timer expires | -- | -- | -- | UPDATE (3) |
| 2 | updateDeviceInfo() detects low RSSI | `<SSID> <RSSI_value>` | Red flash (medium) | Medium beep | -- |
| 3 | msg set to "LOWiFi" | `STATE_UPDATE` | -- | -- | TRANSMIT_DEVICE_UPDATE (5) |
| 4 | MQTT check | `STATE_TRANSMIT_DEVICE_UPDATE` | -- | -- | -- |
| 5 | publishRF_ID_Manejo() | `publishing device data to manageTopic:` then JSON payload | -- | -- | -- |
| 6a | Publish SUCCESS | `enviado data de dispositivo:OK` | -- | -- | IDLE (0) |
| 6b | Publish FAIL | `enviado data de dispositivo:FAILED` | -- | -- | IDLE (0) |

**MQTT Payload (to manageTopic):**

```json
{"d":{"Ddata":{"ChipID":"<NodeID>","Msg":"LOWiFi","batt":<voltage>,"RSSI":<rssi_negative>,...}}}
```

**PASS criteria:** Msg field is `"LOWiFi"`. Red LED flash and medium beep observed. RSSI value in serial matches actual weak signal.

---

## Flow 11: 60-Minute NTP Resync

**Precondition:** Device in IDLE state for > 60 minutes.

| Step | Action | Expected Serial Output | LED | Buzzer | Next State |
| --- | --- | --- | --- | --- | --- |
| 1 | 60-min timer expires | -- | -- | -- | UPDATE_TIME (6) |
| 2 | NTP client update | `NTP_CLIENT` | -- | -- | -- |
| 3a | NTP sync SUCCESS (within 5 retries) | -- | -- | -- | IDLE (0) |
| 3b | NTP sync FAIL (all 5 retries) | `NTP sync failed after 5 attempts, continuing...` | -- | -- | IDLE (0) |

**PASS criteria:** NTP resync attempted up to 5 times. Device returns to IDLE regardless of success/failure. Time updates if sync succeeds.

---

## Flow 12: IDLE -- Low WiFi Signal Alarm (checkalarms)

**Precondition:** Device in IDLE state. WiFi RSSI drops below -75 dBm.

| Step | Action | Expected Serial Output | LED | Buzzer | Next State |
| --- | --- | --- | --- | --- | --- |
| 1 | RSSI < -75 detected in checkalarms() | -- | White flash (long) | Long beep (1st of max 4) | Stays IDLE (0) |
| 2 | Next loop iterations (up to 3 more) | -- | White flash (long) | Long beep (2nd, 3rd, 4th) | Stays IDLE (0) |
| 3 | After 4 beeps | -- | White flash (long) continues | NO more beeps | Stays IDLE (0) |
| 4 | If RSSI recovers above -75 | -- | -- | -- | BeepSignalWarning resets to 0 |

**PASS criteria:** Maximum 4 beeps per low-signal episode. White LED flashes on every loop while signal is low. No state change -- stays IDLE. Beep counter resets when signal recovers.

---

## Flow 13: 24-Hour Normal Reset

**Precondition:** Device running continuously for > 24 hours (hora counter > 24).

| Step | Action | Expected Serial Output | LED | Buzzer | Next State |
| --- | --- | --- | --- | --- | --- |
| 1 | hora exceeds 24 | -- | -- | -- | -- |
| 2 | MQTT check | (if disconnected: `Attempting MQTT connection...`) | White flash | Short beep | -- |
| 3 | CheckTime() runs | -- | -- | -- | -- |
| 4 | publishRF_ID_Manejo() | `publishing device data to manageTopic:` then JSON with `"Msg":"24h Normal Reset"` | -- | -- | -- |
| 5 | Disconnect + restart | -- | -- | -- | ESP.restart() |

**MQTT Payload (to manageTopic):**

```json
{"d":{"Ddata":{"ChipID":"<NodeID>","Msg":"24h Normal Reset","batt":<voltage>,...}}}
```

**PASS criteria:** Device publishes "24h Normal Reset" message before restarting. Device reboots and goes through normal startup (Flow 1).

---

## Flow 14: MQTT Failure Threshold Restart

**Precondition:** Device in IDLE state. `failed` counter reaches `fail_threshold` (default 150, configurable via update topic).

| Step | Action | Expected Serial Output | LED | Buzzer | Next State |
| --- | --- | --- | --- | --- | --- |
| 1 | failed >= fail_threshold detected in IDLE | -- | -- | -- | ESP.restart() |

**PASS criteria:** Device restarts automatically. Counters (failed, published, sent) reset to 0 before restart. Device goes through normal startup.

**Note on failed counter behavior:** The `failed` counter halves on each successful publish (instead of zeroing). This means:

- 10 failures + 1 success = 5 remaining failures
- 149 failures + 1 success = 74 remaining failures
- Persistent degradation still accumulates toward threshold

---

## Flow 15: MQTT Reconnect (runtime, within FSM states)

**Precondition:** Device in any TRANSMIT state. MQTT client disconnected. This is the **runtime** reconnect via `MQTTreconnect()`, NOT the startup connect (see Flow 1b).

| Step | Action | Expected Serial Output | LED | Buzzer | Next State |
| --- | --- | --- | --- | --- | --- |
| 1 | Reconnect attempt 1 | `Attempting MQTT connection...` | White flash (short) | Short beep | -- |
| 2a | Connect SUCCESS | `connected` | -- | -- | Continues to publish |
| 2b | Connect FAIL | `failed, rc=<code> retry #:0` | Purple flash (medium) | Medium beep | -- |
| 3 | Wait 3 seconds, retry (up to 3 total) | `Attempting MQTT connection...` | White flash | Short beep | -- |
| 4 | All 3 retries fail | `MQTT reconnect failed after 3 attempts` | -- | -- | Continues (publish will fail) |

**Note:** Runtime reconnect does NOT open WiFiManager (unlike startup connect in Flow 1b). Instead, failed publishes increment the `failed` counter. When `failed` reaches 150 (FAILTRESHOLD), the device restarts (Flow 14), which then triggers the startup connect with WiFiManager fallback if the broker is still unreachable.

**PASS criteria:** Maximum 3 attempts per call (~9 seconds max blocking). Device does NOT restart on reconnect failure. Publish attempt proceeds even if reconnect fails (publish will increment `failed` counter).

---

## Flow 16: MQTT Remote Reboot

**Precondition:** Device in IDLE state, MQTT connected, subscribed to reboot topic.

| Step | Action | Expected Serial Output | LED | Buzzer | Next State |
| --- | --- | --- | --- | --- | --- |
| 1a | Publish `{"k":"A5F0"}` to `iotdm-1/mgmt/d7e3f1a2` | `Mensaje recibido desde el Topico: iotdm-1/mgmt/d7e3f1a2`, then `Reiniciando...` | -- | -- | ESP.reset() |
| 1b | Publish `{"k":"WRONG"}` to `iotdm-1/mgmt/d7e3f1a2` | `Mensaje recibido desde el Topico: iotdm-1/mgmt/d7e3f1a2`, then `Reboot: invalid token, ignoring` | -- | -- | Stays in current state |
| 1c | Publish `reboot` (plaintext) to `iotdm-1/mgmt/d7e3f1a2` | `Reboot: invalid token, ignoring` | -- | -- | Stays in current state |

**PASS criteria:** Device reboots ONLY when payload contains valid JSON token `{"k":"A5F0"}`. Invalid tokens, non-JSON payloads, and empty messages are rejected with serial warning. Topic name is non-descriptive (obscured).

---

## Flow 17: MQTT Remote Update -- Parameter Change

**Precondition:** Device in IDLE state, MQTT connected, subscribed to update topic.

| Step | Action | Expected Serial Output | LED | Buzzer | Next State |
| --- | --- | --- | --- | --- | --- |
| 1 | Publish `{"publish_interval": 2000}` to `iotdm-1/device/update` | `Mensaje recibido desde el Topico: iotdm-1/device/update` | -- | -- | -- |
| 2 | Payload parsed and applied | `Update payload:` then pretty-printed JSON, then `Updated publish_interval: 2000` | -- | -- | Stays in current state |

**Configurable parameters:** `publish_interval`, `btn_hold_time`, `tono_corto`, `tono_medio`, `tono_largo`, `flash_corto`, `flash_medio`, `flash_largo`, `fail_threshold`, `rssi_threshold`

**Multi-parameter example:**

```json
{"publish_interval": 500, "fail_threshold": 100, "rssi_threshold": -80}
```

**PASS criteria:** Each updated parameter prints `Updated <key>: <value>` to serial. Parameters not included in the payload remain unchanged. Changes are runtime only (reset on reboot).

---

## Flow 17b: MQTT Remote Update -- OTA Trigger

**Precondition:** Device in IDLE state, MQTT connected, subscribed to update topic.

| Step | Action | Expected Serial Output | LED | Buzzer | Next State |
| --- | --- | --- | --- | --- | --- |
| 1 | Publish `{"ota": true}` to `iotdm-1/device/update` | `Mensaje recibido desde el Topico: iotdm-1/device/update` | -- | -- | -- |
| 2 | OTA mode triggered | `Remote OTA trigger received`, `Starting OTA`, `Ready` | Blue flash (medium) | Medium, Short, Medium beep | RDY_TO_UPDATE_OTA (7) |
| 3 | Device creates AP: `RFID_OTA` | -- | -- | -- | Stays in state 7 |

**PASS criteria:** Device enters OTA mode exactly as in Flow 3 (step 2 onward). AP `RFID_OTA` appears. OTA upload proceeds normally.

---

## Flow 17c: MQTT Remote Control -- RGB Color

**Precondition:** Device in IDLE state, MQTT connected, subscribed to ctrl topic.

| Step | Action | Expected Serial Output | LED | Buzzer | Next State |
| --- | --- | --- | --- | --- | --- |
| 1a | Publish `{"r": 255, "g": 0, "b": 0}` to `iotdm-1/device/ctrl` | `Mensaje recibido desde el Topico: iotdm-1/device/ctrl`, `RGB set to R:255 G:0 B:0` | Red LED at full brightness | -- | Stays in current state |
| 1b | Publish `{"hex": "#00FF00"}` to `iotdm-1/device/ctrl` | `RGB set to R:0 G:255 B:0` | Green LED at full brightness | -- | Stays in current state |
| 1c | Publish `{"cmd": "on"}` to `iotdm-1/device/ctrl` | `RGB set to R:255 G:255 B:255` | All LEDs max (white) | -- | Stays in current state |
| 1d | Publish `{"cmd": "off"}` to `iotdm-1/device/ctrl` | `RGB set to R:0 G:0 B:0` | All LEDs off | -- | Stays in current state |

**PASS criteria:** LED color matches the requested RGB values. PWM dimming works (e.g., `{"r": 128, "g": 0, "b": 0}` produces half-brightness red). No state change.

---

## Flow 17d: MQTT Remote Control -- Buzzer

**Precondition:** Device in IDLE state, MQTT connected, subscribed to ctrl topic.

| Step | Action | Expected Serial Output | LED | Buzzer | Next State |
| --- | --- | --- | --- | --- | --- |
| 1 | Publish `{"beep": 3}` to `iotdm-1/device/ctrl` | `Mensaje recibido desde el Topico: iotdm-1/device/ctrl`, `Buzzer: 3 seconds` | -- | Buzzer ON for 3 seconds | Stays in current state |

**Note:** Buzzer is blocking -- the device will not process other MQTT messages until the beep finishes. Duration is clamped to 1-300 seconds. Accepts both integer (`{"beep": 5}`) and string (`{"beep": "60"}`) values.

**PASS criteria:** Buzzer sounds for the specified duration. Serial shows the parsed duration. No state change.

---

## Flow 17e: MQTT Remote Update -- WiFi Credential Push

**Precondition:** Device in IDLE state, MQTT connected.

| Step | Action | Expected Serial Output | LED | Buzzer | Next State |
| --- | --- | --- | --- | --- | --- |
| 1 | Publish `{"wifi_ssid": "NewNet", "wifi_pass": "NewPass123"}` to `iotdm-1/device/update` | `Remote WiFi credential update received`, `Stored new WiFi creds as fallback: NewNet`, `config.json guardado` | -- | -- | Stays IDLE |
| 2 | Disconnect primary WiFi AP | Device loses WiFi, MQTT reconnect fails | -- | -- | -- |
| 3 | Device tries fallback | `Intentando red WiFi de respaldo: NewNet` | -- | -- | -- |
| 4a | Fallback available | `Conectado a red WiFi de respaldo`, MQTT reconnects | -- | -- | IDLE |
| 4b | Fallback unavailable | `Fallo conexion a red WiFi de respaldo`, opens WiFiManager | Purple ON | Medium beep x2 | WiFi portal |

**PASS criteria:** New credentials are persisted to LittleFS. On primary WiFi failure, device attempts fallback SSID before falling back to WiFiManager portal.

---

## Flow 17f: MQTT Remote Update -- Location Update

**Precondition:** Device in IDLE state, MQTT connected.

| Step | Action | Expected Serial Output | LED | Buzzer | Next State |
| --- | --- | --- | --- | --- | --- |
| 1 | Publish `{"location": "Building A, Floor 2"}` to `iotdm-1/device/update` | `Updated location: Building A, Floor 2`, `config.json guardado` | -- | -- | Stays IDLE |
| 2 | Wait for next heartbeat | Heartbeat JSON contains `"Location": "Building A, Floor 2"` | -- | -- | -- |

**PASS criteria:** Location persists to LittleFS. Appears in subsequent heartbeat payloads. Survives reboot.

---

## Flow 17g: Startup -- Fallback WiFi

**Precondition:** Primary WiFi network unavailable. Fallback SSID configured in `config.json`. Fallback network available.

| Step | Action | Expected Serial Output | LED | Buzzer | Next State |
| --- | --- | --- | --- | --- | --- |
| 1 | Power on, primary WiFi fails | `Empezando Configuracion de WIFI en Automatico`, WiFiManager autoConnect fails | Purple ON | Short beep x2 | -- |
| 2 | Fallback WiFi attempted | `Intentando red WiFi de respaldo: <ssid>`, `Conectado a red WiFi de respaldo` | -- | -- | -- |
| 3 | NTP + MQTT proceed normally | Normal startup from step 5 of Flow 1 | -- | -- | IDLE |

**PASS criteria:** Device connects to fallback WiFi without user intervention. Proceeds to NTP sync and MQTT connection.

---

## Flow 18: MQTT Response Message

**Precondition:** Device in IDLE state, MQTT connected, subscribed to response topic.

| Step | Action | Expected Serial Output | LED | Buzzer | Next State |
| --- | --- | --- | --- | --- | --- |
| 1 | Publish JSON to `iotdm-1/response/` | `Mensaje recibido desde el Topico: iotdm-1/response/` | -- | -- | -- |
| 2 | Payload parsed | `Response payload:` then serialized JSON | -- | -- | Stays in current state |

**PASS criteria:** JSON payload is parsed and printed to serial. No state change.

---

## Flow 20: LWT and Retained Heartbeat

**Precondition:** Device in IDLE, MQTT connected, dashboard/MQTT client subscribed to `iot-2/evt/status/fmt/json` (manageTopic).

| Step | Action | Expected Serial Output | LED | Buzzer | Next State |
| --- | --- | --- | --- | --- | --- |
| 1 | Wait for heartbeat | Heartbeat JSON published with `retain=true` | -- | -- | -- |
| 2 | Disconnect dashboard, reconnect | Dashboard immediately receives last heartbeat (retained) without waiting | -- | -- | -- |
| 3 | Unplug device (hard power loss) | Broker publishes LWT: `{"d":{"Ddata":{"Msg":"offline"}}}` to manageTopic | -- | -- | Device off |

**PASS criteria:** Retained heartbeat available on dashboard reconnect. LWT published by broker within MQTT keepalive timeout after device power loss.

---

## Flow 21: Boot Reason and Message Sequence

**Precondition:** Device powered off.

| Step | Action | Expected Serial Output | LED | Buzzer | Next State |
| --- | --- | --- | --- | --- | --- |
| 1 | Power on | `Boot reason: Power On` (or similar) | -- | -- | -- |
| 2 | Wait for first heartbeat | JSON contains `"boot_reason": "Power On"` and `"seq": 1` | -- | -- | -- |
| 3 | Trigger button press | Button JSON contains `"seq": 2` | -- | -- | -- |
| 4 | Present RFID card | Card JSON contains `"seq": 3` | -- | -- | -- |
| 5 | Remote reboot via MQTT | After reboot: `Boot reason: Software/System restart` | -- | -- | -- |

**PASS criteria:** `seq` increments monotonically across all message types. `boot_reason` reflects actual reset cause. Server can detect gaps in seq numbers.

---

## Flow 22: RFID Rate Limiting

**Precondition:** Device in IDLE state.

| Step | Action | Expected Serial Output | LED | Buzzer | Next State |
| --- | --- | --- | --- | --- | --- |
| 1 | Present card A | Normal publish, `"seq": N` | Green flash | Short beep | IDLE |
| 2 | Wait 6s (past duplicate cooldown), present card A again | `RFID: rate limited, same card too soon` | -- | -- | IDLE |
| 3 | Wait 60s total, present card A again | Normal publish, `"seq": N+1` | Green flash | Short beep | IDLE |
| 4 | Present card B (different) within 60s | Normal publish (rate limit is per-card-ID) | Green flash | Short beep | IDLE |

**PASS criteria:** Same card cannot be published more than once per 60s. Different cards are not affected by rate limit.

---

## Flow 23: Exponential Backoff on MQTT Reconnect

**Precondition:** Device in IDLE, MQTT broker taken offline.

| Step | Action | Expected Serial Output | LED | Buzzer | Next State |
| --- | --- | --- | --- | --- | --- |
| 1 | Broker goes down | `Attempting MQTT connection...`, `failed, rc=<code> backoff: 3000ms` | White flash | Short beep | IDLE |
| 2 | Next reconnect cycle | `failed, rc=<code> backoff: 6000ms` | White flash | Short beep | IDLE |
| 3 | Next reconnect cycle | `failed, rc=<code> backoff: 12000ms` | -- | -- | IDLE |
| 4 | Continue until cap | Backoff increases to 60000ms max | -- | -- | IDLE |
| 5 | Broker comes back | `connected`, backoff resets to 3000ms | -- | -- | IDLE |

**PASS criteria:** Backoff doubles each cycle, caps at 60s. Resets to 3s on successful connect.

---

## Flow 24: Non-Blocking Remote Buzzer

**Precondition:** Device in IDLE state, MQTT connected.

| Step | Action | Expected Serial Output | LED | Buzzer | Next State |
| --- | --- | --- | --- | --- | --- |
| 1 | Publish `{"beep": 10}` to ctrl topic | `Buzzer: 10 seconds` | -- | Buzzer ON | IDLE |
| 2 | Present RFID card while buzzer is active | Card read processed normally | Green flash | -- | TRANSMIT_SENSOR_DATA |
| 3 | After 10s | Buzzer turns OFF automatically | -- | Buzzer OFF | IDLE |

**PASS criteria:** FSM continues processing card reads and button presses while buzzer is active. Buzzer turns off automatically after specified duration.

---

## Flow 25: Config Migration

**Precondition:** Device has old `config.json` without `config_version` field (from firmware < v5.00), or has v2 config (binary XOR password, from v5.00).

| Step | Action | Expected Serial Output | LED | Buzzer | Next State |
| --- | --- | --- | --- | --- | --- |
| 1a | Flash v6.00 on device with v0/v1 config (no `config_version`) | `Config migration: v0 -> v3`, `config.json guardado` | -- | -- | -- |
| 1b | Flash v6.00 on device with v2 config (binary XOR) | `Config v2 detected: password corrupted by binary XOR, using default`, `Config migration: v2 -> v3`, `config.json guardado` | -- | -- | -- |
| 2 | Subsequent reboots | No migration message (version matches) | -- | -- | -- |

**PASS criteria:** Old config files are automatically migrated to v3 (hex-encoded XOR password). v2 configs reset password to default (must re-enter via WiFiManager if not `esp8266`). No data loss for other fields.

---

## Flow 26: v6.00 Modular Refactor Smoke Test

**Precondition:** Device flashed with v6.00 firmware.

| Step | Action | Expected Serial Output | LED | Buzzer | Next State |
| --- | --- | --- | --- | --- | --- |
| 1 | Power on, normal startup | Same serial output as v5.00 (boot reason, config load, WiFi, NTP, MQTT) | Same LED sequence | Same beep sequence | IDLE |
| 2 | Present RFID card | `RFID CARD ID IS: <id>`, green flash, beep, JSON published | Green flash | Short beep | TRANSMIT_SENSOR -> IDLE |
| 3 | Press touch button | `Pressed`, blue flash, beep, JSON published | Blue flash | Short beep | TRANSMIT_SENSOR -> IDLE |
| 4 | Wait for heartbeat | `STATE_UPDATE`, heartbeat published to manageTopic with retain | -- | -- | UPDATE -> TRANSMIT -> IDLE |
| 5 | Send remote RGB command | `RGB set to R:x G:x B:x` | LED color changes | -- | IDLE |
| 6 | Send remote buzzer | `Buzzer: N seconds`, FSM continues | -- | Non-blocking beep | IDLE |
| 7 | Send remote reboot (`{"k":"A5F0"}` to obscured topic) | `Reiniciando...`, device reboots | -- | -- | -- |

**PASS criteria:** All behavior identical to v5.00. No regressions from modular refactor.

---

## Flow 27: Sensor Abstraction Verification

**Precondition:** Device flashed with v6.00 firmware (Phase 2 sensor abstraction).

| Step | Action | Expected Serial Output | LED | Buzzer | Next State |
| --- | --- | --- | --- | --- | --- |
| 1 | Present RFID card | `RFID CARD ID IS: <id>`, then `SENSOR DATA SENT` (not `CARD DATA SENT`) | Green flash | Short beep | TRANSMIT_SENSOR_DATA (1) -> IDLE |
| 2 | Press touch button | `Pressed`, then `SENSOR DATA SENT` (not `BOTON DATA SENT`) | Blue flash | Short beep | TRANSMIT_SENSOR_DATA (1) -> IDLE |
| 3 | Present card + press button simultaneously | RFID processed first (array order), button in next loop | Green flash | Short beep | TRANSMIT_SENSOR_DATA -> IDLE -> TRANSMIT_SENSOR_DATA -> IDLE |
| 4 | Verify JSON payloads | Card JSON: `tagdata` with `IDeventoTag`. Button JSON: `botondata` with `IDEventoBoton` | -- | -- | -- |
| 5 | Wait 5s, present same card | Card accepted (stale tag reset via `resetStaleTag()`) | Green flash | Short beep | TRANSMIT_SENSOR_DATA -> IDLE |
| 6 | Present same card within 60s | `RFID: rate limited, same card too soon` | -- | -- | Stays IDLE |

**PASS criteria:** Both sensors use unified `TRANSMIT_SENSOR_DATA` state and `publishSensorEvent()`. Serial shows `SENSOR DATA SENT` (not sensor-specific state names). JSON payloads unchanged from v5.00. Sensor-specific feedback (LED color, beep) preserved. Rate limiting and duplicate detection work as before.

---

## Flow 19: Unknown FSM State (safety net)

**Precondition:** `fsm_state` set to an undefined value (e.g., via memory corruption or bug). Cannot be triggered manually in normal operation.

| Step | Action | Expected Serial Output | LED | Buzzer | Next State |
| --- | --- | --- | --- | --- | --- |
| 1 | FSM switch hits default case | `FSM: unknown state, resetting to IDLE` | -- | -- | IDLE (0) |

**PASS criteria:** Device recovers to IDLE without restart. Warning printed to serial.

---

## Summary Checklist

| # | Flow | Trigger | Key Verification | Pass? |
| --- | --- | --- | --- | --- |
| 1 | Normal Startup | Power on (no button) | Serial summary, enters IDLE | [ ] |
| 1b | MQTT Connect Failure | Broker unreachable at startup | 4 retries, WiFiManager fallback, rc= codes logged | [ ] |
| 2 | WiFi Config Portal | Button in 1st 2s | WiFiManager AP accessible | [ ] |
| 3 | OTA Update Mode | Button in 2nd 2s | `RFID_OTA` AP, OTA password required, progress/error feedback | [ ] |
| 4 | Card Read (new) | Present new RFID card | Green flash, beep, JSON published | [ ] |
| 5 | Card Read (duplicate) | Same card within 5s | "Duplicate read, ignoring", NO flash/beep | [ ] |
| 6 | Card Read (after cooldown) | Same card after > 5s | Accepted as new read | [ ] |
| 6b | Invalid RFID Frame | Noisy serial data | "RFID: invalid frame, discarding", NO flash/beep | [ ] |
| 7 | Button Press | Touch button | Blue flash, beep, JSON published | [ ] |
| 8 | Priority: Card vs Button | Both at same time | RFID wins (first in sensors[]), button deferred to next loop | [ ] |
| 9 | 30-min Update (normal) | Timer expires, RSSI OK | Msg="on", published to manageTopic | [ ] |
| 10 | 30-min Update (low WiFi) | Timer expires, RSSI < -75 | Msg="LOWiFi", red flash, medium beep | [ ] |
| 11 | 60-min NTP Resync | Timer expires | NTP sync attempted (5 retries max) | [ ] |
| 12 | Low WiFi Alarm | RSSI < -75 in IDLE | White flash, max 4 beeps, stays IDLE | [ ] |
| 13 | 24-Hour Reset | hora > 24 | "24h Normal Reset" published, then restart | [ ] |
| 14 | Fail Threshold Restart | failed >= fail_threshold | Device restarts, counters zeroed | [ ] |
| 15 | MQTT Reconnect | Broker unreachable | Max 3 retries with exponential backoff, then continues | [ ] |
| 16 | Remote Reboot | `{"k":"A5F0"}` on obscured topic | Reboots with valid token, rejects invalid | [ ] |
| 17 | Remote Update (params) | MQTT update JSON | Parameters applied, "Updated" logs | [ ] |
| 17b | Remote OTA Trigger | `{"ota": true}` on update topic | Enters OTA mode (same as Flow 3) | [ ] |
| 17c | Remote RGB Control | RGB/hex/on/off on ctrl topic | LED color changes, PWM dimming works | [ ] |
| 17d | Remote Buzzer | `{"beep": N}` on ctrl topic | Buzzer sounds for N seconds | [ ] |
| 17e | Remote WiFi Push | `wifi_ssid`+`wifi_pass` on update topic | Creds stored as fallback, used on next reconnect | [ ] |
| 17f | Remote Location | `location` on update topic | Location persisted, appears in heartbeat | [ ] |
| 17g | Startup Fallback WiFi | Primary WiFi down, fallback configured | Connects to fallback without user intervention | [ ] |
| 18 | Response Message | MQTT response message | JSON printed, no state change | [ ] |
| 19 | Unknown State | Invalid fsm_state value | "FSM: unknown state", recovers to IDLE | [ ] |
| 20 | LWT + Retained Heartbeat | Power loss / dashboard reconnect | LWT published by broker, retained heartbeat available | [ ] |
| 21 | Boot Reason + Seq | Power on, trigger events | boot_reason in heartbeat, seq increments monotonically | [ ] |
| 22 | RFID Rate Limit | Same card within 60s | "rate limited", rejected. After 60s, accepted | [ ] |
| 23 | Exponential Backoff | Broker offline | Backoff 3s->6s->12s->...->60s cap, resets on connect | [ ] |
| 24 | Non-Blocking Buzzer | Remote beep + card read | FSM processes events during active buzzer | [ ] |
| 25 | Config Migration | Old config.json | Auto-migrates to v3 (hex XOR), v2 resets password | [ ] |
| 26 | v6.00 Smoke Test | Normal operation | All behavior identical to v5.00 after modular refactor | [ ] |
| 27 | Sensor Abstraction | Card + button via sensor array | Unified TRANSMIT_SENSOR_DATA state, generic publishSensorEvent | [ ] |

---

## LED Reference

| Color | Variable | Pin | Meaning |
| --- | --- | --- | --- |
| Green | Verde | D7 | Success (publish OK, card read accepted) |
| Blue | Azul | D6 | Button press acknowledged / OTA mode entered |
| Red | Rojo | D8 | Failure (publish failed) / Low WiFi alarm in updateDeviceInfo |
| White | Blanco | D6+D7+D8 | MQTT reconnect attempt / Low WiFi in checkalarms / Turned off after button publish |
| Purple | Purpura | D4 | MQTT reconnect failure |
| Custom | via MQTT | D6+D7+D8 | Remote RGB control via `iotdm-1/device/ctrl` (PWM 0-255 per channel) |

## Buzzer Reference

| Tone | Variable | Meaning |
| --- | --- | --- |
| Short | tono_corto | Card read, button press, publish success, MQTT reconnect attempt |
| Medium | tono_medio | MQTT reconnect fail, low WiFi alarm in updateDeviceInfo, OTA mode |
| Long | tono_largo | Low WiFi alarm in checkalarms (up to 4x) |
| Remote | via MQTT | Remote buzzer activation via `{"beep": N}` on ctrl topic (1-300 seconds, **non-blocking** -- FSM keeps running) |

## Counter Behavior Reference

| Counter | Incremented | Decremented/Reset | Threshold |
| --- | --- | --- | --- |
| `sent` | Every publish attempt | Reset to 0 on fail_threshold restart | -- |
| `published` | Every successful publish | Reset to 0 on fail_threshold restart | -- |
| `failed` | Every failed publish (+1) | Halved on success (/2) | >= `fail_threshold` (default 150) triggers restart |
| `BeepSignalWarning` | Each low-RSSI beep (+1) | Reset to 0 when RSSI >= `rssi_low_threshold` | Max 4 beeps |
| `msg_seq` | Every MQTT publish (+1) | Never reset (monotonic since boot) | -- (server detects gaps) |
| `mqtt_backoff_ms` | Doubles on reconnect fail | Reset to 3000 on connect success | Max 60000ms |
| `hora` | Every 60 minutes (+1) | Reset to 0 on 24h restart | > 24 triggers restart |
| `ButtonSensor::_eventCount` | Each button press (+1) | Never reset (session lifetime) | -- |
| `RFIDSensor::_eventCount` | Each new card publish (+1) | Never reset (session lifetime) | -- |
