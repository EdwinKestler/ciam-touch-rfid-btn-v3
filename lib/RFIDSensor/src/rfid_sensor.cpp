/*
  rfid_sensor.cpp - RFID reader sensor implementation.
*/
#include "rfid_sensor.h"
#include <Settings.h>
#include <feedback.h>

RFIDSensor::RFIDSensor(uint8_t rxPin, uint8_t txPin, const String& nodeId, flatbox& publisher)
    : _serial(rxPin, txPin, false)
    , _publisher(publisher)
    , _nodeId(nodeId)
    , _oldTagRead("1")
    , _lastCardPublishMillis(0)
    , _lastStaleCheckMillis(0)
    , _eventCount(0)
    , _eventPending(false)
{
    memset(_tagID, 0, sizeof(_tagID));
    _eventId[0] = '\0';
}

void RFIDSensor::begin() {
    _serial.begin(9600);
    _lastStaleCheckMillis = millis();
}

void RFIDSensor::poll() {
    if (!_serial.available()) return;

    _inputString = "";
    unsigned int count = 0;
    while (_serial.available() > 0) {
        byte incoming = _serial.read();
        _tagID[count] = incoming;
        if (count > 3 && count < 8) {
            _inputString += incoming;
        }
        delay(2);
        if (count == 12) break;
        count++;
    }

    // Validate frame
    bool allZero = true;
    for (unsigned int i = 0; i < _inputString.length(); i++) {
        if (_inputString[i] != '0' && _inputString[i] != '\0') { allZero = false; break; }
    }
    if (_inputString.length() == 0 || allZero) {
        Serial.println(F("RFID: invalid frame, discarding"));
        _inputString = "";
        memset(_tagID, 0, sizeof(_tagID));
        return;
    }

    Serial.print(F("RFID CARD ID IS: "));
    Serial.println(_inputString);

    if (_oldTagRead == _inputString) {
        Serial.println(F("Duplicate read, ignoring"));
        return;
    }

    // Rate limit: same card published too recently
    if (_inputString == _lastPublishedCardID && millis() - _lastCardPublishMillis < RFID_RATE_LIMIT_MS) {
        Serial.println(F("RFID: rate limited, same card too soon"));
        _oldTagRead = _inputString;
        _inputString = "";
        return;
    }

    feedback_card_read();
    _eventPending = true;
}

bool RFIDSensor::hasEvent() {
    return _eventPending;
}

const char* RFIDSensor::buildPayload(const char* timestamp, unsigned long seq) {
    _oldTagRead = _inputString;
    _lastPublishedCardID = _inputString;
    _lastCardPublishMillis = millis();
    _eventCount++;
    snprintf(_eventId, sizeof(_eventId), "%s-%d", _nodeId.c_str(), _eventCount);
    return _publisher.Evento_Tarjeta(_eventId, timestamp, _inputString, seq);
}

const char* RFIDSensor::topicName() {
    return publishTopic;
}

void RFIDSensor::onPublishOk() {
    Serial.println(F("enviado data de RFID: OK"));
    feedback_publish_ok();
    _inputString = "";
    memset(_tagID, 0, sizeof(_tagID));
    _eventPending = false;
}

void RFIDSensor::onPublishFail() {
    Serial.println(F("enviado data de RFID: FAILED"));
    feedback_publish_fail();
    _oldTagRead = "1";
    _inputString = "";
    memset(_tagID, 0, sizeof(_tagID));
    _eventPending = false;
}

void RFIDSensor::resetStaleTag() {
    if (millis() - _lastStaleCheckMillis > RFID_STALE_RESET_MS) {
        _oldTagRead = "1";
        _lastStaleCheckMillis = millis();
    }
}
