/*
  rfid_sensor.h - RFID reader sensor module implementing SensorBase.
*/
#ifndef RFID_SENSOR_H
#define RFID_SENSOR_H

#include <sensor_base.h>
#include <SoftwareSerial.h>
#include <Flatbox_Publish.h>

#define RFID_FRAME_SIZE 13
#define RFID_RATE_LIMIT_MS 60000UL
#define RFID_STALE_RESET_MS 5000UL

class RFIDSensor : public SensorBase {
public:
    RFIDSensor(uint8_t rxPin, uint8_t txPin, const String& nodeId, flatbox& publisher);
    void begin() override;
    void poll() override;
    bool hasEvent() override;
    const char* buildPayload(const char* timestamp, unsigned long seq) override;
    const char* topicName() override;
    void onPublishOk() override;
    void onPublishFail() override;

    void resetStaleTag();

private:
    SoftwareSerial _serial;
    flatbox& _publisher;
    const String& _nodeId;

    byte _tagID[RFID_FRAME_SIZE];
    String _inputString;
    String _oldTagRead;
    String _lastPublishedCardID;
    unsigned long _lastCardPublishMillis;
    unsigned long _lastStaleCheckMillis;
    int _eventCount;
    bool _eventPending;
    char _eventId[32];
};

#endif // RFID_SENSOR_H
