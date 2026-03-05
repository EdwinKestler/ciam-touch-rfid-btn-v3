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

**Tip:** For accelerated testing, temporarily set `Universal_1_sec_Interval = 100` in Settings.h. This makes 30-min timers fire in 3 minutes and 60-min timers in 6 minutes.

---

## Flow 1: Normal Startup (no button held)

**Precondition:** Device powered off, WiFi configured, MQTT broker reachable.

| Step | Action | Expected Serial Output | LED | Buzzer | Next State |
| --- | --- | --- | --- | --- | --- |
| 1 | Power on / reset | `inicio exitosamnte el puerto Serial` | Green ON | -- | -- |
| 2 | Wait 2s (don't press button) | `estado del Boton: 0` | Green OFF, Red ON | -- | -- |
| 3 | Wait 2s (don't press button) | -- | Red OFF | -- | -- |
| 4 | WiFi auto-connects | `Wifi conectado, Direccion de IP Asignado: x.x.x.x` | -- | -- | -- |
| 5 | NTP sync | `servidor de NTP: <server>`, `.....` (polling dots), `Receive NTP Response` | -- | -- | -- |
| 6 | MQTT connect (up to 4 attempts) | `Conectando al servidor MQTT: <server>`, `MQTT attempt #1`, `MQTT connected` then `Mqtt Connection Done!, sending Device Data` | -- | -- | -- |
| 7 | Subscribe topics | `se ha subscrito al Topico de respuestas` / `...Reincio Remoto` / `...Actulizaciones Remotas` | -- | -- | -- |
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
| 6 | WiFiManager fallback | `MQTT connect failed after 4 attempts, opening WiFiManager...` | Purple ON | Medium beep x2 | WiFi portal active |
| 7 | Connect to `flatwifi` AP, correct MQTT settings | WiFiManager portal logs | -- | -- | -- |
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

**PASS criteria:** Exactly 4 attempts logged with rc= codes. WiFiManager config portal opens after 4th failure. After saving corrected MQTT settings, device restarts and connects successfully (Flow 1).

---

## Flow 2: Startup -- WiFi Configuration Portal (button held in first 2s window)

**Precondition:** Device powered off.

| Step | Action | Expected Serial Output | LED | Buzzer | Next State |
| --- | --- | --- | --- | --- | --- |
| 1 | Power on, hold button immediately | `estado del Boton: 1` | Green ON | -- | -- |
| 2 | Keep holding through 2s window | WiFiManager portal starts | -- | -- | WiFi portal active |
| 3 | Connect to AP, configure WiFi | WiFiManager logs | -- | -- | Continues to NTP/MQTT |

**PASS criteria:** WiFiManager configuration portal is accessible. After saving credentials, device connects to WiFi and proceeds to MQTT setup.

---

## Flow 3: Startup -- OTA Update Mode (button held in second 2s window)

**Precondition:** Device powered off. Do NOT hold button during first 2s.

| Step | Action | Expected Serial Output | LED | Buzzer | Next State |
| --- | --- | --- | --- | --- | --- |
| 1 | Power on, wait for green LED to turn off | `estado del Boton: 0` | Green ON then OFF | -- | -- |
| 2 | Hold button when red LED turns on | `Starting OTA` then `Ready` | Blue flash (medium) | Medium, Short, Medium beep pattern | RDY_TO_UPDATE_OTA (7) |
| 3 | Device creates AP: `RFID_OTA` | -- | -- | -- | Stays in state 7 |
| 4 | Upload firmware via ArduinoOTA | OTA progress logs | -- | -- | Stays in state 7 |

**PASS criteria:** AP `RFID_OTA` appears. OTA upload completes successfully. Device loops in state 7 indefinitely (no IDLE transition).

---

## Flow 4: IDLE -- RFID Card Read (new card)

**Precondition:** Device in IDLE state, MQTT connected. Card has NOT been read before (or 5s cooldown has passed).

| Step | Action | Expected Serial Output | LED | Buzzer | Next State |
| --- | --- | --- | --- | --- | --- |
| 1 | Present RFID card to reader | `RFID CARD ID IS: <card_id>` | Green flash (short) | Short beep | TRANSMIT_CARD_DATA (2) |
| 2 | FSM enters state 2 | `CARD DATA SENT` | -- | -- | -- |
| 3 | MQTT connection check | (if disconnected: `Attempting MQTT connection...` `connected`) | White flash (if reconnecting) | Short beep (if reconnecting) | -- |
| 4 | CheckTime() runs | (if time not synced: `Time not Sync, Syncronizing time`) | -- | -- | -- |
| 5 | publishRF_ID_Lectura() | `publishing Tag data to publishTopic:` then JSON payload | -- | -- | -- |
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

**Precondition:** Device in IDLE state. Same card read > 5 seconds ago (RetardoLectura timer resets OldTagRead to "1").

| Step | Action | Expected Serial Output | LED | Buzzer | Next State |
| --- | --- | --- | --- | --- | --- |
| 1 | Wait > 5 seconds after last read | -- | -- | -- | IDLE (OldTagRead reset to "1") |
| 2 | Present same card again | `RFID CARD ID IS: <card_id>` | Green flash (short) | Short beep | TRANSMIT_CARD_DATA (2) |
| 3 | Normal card publish flow | Same as Flow 4 steps 2-6 | -- | -- | IDLE (0) |

**PASS criteria:** After 5s cooldown, same card is accepted as a new read.

---

## Flow 7: IDLE -- Button Press

**Precondition:** Device in IDLE state, MQTT connected. No RFID card presented simultaneously.

| Step | Action | Expected Serial Output | LED | Buzzer | Next State |
| --- | --- | --- | --- | --- | --- |
| 1 | Touch the capacitive button | `Pressed` | Blue flash (short) | Short beep | TRANSMIT_BOTON_DATA (1) |
| 2 | FSM enters state 1 | `BOTON DATA SENT` | -- | -- | -- |
| 3 | MQTT connection check | (if disconnected: `Attempting MQTT connection...` `connected`) | White flash (if reconnecting) | Short beep (if reconnecting) | -- |
| 4 | CheckTime() runs | (if time not synced: `Time not Sync, Syncronizing time`) | -- | -- | -- |
| 5 | publish_Boton_Data() | `publishing device publishTopic metadata:` then JSON payload | -- | -- | -- |
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
| 1 | Present RFID card AND press button at same time | `RFID CARD ID IS: <card_id>` | Green flash (short) | Short beep | TRANSMIT_CARD_DATA (2) |
| 2 | `readBtn()` is SKIPPED | NO `Pressed` message | -- | -- | -- |
| 3 | Card flow completes normally | Same as Flow 4 steps 2-6 | -- | -- | IDLE (0) |
| 4 | Next loop: button detected (if still held) | `Pressed` | Blue flash | Short beep | TRANSMIT_BOTON_DATA (1) |

**PASS criteria:** Card takes priority. Button is NOT processed in the same loop iteration. `readBtn()` output only appears in subsequent loop.

---

## Flow 9: 30-Minute Device Update (RSSI normal)

**Precondition:** Device in IDLE state for > 30 minutes. WiFi RSSI >= -75 dBm.

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

**Precondition:** Device in IDLE state for > 30 minutes. WiFi RSSI < -75 dBm (weak signal).

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

**Precondition:** Device in IDLE state. `failed` counter reaches 150 (FAILTRESHOLD).

| Step | Action | Expected Serial Output | LED | Buzzer | Next State |
| --- | --- | --- | --- | --- | --- |
| 1 | failed >= 150 detected in IDLE | -- | -- | -- | ESP.restart() |

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
| 1 | Publish message to `iotdm-1/mgmt/initiate/device/reboot` | `Mensaje recibido desde el Topico: iotdm-1/mgmt/initiate/device/reboot` | -- | -- | -- |
| 2 | Reboot triggered | `Reiniciando...` | -- | -- | ESP.reset() |

**PASS criteria:** Device reboots immediately upon receiving reboot message. Normal startup follows.

---

## Flow 17: MQTT Remote Update (config push)

**Precondition:** Device in IDLE state, MQTT connected, subscribed to update topic.

| Step | Action | Expected Serial Output | LED | Buzzer | Next State |
| --- | --- | --- | --- | --- | --- |
| 1 | Publish JSON to `iotdm-1/device/update` | `Mensaje recibido desde el Topico: iotdm-1/device/update` | -- | -- | -- |
| 2 | Payload parsed | `Update payload:` then pretty-printed JSON | -- | -- | Stays in current state |

**PASS criteria:** JSON payload is parsed and printed to serial. No state change. No restart.

---

## Flow 18: MQTT Response Message

**Precondition:** Device in IDLE state, MQTT connected, subscribed to response topic.

| Step | Action | Expected Serial Output | LED | Buzzer | Next State |
| --- | --- | --- | --- | --- | --- |
| 1 | Publish JSON to `iotdm-1/response/` | `Mensaje recibido desde el Topico: iotdm-1/response/` | -- | -- | -- |
| 2 | Payload parsed | `Response payload:` then serialized JSON | -- | -- | Stays in current state |

**PASS criteria:** JSON payload is parsed and printed to serial. No state change.

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
| 3 | OTA Update Mode | Button in 2nd 2s | `RFID_OTA` AP, stays in state 7 | [ ] |
| 4 | Card Read (new) | Present new RFID card | Green flash, beep, JSON published | [ ] |
| 5 | Card Read (duplicate) | Same card within 5s | "Duplicate read, ignoring", NO flash/beep | [ ] |
| 6 | Card Read (after cooldown) | Same card after > 5s | Accepted as new read | [ ] |
| 7 | Button Press | Touch button | Blue flash, beep, JSON published | [ ] |
| 8 | Priority: Card vs Button | Both at same time | Card wins, button deferred to next loop | [ ] |
| 9 | 30-min Update (normal) | Timer expires, RSSI OK | Msg="on", published to manageTopic | [ ] |
| 10 | 30-min Update (low WiFi) | Timer expires, RSSI < -75 | Msg="LOWiFi", red flash, medium beep | [ ] |
| 11 | 60-min NTP Resync | Timer expires | NTP sync attempted (5 retries max) | [ ] |
| 12 | Low WiFi Alarm | RSSI < -75 in IDLE | White flash, max 4 beeps, stays IDLE | [ ] |
| 13 | 24-Hour Reset | hora > 24 | "24h Normal Reset" published, then restart | [ ] |
| 14 | Fail Threshold Restart | failed >= 150 | Device restarts, counters zeroed | [ ] |
| 15 | MQTT Reconnect | Broker unreachable | Max 3 retries (~9s), then continues | [ ] |
| 16 | Remote Reboot | MQTT reboot message | "Reiniciando...", device reboots | [ ] |
| 17 | Remote Update | MQTT update message | JSON printed, no state change | [ ] |
| 18 | Response Message | MQTT response message | JSON printed, no state change | [ ] |
| 19 | Unknown State | Invalid fsm_state value | "FSM: unknown state", recovers to IDLE | [ ] |

---

## LED Reference

| Color | Variable | Pin | Meaning |
| --- | --- | --- | --- |
| Green | Verde | D7 | Success (publish OK, card read accepted) |
| Blue | Azul | D6 | Button press acknowledged / OTA mode entered |
| Red | Rojo | D8 | Failure (publish failed) / Low WiFi alarm in updateDeviceInfo |
| White | Blanco | D6+D7+D8 | MQTT reconnect attempt / Low WiFi in checkalarms / Turned off after button publish |
| Purple | Purpura | D4 | MQTT reconnect failure |

## Buzzer Reference

| Tone | Variable | Meaning |
| --- | --- | --- |
| Short | tono_corto | Card read, button press, publish success, MQTT reconnect attempt |
| Medium | tono_medio | MQTT reconnect fail, low WiFi alarm in updateDeviceInfo, OTA mode |
| Long | tono_largo | Low WiFi alarm in checkalarms (up to 4x) |

## Counter Behavior Reference

| Counter | Incremented | Decremented/Reset | Threshold |
| --- | --- | --- | --- |
| `sent` | Every publish attempt | Reset to 0 on FAILTRESHOLD restart | -- |
| `published` | Every successful publish | Reset to 0 on FAILTRESHOLD restart | -- |
| `failed` | Every failed publish (+1) | Halved on success (/2) | >= 150 triggers restart |
| `BeepSignalWarning` | Each low-RSSI beep (+1) | Reset to 0 when RSSI >= -75 | Max 4 beeps |
| `hora` | Every 60 minutes (+1) | Reset to 0 on 24h restart | > 24 triggers restart |
| `Numero_ID_Evento_Boton` | Each button press (+1) | Never reset (session lifetime) | -- |
| `Numero_ID_Eventos_Tarjeta` | Each new card publish (+1) | Never reset (session lifetime) | -- |
