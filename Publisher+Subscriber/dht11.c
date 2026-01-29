#include "dht11.h"
#include <wiringPi.h>
#include <stdio.h>
#include <stdint.h>

// Inicjalizacja wiringPi i czujnika DHT11
int DHT11_Init() {
    // Inicjalizacja wiringPi
    if (wiringPiSetupPinType(WPI_PIN_PHYS) == -1) {
        perror("wiringPiSetup");
        return -1;
    }
    
    // Ustawienie pinu DHT11 jako wyjście na początku
    pinMode(DHT11_PIN, OUTPUT);
    digitalWrite(DHT11_PIN, HIGH);
    
    return 0;
}

// Odczyt danych z czujnika DHT11
int DHT11_ReadSensor(float *temperature, float *humidity) {
    uint8_t laststate = HIGH;
    uint8_t counter = 0;
    uint8_t j = 0, i;
    int dht11_dat[5] = {0, 0, 0, 0, 0};
    
    // Przygotowanie pinu
    pinMode(DHT11_PIN, OUTPUT);
    digitalWrite(DHT11_PIN, LOW);
    delay(18);  // 18ms zgodnie z dokumentacją DHT11
    
    // Przełączenie pinu na wejście z podciągnięciem do wysokiego stanu
    pinMode(DHT11_PIN, INPUT);
    
    // Oczekiwanie na odpowiedź czujnika
    for (i = 0; i < DHT11_MAX_TIMINGS; i++) {
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
        
        // Ignorujemy pierwsze 3 przejścia
        if ((i >= 4) && (i % 2 == 0)) {
            // Każdy bit jest kodowany długością stanu wysokiego
            dht11_dat[j / 8] <<= 1;
            if (counter > 16) {
                dht11_dat[j / 8] |= 1;
            }
            j++;
        }
    }
    
    // Sprawdzenie poprawności odczytu (suma kontrolna)
    if ((j >= 40) && 
        (dht11_dat[4] == ((dht11_dat[0] + dht11_dat[1] + dht11_dat[2] + dht11_dat[3]) & 0xFF))) {
        // Konwersja danych
        *humidity = (float)dht11_dat[0];  // Wilgotność w %
        *temperature = (float)dht11_dat[2];  // Temperatura w °C
        
        // DHT11 może zwracać ujemną temperaturę (w formacie uzupełnienia do dwóch)
        if (dht11_dat[2] & 0x80) {
            *temperature = -(float)(dht11_dat[2] & 0x7F);
        }
        
        // Ograniczenie zakresu wilgotności do 0-100%
        if (*humidity < 0) *humidity = 0;
        if (*humidity > 100) *humidity = 100;
        
        return 0;  // Sukces
    }
    
    return -1;  // Błąd odczytu
}

// Zamknięcie czujnika (zwolnienie zasobów)
void DHT11_Close() {
    // Dla DHT11 nie ma specjalnych operacji do wykonania
    // wiringPi nie wymaga specjalnego zamykania
}
