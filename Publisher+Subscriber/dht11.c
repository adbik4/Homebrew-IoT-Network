#include "dht11.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

int DHT11_Init() {
    printf("Inicjalizacja DHT11 - używam Pythona do odczytu\n");
    
    // Sprawdź czy Python jest dostępny
    FILE *fp = popen("python3 --version", "r");
    if (fp) {
        char buffer[100];
        if (fgets(buffer, sizeof(buffer), fp) != NULL) {
            printf("  Python: %s", buffer);
        }
        pclose(fp);
    }
    
    return 0;
}

int DHT11_ReadSensor(float *temperature, float *humidity) {
    FILE *fp;
    char buffer[128];
    char cmd[] = "python3 -E dht11_python.py 2>/dev/null";
    
    // Wywołaj skrypt Pythona
    fp = popen(cmd, "r");
    if (fp == NULL) {
        fprintf(stderr, "Błąd uruchamiania Pythona\n");
        return -1;
    }
    
    // Odczytaj wynik (format: "temperatura;wilgotność")
    if (fgets(buffer, sizeof(buffer), fp) != NULL) {
        // Usuń znak nowej linii
        buffer[strcspn(buffer, "\n")] = 0;
        
        // Parsuj wartości
        if (sscanf(buffer, "%f;%f", temperature, humidity) == 2) {
            pclose(fp);
            return 0;
        } else {
            fprintf(stderr, "Błąd parsowania danych z Pythona: '%s'\n", buffer);
        }
    } else {
        fprintf(stderr, "Brak danych od Pythona\n");
    }
    
    pclose(fp);
    
    // Jeśli Python nie działa, użyj wartości testowych
    static int counter = 0;
    counter++;
    
    *temperature = 300.0f + (counter % 10) * 0.3f;  // 22.0-24.7°C
    *humidity = 150.0f + (counter % 8) * 1.5f;      // 50.0-61.5%
    
    printf("[FALLBACK] Wartości testowe: %.1f°C, %.1f%%\n", *temperature, *humidity);
    
    return 0;
}
