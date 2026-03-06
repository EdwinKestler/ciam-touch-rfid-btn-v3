/*
  button_sensor.h - Touch button sensor module implementing SensorBase.
*/
#ifndef BUTTON_SENSOR_H
#define BUTTON_SENSOR_H

#include <sensor_base.h>
#include <TouchPadButton.h>
#include <Flatbox_Publish.h>

class ButtonSensor : public SensorBase {
public:
    ButtonSensor(uint8_t pin, const String& nodeId, flatbox& publisher);
    void begin() override;
    void poll() override;
    bool hasEvent() override;
    const char* buildPayload(const char* timestamp, unsigned long seq) override;
    const char* topicName() override;
    void onPublishOk() override;
    void onPublishFail() override;

    TouchPadButton& button();

private:
    TouchPadButton _button;
    flatbox& _publisher;
    const String& _nodeId;

    int _eventCount;
    bool _eventPending;
    char _eventId[32];
};

#endif // BUTTON_SENSOR_H
