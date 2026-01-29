#include "simple_MQTT.h"
#include "data_structs.h"
#include "constants.h"
#include "client_util.h"

#include <stdbool.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include <memory.h>
#include <stdio.h>
#include <unistd.h>

extern int udp_socket, tcp_conn_socket, tcp_listen_socket;
extern uint8_t client_id, target_id, subscription_action;
extern bool is_running, is_publisher;

// Funkcja Connect - odnajduje serwer przez UDP multicast i nawiązuje połączenie TCP
int Connect() {
    struct sockaddr_in multicast_addr;
    struct ip_mreq mreq;
    char buffer[MAXTXSIZE];
    struct timeval tv;
    
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
    int loop = 0;
    setsockopt(udp_socket, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
    
    // Dołącz do grupy multicast
    memset(&multicast_addr, 0, sizeof(multicast_addr));
    multicast_addr.sin_family = AF_INET;
    multicast_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    multicast_addr.sin_port = htons(DISCOVERY_PORT);
    
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
    send_addr.sin_port = htons(DISCOVERY_PORT);
    
    sendto(udp_socket, &client_id, sizeof(client_id), 0,
           (struct sockaddr*)&send_addr, sizeof(send_addr));
    
    printf("Wysłano ID 0x%02X przez multicast\n", client_id);
    
    // 3. Nasłuchuj odpowiedzi od serwera
    printf("Oczekiwanie na odpowiedź serwera...\n");
    
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);
    int n = recvfrom(udp_socket, buffer, MAXTXSIZE, 0,
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
        server_addr.sin_port = htons(LISTEN_PORT);
        
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

// Funkcja Publish - wysyła dane do serwera (ZAMIENIONE: pressure -> humidity)
void Publish(uint16_t temp, uint16_t humidity) {
    if (tcp_conn_socket < 0) return;
    
    SensorData data;
    data.id = client_id;
    data.temperature = htons(temp);
    data.humidity = htons(humidity);  // ZAMIENIONE: pressure -> humidity
    
    // Wysyłaj w odpowiedniej kolejności: ID, temp, humidity
    uint8_t buffer[MAXRXSIZE];
    buffer[0] = data.id;
    memcpy(&buffer[1], &data.temperature, 2);
    memcpy(&buffer[3], &data.humidity, 2);  // ZAMIENIONE: pressure -> humidity
    
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
    uint8_t buffer[MAXRXSIZE];
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
            data.humidity = ntohs(data.humidity);  // ZAMIENIONE: pressure -> humidity
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