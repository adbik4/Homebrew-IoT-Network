#ifndef DHT11_H
#define DHT11_H

// Definicje dla DHT11

#define DHT11_PIN 16  // GPIO4 (wiringPi numeracja), fizyczny pin 16 na Raspberry Pi
#define DHT11_MAX_TIMINGS 85
#define DHT11_WAIT_TIME 2000  // 2 sekundy między odczytami

// Funkcje do obsługi czujnika DHT11

int DHT11_Init();
void DHT11_Close();
int DHT11_ReadSensor(float *temperature, float *humidity);

#endif