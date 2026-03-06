/*
  button_sensor.cpp - Touch button sensor implementation.
*/
#include "button_sensor.h"
#include <Settings.h>
#include <feedback.h>

ButtonSensor::ButtonSensor(uint8_t pin, const String& nodeId, flatbox& publisher)
    : _button(pin)
    , _publisher(publisher)
    , _nodeId(nodeId)
    , _eventCount(0)
    , _eventPending(false)
{
    _eventId[0] = '\0';
}

void ButtonSensor::begin() {
    // TouchPadButton is ready after construction
}

void ButtonSensor::poll() {
    if (_button.check()) {
        Serial.println(F("Pressed"));
        _eventCount++;
        snprintf(_eventId, sizeof(_eventId), "%s-%d", _nodeId.c_str(), _eventCount);
        feedback_button_press();
        _eventPending = true;
    }
}

bool ButtonSensor::hasEvent() {
    return _eventPending;
}

const char* ButtonSensor::buildPayload(const char* timestamp, unsigned long seq) {
    return _publisher.Evento_Boton(timestamp, _eventId, seq);
}

const char* ButtonSensor::topicName() {
    return publishTopic;
}

void ButtonSensor::onPublishOk() {
    Serial.println(F("enviado data de boton: OK"));
    feedback_publish_ok();
    Blanco.COff();
    _eventPending = false;
}

void ButtonSensor::onPublishFail() {
    Serial.println(F("enviado data de boton: FAILED"));
    feedback_publish_fail();
    Blanco.COff();
    _eventPending = false;
}

TouchPadButton& ButtonSensor::button() {
    return _button;
}
