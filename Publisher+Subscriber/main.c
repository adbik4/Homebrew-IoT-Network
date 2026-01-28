#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <stdint.h>
#include <stdbool.h>

// Definicje stałych
#define MULTICAST_GROUP "239.0.0.1"
#define MULTICAST_PORT 2137
#define TCP_PORT 2138
#define BUFFER_SIZE 1024
#define SPECIAL_LISTEN_ID 0xFF
#define LISTEN_QUEUE_SIZE 5

// Struktury danych
#pragma pack(push, 1)
typedef struct {
    uint8_t id;
    uint16_t temperature;
    uint16_t pressure;
} SensorData;

typedef struct {
    uint8_t special_id;      // 0xFF dla subskrypcji
    uint8_t target_id;       // ID do nasłuchiwania
    uint8_t action;          // 0=start, 1=stop
} SubscriptionRequest;

typedef struct {
    uint8_t id;
    uint16_t temperature;
    uint16_t pressure;
    time_t timestamp;
} MeasurementData;
#pragma pack(pop)

// Zmienne globalne
int tcp_listen_socket = -1;
int tcp_conn_socket = -1;
int udp_socket = -1;
bool is_running = true;
uint8_t client_id = 0;
uint8_t target_id = 0;
uint8_t subscription_action = 0;
bool is_publisher = false;

// Funkcje prototypy
int Connect();
void Disconnect();
void Publish(uint16_t temp, uint16_t pressure);
void Subscribe(uint8_t target_id, uint8_t action);
void HandleIncomingData();
void FindServer();
void PrintMeasurement(const MeasurementData *data);
void SignalHandler(int sig);
int WaitForServerConnection();

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
        // Tryb publikowania - wysyłaj dane co sekundę
        srand(time(NULL));
        while (is_running) {
            uint16_t temp = rand() % 500 + 2000;  // 20.0-25.0°C (w setnych)
            uint16_t pressure = rand() % 300 + 10000; // 1000-1030 hPa (w dziesiątych)
            
            Publish(temp, pressure);
            printf("Wysłano: ID=0x%02X | temp=%u.%02u°C | pressure=%u.%uhPa\n",
                   client_id, temp/100, temp%100, pressure/10, pressure%10);
            
            sleep(1);
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

// Funkcja Connect - wysyła ID przez multicast i nasłuchuje na połączenie TCP od serwera
int Connect() {
    struct sockaddr_in multicast_addr;
    struct ip_mreq mreq;
    char buffer[BUFFER_SIZE];
    struct timeval tv;
    unsigned char loop;

    // 1. Utwórz socket UDP do multicast
    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket < 0) {
        perror("socket UDP");
        return -1;
    }
    
    // Ustaw timeout na odbiór
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(udp_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    // Ignoruj własne wiadomości
    loop = 0;
    setsockopt(udp_socket, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));

    // Dołącz do grupy multicast
    memset(&multicast_addr, 0, sizeof(multicast_addr));
    multicast_addr.sin_family = AF_INET;
    multicast_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    multicast_addr.sin_port = htons(MULTICAST_PORT);
    
    if (bind(udp_socket, (struct sockaddr*)&multicast_addr, sizeof(multicast_addr)) < 0) {
        perror("bind UDP");
        close(udp_socket);
        return -1;
    }
    
    mreq.imr_multiaddr.s_addr = inet_addr(MULTICAST_GROUP);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    
    if (setsockopt(udp_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        perror("multicast join");
        close(udp_socket);
        return -1;
    }
    
    // 2. Wyślij swoje ID przez multicast
    struct sockaddr_in send_addr;
    memset(&send_addr, 0, sizeof(send_addr));
    send_addr.sin_family = AF_INET;
    send_addr.sin_addr.s_addr = inet_addr(MULTICAST_GROUP);
    send_addr.sin_port = htons(MULTICAST_PORT);
    
    sendto(udp_socket, &client_id, sizeof(client_id), 0,
           (struct sockaddr*)&send_addr, sizeof(send_addr));
    
    printf("Wysłano ID 0x%02X przez multicast\n", client_id);
    
    // 3. Nasłuchuj odpowiedzi od serwera
    printf("Oczekiwanie na odpowiedź serwera...\n");
    
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);
    int n = recvfrom(udp_socket, buffer, BUFFER_SIZE, 0,
                     (struct sockaddr*)&server_addr, &addr_len);
    
    if (n > 0) {
        printf("Otrzymano odpowiedź od serwera: %s:%d\n",
               inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));
        
        // 4. Utwórz połączenie TCP z serwerem
        tcp_conn_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (tcp_conn_socket < 0) {
            perror("socket TCP");
            close(udp_socket);
            return -1;
        }
        
        // Połącz z serwerem (używamy innego portu dla TCP)
        server_addr.sin_port = htons(TCP_PORT);
        
        if (connect(tcp_conn_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            perror("connect TCP");
            close(tcp_conn_socket);
            close(udp_socket);
            return -1;
        }
        
        close(udp_socket); // UDP już niepotrzebne
        return 0;
    } else {
        printf("Timeout - nie otrzymano odpowiedzi od serwera\n");
        close(udp_socket);
        return -1;
    }
}


