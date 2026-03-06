/*
  sensor_base.h - Abstract base class for plug-and-play sensor modules.
  Part of the CIAM Touch RFID Button platform.
*/
#ifndef SENSOR_BASE_H
#define SENSOR_BASE_H

class SensorBase {
public:
    virtual void begin() = 0;
    virtual void poll() = 0;
    virtual bool hasEvent() = 0;
    virtual const char* buildPayload(const char* timestamp, unsigned long seq) = 0;
    virtual const char* topicName() = 0;
    virtual void onPublishOk() = 0;
    virtual void onPublishFail() = 0;
    virtual ~SensorBase() = default;
};

#endif // SENSOR_BASE_H
