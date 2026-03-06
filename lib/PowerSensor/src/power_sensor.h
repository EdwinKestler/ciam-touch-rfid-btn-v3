/*
  power_sensor.h - Battery/power monitoring sensor implementing SensorBase.
  Reads ADC voltage on a configurable interval and publishes as a sensor event.
*/
#ifndef POWER_SENSOR_H
#define POWER_SENSOR_H

#include <sensor_base.h>
#include <Flatbox_Publish.h>

class PowerSensor : public SensorBase {
public:
    PowerSensor(uint8_t pin, const String& nodeId, flatbox& publisher, unsigned long intervalMs);
    void begin() override;
    void poll() override;
    bool hasEvent() override;
    const char* buildPayload(const char* timestamp, unsigned long seq) override;
    const char* topicName() override;
    void onPublishOk() override;
    void onPublishFail() override;

    float voltage() const { return _voltage; }
    int adcRaw() const { return _adcRaw; }

private:
    uint8_t _pin;
    flatbox& _publisher;
    const String& _nodeId;
    unsigned long _intervalMs;
    unsigned long _lastReadMillis;
    float _voltage;
    int _adcRaw;
    bool _eventPending;
};

#endif // POWER_SENSOR_H