// Funkcja Disconnect - zamyka połączenia
void Disconnect() {
    if (tcp_conn_socket >= 0) {
        // Jeśli jesteśmy subskrybentem i nasłuchujemy (nie rezygnujemy), wyślij żądanie anulowania
        if (!is_publisher && subscription_action == 0) {
            printf("Wysyłanie żądania rezygnacji z nasłuchu przed zamknięciem...\n");
            Subscribe(target_id, 1); // 1 = stop
            sleep(1); // Czekaj na wysłanie
        }
        close(tcp_conn_socket);
        tcp_conn_socket = -1;
    }
    
    if (tcp_listen_socket >= 0) {
        close(tcp_listen_socket);
        tcp_listen_socket = -1;
    }
    
    if (udp_socket >= 0) {
        close(udp_socket);
        udp_socket = -1;
    }
}

// Funkcja Publish - wysyła dane do serwera
void Publish(uint16_t temp, uint16_t pressure) {
    if (tcp_conn_socket < 0) return;
    
    SensorData data;
    data.id = client_id;
    data.temperature = htons(temp);
    data.pressure = htons(pressure);
    
    // Wysyłaj w odpowiedniej kolejności: ID, temp, pressure
    uint8_t buffer[5];
    buffer[0] = data.id;
    memcpy(&buffer[1], &(data.temperature), sizeof(data.temperature));
    memcpy(&buffer[3], &(data.pressure), sizeof(data.pressure));
    
    send(tcp_conn_socket, buffer, sizeof(buffer), 0);
}

// Funkcja Subscribe - wysyła żądanie subskrypcji/anulowania
void Subscribe(uint8_t target_id, uint8_t action) {
    if (tcp_conn_socket < 0) return;
    
    SubscriptionRequest req;
    req.special_id = SPECIAL_LISTEN_ID;
    req.target_id = target_id;
    req.action = action; // 0=start, 1=stop
    
    // Wysyłaj w odpowiedniej kolejności
    uint8_t buffer[3];
    buffer[0] = req.special_id;
    buffer[1] = req.target_id;
    buffer[2] = req.action;
    
    int bytes_sent = send(tcp_conn_socket, buffer, sizeof(buffer), 0);
    
    if (bytes_sent > 0) {
        if (action == 0) {
            printf("Żądanie rozpoczęcia nasłuchu dla ID: 0x%02X wysłane\n", target_id);
        } else {
            printf("Żądanie rezygnacji z nasłuchu dla ID: 0x%02X wysłane\n", target_id);
        }
    } else {
        perror("Błąd wysyłania żądania subskrypcji");
    }
}

// Funkcja HandleIncomingData - odbiera i przetwarza dane z serwera
void HandleIncomingData() {
    if (tcp_conn_socket < 0) return;
    
    MeasurementData data;
    fd_set readfds;
    struct timeval tv;
    
    // Sprawdź czy są dane do odczytu
    FD_ZERO(&readfds);
    FD_SET(tcp_conn_socket, &readfds);
    tv.tv_sec = 0;
    tv.tv_usec = 10000; // 10ms
    
    int activity = select(tcp_conn_socket + 1, &readfds, NULL, NULL, &tv);
    
    if (activity > 0 && FD_ISSET(tcp_conn_socket, &readfds)) {
        int n = recv(tcp_conn_socket, &data, sizeof(MeasurementData), 0);
        
        if (n == sizeof(MeasurementData)) {
            // Konwersja z sieciowej kolejności bajtów
            data.temperature = ntohs(data.temperature);
            data.pressure = ntohs(data.pressure);
            data.timestamp = ntohl(data.timestamp);
            
            PrintMeasurement(&data);
        } else if (n == 0) {
            printf("Serwer zamknął połączenie\n");
            is_running = false;
        } else if (n < 0) {
            perror("recv");
        } else {
            printf("Otrzymano niepełne dane (%d bajtów)\n", n);
        }
    }
}

// Funkcja PrintMeasurement - wyświetla odebrane dane pomiarowe
void PrintMeasurement(const MeasurementData *data) {
    char time_buf[64];
    struct tm *timeinfo;
    
    timeinfo = localtime(&data->timestamp);
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    printf("Odebrano pomiar:\n");
    printf("  ID źródła: 0x%02X\n", data->id);
    printf("  Temperatura: %u.%02u°C\n", data->temperature/100, data->temperature%100);
    printf("  Ciśnienie: %u.%uhPa\n", data->pressure/10, data->pressure%10);
    printf("  Czas pomiaru: %s\n", time_buf);
    printf("----------------------------------------\n");
}

// Handler sygnałów
void SignalHandler(int sig) {
    printf("\nOtrzymano sygnał %d, zamykanie...\n", sig);
    is_running = false;
}