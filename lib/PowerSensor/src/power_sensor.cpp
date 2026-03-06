/*
  power_sensor.cpp - Battery/power monitoring sensor implementation.
  Reads A0 ADC at a configurable interval. WeMos D1 Mini voltage divider
  maps 0-3.3V input to 0-1024 ADC counts.
*/
#include "power_sensor.h"
#include <Settings.h>
#include <feedback.h>

PowerSensor::PowerSensor(uint8_t pin, const String& nodeId, flatbox& publisher, unsigned long intervalMs)
    : _pin(pin)
    , _publisher(publisher)
    , _nodeId(nodeId)
    , _intervalMs(intervalMs)
    , _lastReadMillis(0)
    , _voltage(0)
    , _adcRaw(0)
    , _eventPending(false)
{
}

void PowerSensor::begin() {
    _lastReadMillis = millis();
    _adcRaw = analogRead(_pin);
    _voltage = _adcRaw / 1024.0 * 3.3;
}

void PowerSensor::poll() {
    if (millis() - _lastReadMillis < _intervalMs) return;
    _lastReadMillis = millis();

    _adcRaw = analogRead(_pin);
    _voltage = _adcRaw / 1024.0 * 3.3;
    _eventPending = true;
}

bool PowerSensor::hasEvent() {
    return _eventPending;
}

const char* PowerSensor::buildPayload(const char* timestamp, unsigned long seq) {
    return _publisher.Evento_Power(timestamp, _voltage, _adcRaw, seq);
}

const char* PowerSensor::topicName() {
    return publishTopic;
}

void PowerSensor::onPublishOk() {
    Serial.print(F("power sensor: OK  V="));
    Serial.println(_voltage);
    feedback_publish_ok();
    _eventPending = false;
}

void PowerSensor::onPublishFail() {
    Serial.println(F("power sensor: FAILED"));
    feedback_publish_fail();
    _eventPending = false;
}
