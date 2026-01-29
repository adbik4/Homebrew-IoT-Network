#ifndef DHT11_H
#define DHT11_H

// Definicje dla DHT11

#define DHT11_PIN 22  // GPIO6 (numeracja fizyczna), fizyczny pin 22 na Raspberry Pi
#define DHT11_MAX_TIMINGS 85
#define DHT11_WAIT_TIME 2  // 2 sekundy między odczytami

// Funkcje do obsługi czujnika DHT11

int DHT11_Init();
int getByte(int b, int buf[]);
int DHT11_ReadSensor(float *temperature, float *humidity);

#endif
