#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <stdint.h>
#include <stdbool.h>

#include "constants.h"
#include "data_structs.h"
#include "simple_MQTT.h"
#include "dht11.h"

// Zmienne globalne
int tcp_listen_socket = -1;
int tcp_conn_socket = -1;
int udp_socket = -1;
bool is_running = true;
uint8_t client_id = 0;
uint8_t target_id = 0;
uint8_t subscription_action = 0;
bool is_publisher = false;

void SignalHandler(int sig);

int main(int argc, char *argv[]) {
    // Rejestracja handlera sygnałów
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);
    
    // Parsowanie argumentów
    if (argc < 2) {
        fprintf(stderr, "Użycie:\n");
        fprintf(stderr, "  %s <ID>                 - publikuj dane z podanym ID (hex)\n", argv[0]);
        fprintf(stderr, "  %s 0xFF <ID> <akcja>    - zarządzaj nasłuchem dla podanego ID (hex)\n", argv[0]);
        fprintf(stderr, "    akcja: 0 - rozpocznij nasłuch, 1 - zrezygnuj z nasłuchu\n");
        return 1;
    }
    
    // Konwersja ID z hex
    uint8_t id1 = (uint8_t)strtol(argv[1], NULL, 0);
    
    if (id1 != SPECIAL_LISTEN_ID) {
        // Tryb publikowania - wymagany 1 argument dodatkowy
        if (argc != 2) {
            fprintf(stderr, "Błąd: Dla trybu publikowania wymagany jest tylko 1 argument\n");
            fprintf(stderr, "Użycie: %s <ID>\n", argv[0]);
            return 1;
        }
        
        is_publisher = true;
        client_id = id1;
        printf("Klient publikujący z ID: 0x%02X\n", client_id);
        
        // Inicjalizacja wiringPi i czujnika DHT11
        printf("Inicjalizacja wiringPi i czujnika DHT11...\n");
        if (DHT11_Init() != 0) {
            fprintf(stderr, "Błąd: Nie udało się zainicjalizować wiringPi lub czujnika DHT11\n");
            fprintf(stderr, "Upewnij się, że:\n");
            fprintf(stderr, "1. Czujnik DHT11 jest podłączony do pinu GPIO6 (fizyczny pin 22)\n");
            fprintf(stderr, "2. Zainstalowano wiringPi: sudo apt-get install wiringpi\n");
            fprintf(stderr, "3. Program jest uruchomiony z uprawnieniami sudo\n");
            return 1;
        }
        printf("Czujnik DHT11 zainicjalizowany pomyślnie na pinie %d\n", DHT11_PIN);
    } else {
        // Tryb nasłuchiwania - wymagane 3 argumenty dodatkowe (razem 4)
        if (argc != 4) {
            fprintf(stderr, "Błąd: Dla trybu nasłuchiwania wymagane są 3 argumenty\n");
            fprintf(stderr, "Użycie: %s 0xFF <ID> <akcja>\n", argv[0]);
            fprintf(stderr, "  akcja: 0 - rozpocznij nasłuch, 1 - zrezygnuj z nasłuchu\n");
            return 1;
        }
        
        is_publisher = false;
        client_id = SPECIAL_LISTEN_ID;
        target_id = (uint8_t)strtol(argv[2], NULL, 0);
        
        // Sprawdzenie akcji
        subscription_action = (uint8_t)strtol(argv[3], NULL, 0);
        if (subscription_action > 1) {
            fprintf(stderr, "Błąd: Nieprawidłowa akcja. Akceptowane wartości: 0 (start) lub 1 (stop)\n");
            return 1;
        }
        
        if (subscription_action == 0) {
            printf("Klient nasłuchujący dla ID: 0x%02X (rozpoczęcie nasłuchu)\n", target_id);
        } else {
            printf("Klient rezygnujący z nasłuchu dla ID: 0x%02X\n", target_id);
        }
    }
    
    // Połączenie z serwerem
    if (Connect() != 0) {
        fprintf(stderr, "Nie udało się połączyć z serwerem\n");
        return 1;
    }
    
    printf("Połączono z serwerem\n");
    
    // Wysłanie żądania subskrypcji (jeśli dotyczy)
    if (!is_publisher) {
        Subscribe(target_id, subscription_action);
        
        // Jeśli rezygnujemy z nasłuchu, to od razu kończymy
        if (subscription_action == 1) {
            printf("Żądanie rezygnacji z nasłuchu wysłane. Zamykanie...\n");
            sleep(1); // Czekaj na potwierdzenie
            Disconnect();
            printf("Klient zakończył działanie\n");
            return 0;
        }
    }
    
    // Główna pętla programu
    if (is_publisher) {
        // Tryb publikowania - odczytuj dane z czujnika i wysyłaj
        printf("Rozpoczynanie odczytu danych z czujnika DHT11...\n");
        printf("Naciśnij Ctrl+C aby zakończyć\n");
        
        static uint32_t last_read_time = 0;
        
        while (is_running) {
            float temperature_c = 0.0f;
            float humidity_percent = 0.0f;
            
            // Sprawdź, czy minęło wystarczająco czasu od ostatniego odczytu DHT11
            uint32_t current_time = time(NULL);
            if (current_time - last_read_time >= DHT11_WAIT_TIME) {
                // Odczyt danych z czujnika DHT11
                if (DHT11_ReadSensor(&temperature_c, &humidity_percent) == 0) {
                    printf("Odczytano: Temp=%.1f°C, Wilgotność=%.1f%%\n", temperature_c, humidity_percent);
                    last_read_time = current_time;
                } else {
                    fprintf(stderr, "Błąd odczytu czujnika DHT11! Używam ostatniej wartości.\n");
                    // W przypadku błędu, używamy temperatury 25.0°C i wilgotności 50%
                    temperature_c = 25.0f;
                    humidity_percent = 50.0f;
                    last_read_time = current_time;
                }
                
                // Przeliczanie jednostek
                // Temperatura: stopnie Celsjusza * 10 (dokładność 0.1°C)
                uint16_t temp_int = (uint16_t)(temperature_c * 10.0f);
                // Wilgotność: procenty * 10 (dokładność 0.1%)
                uint16_t humidity_int = (uint16_t)(humidity_percent * 10.0f);
                
                // Ograniczenie wartości do zakresu (0-1000 = 0-100%)
                if (humidity_int > 1000) humidity_int = 1000;
                
                Publish(temp_int, humidity_int);
                
                printf("Wysłano: ID=0x%02X | Temp=%.1f°C | Humidity=%.1f%%\n",
                       client_id, temperature_c, humidity_percent);
            } else {
                // Czekaj do następnego odczytu
                usleep(100000); // 100ms
                continue;
            }
            
            sleep(1); // Ogólny cykl co sekundę
        }
        
    } else {
        // Tryb nasłuchiwania - odbieraj dane
        printf("Nasłuchiwanie danych dla ID 0x%02X...\n", target_id);
        printf("Naciśnij Ctrl+C aby zakończyć\n");
        while (is_running) {
            HandleIncomingData();
            usleep(100000); // 100ms
        }
    }
    
    // Rozłączenie
    Disconnect();
    printf("Klient zakończył działanie\n");
    return 0;
}


// Handler sygnałów
void SignalHandler(int sig) {
    printf("\nOtrzymano sygnał %d, zamykanie...\n", sig);
    is_running = false;
}
