#ifndef MQTT_H
#define MQTT
#include <stdint.h>

int Connect();
void Disconnect();
void Publish(uint16_t temp, uint16_t humidity);
void Subscribe(uint8_t target_id, uint8_t action);

void HandleIncomingData();

#endif