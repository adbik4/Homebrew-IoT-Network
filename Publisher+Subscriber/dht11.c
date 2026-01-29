#include "dht11.h"
#include <wiringPi.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

int DHT11_Init() {
    if (wiringPiSetupPinType(WPI_PIN_WPI) == -1) {
        fprintf(stderr, "Błąd: Nie można zainicjalizować wiringPi\n");
        return -1;
    }
    
    pinMode(DHT11_PIN, OUTPUT);
    digitalWrite(DHT11_PIN, HIGH);
    
    // Opóźnienie na początek
    delay(1000);
    
    return 0;
}

int DHT11_ReadSensor(float *temperature, float *humidity) {
    uint8_t data[5] = {0, 0, 0, 0, 0};
    uint8_t laststate = HIGH;
    uint8_t counter = 0;
    uint8_t j = 0, i;
    
    // Przygotuj czujnik
    pinMode(DHT11_PIN, OUTPUT);
    digitalWrite(DHT11_PIN, LOW);
    delay(18);  // Minimum 18ms dla DHT11
    
    // Rozpocznij odczyt
    digitalWrite(DHT11_PIN, HIGH);
    delayMicroseconds(40);
    pinMode(DHT11_PIN, INPUT);
    
    // Odczytaj 40 bitów danych
    for (i = 0; i < 85; i++) {
        counter = 0;
        while (digitalRead(DHT11_PIN) == laststate) {
            counter++;
            delayMicroseconds(1);
            if (counter == 255) {
                break;  // Timeout
            }
        }
        
        laststate = digitalRead(DHT11_PIN);
        
        if (counter == 255) {
            break;  // Timeout
        }
        
        // Ignoruj pierwsze 3 przejścia (odpowiedź czujnika)
        if ((i >= 4) && (i % 2 == 0)) {
            // Zbierz każdy bit (0 lub 1)
            data[j / 8] <<= 1;
            if (counter > 16) {  // Jeśli czas wysoki > 16us, to bit 1
                data[j / 8] |= 1;
            }
            j++;
        }
    }
    
    // Przywróć stan pinu
    pinMode(DHT11_PIN, OUTPUT);
    digitalWrite(DHT11_PIN, HIGH);
    
    // Sprawdź czy odebrano 40 bitów i sumę kontrolną
    if ((j >= 40) && (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF))) {
        // DHT11: data[0] = humidity integer, data[1] = humidity decimal
        //        data[2] = temperature integer, data[3] = temperature decimal
        
        float humidity_temp = (float)data[0] + (float)data[1] / 10.0f;
        float temperature_temp = (float)data[2] + (float)data[3] / 10.0f;
        
        // Sprawdź zakresy
        if (humidity_temp > 100.0f || temperature_temp > 50.0f || temperature_temp < 0.0f) {
            return -1;  // Nieprawidłowe wartości
        }
        
        *humidity = humidity_temp;
        *temperature = temperature_temp;
        return 0;
    }
    
    return -1;  // Błąd odczytu
}
