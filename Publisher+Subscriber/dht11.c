#include "dht11.h"
#include <wiringPi.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

// Inicjalizacja wiringPi i czujnika DHT11
int DHT11_Init() {
    // Inicjalizacja wiringPi
    if (wiringPiSetupGpio() == -1) {
        fprintf(stderr, "Błąd: Nie można zainicjalizować wiringPi\n");
        return -1;
    }
    
    // Ustawienie pinu DHT11 jako wyjście na początku
    pinMode(DHT11_PIN, OUTPUT);
    digitalWrite(DHT11_PIN, HIGH);
    
    // Opóźnienie inicjalizacji
    delay(1000);
    
    return 0;
}

// Odczyt danych z czujnika DHT11 (poprawiona wersja)
int DHT11_ReadSensor(float *temperature, float *humidity) {
    uint8_t data[5] = {0, 0, 0, 0, 0};
    uint8_t laststate = HIGH;
    uint8_t counter = 0;
    uint8_t j = 0, i;
    
    // Czekaj na gotowość czujnika
    digitalWrite(DHT11_PIN, HIGH);
    delay(250);
    
    // Rozpocznij odczyt
    pinMode(DHT11_PIN, OUTPUT);
    digitalWrite(DHT11_PIN, LOW);
    delay(18);
    
    // Przygotuj pin do odczytu
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
                break;
            }
        }
        laststate = digitalRead(DHT11_PIN);
        
        if (counter == 255) {
            break;
        }
        
        // Ignoruj pierwsze 3 przejścia
        if ((i >= 4) && (i % 2 == 0)) {
            // Zbierz każdy bit
            data[j / 8] <<= 1;
            if (counter > 16) {
                data[j / 8] |= 1;
            }
            j++;
        }
    }
    
    // Sprawdź sumę kontrolną i zwróc dane
    if ((j >= 40) && (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF))) {
        *humidity = (float)data[0] + (float)data[1] / 10.0f;
        *temperature = (float)data[2] + (float)data[3] / 10.0f;
        
        // Sprawdź poprawność zakresów
        if (*humidity > 100.0f || *temperature > 80.0f || *temperature < 0.0f) {
            return -1; // Nieprawidłowe wartości
        }
        
        return 0;
    } else {
        return -1; // Błąd sumy kontrolnej
    }
}
