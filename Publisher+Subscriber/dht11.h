#ifndef DHT11_H
#define DHT11_H

// Definicje dla DHT11, podłączenie na GPIO5
#define DHT11_PIN 5  // wybranie GPIO5 (fizyczny pin 29 na Raspberry Pi)
#define DHT11_WAIT_TIME 2  // Czas przerwy między odczytami, ok. 2 sekundy

// Funkcje do obsługi czujnika DHT11
int DHT11_Init();
int DHT11_ReadSensor(float *temperature, float *humidity);

#endif
