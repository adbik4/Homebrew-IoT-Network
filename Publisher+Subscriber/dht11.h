#ifndef DHT11_H
#define DHT11_H

// Definicje dla DHT11
#define DHT11_PIN 4  // GPIO4 (fizyczny pin 7)
#define DHT11_WAIT_TIME 2  // 2 sekundy między odczytami

// Funkcje do obsługi czujnika DHT11
int DHT11_Init();
int DHT11_ReadSensor(float *temperature, float *humidity);

#endif
