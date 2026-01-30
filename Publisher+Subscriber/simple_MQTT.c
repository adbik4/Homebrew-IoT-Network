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
#include <string.h>
#include <unistd.h>

extern int udp_socket, tcp_conn_socket;
extern uint8_t client_id, target_id, subscription_action;
extern bool is_running, is_publisher;

// Funkcja Connect dla klienta, wyszukuje serwer przez UDP podając swoje ID, oraz próbuje dokonać połączenia po TCP po znalezieniu
int Connect() {
    struct sockaddr_in multicast_addr;
    struct ip_mreq mreq;
    char buffer[BUFSIZE];
    struct timeval tv;
    
    // Tworzenie gniazda UDP do wysyłania wiadomości na adres multicast i odbioru zwrotki z serwera
    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket < 0) {
        perror("socket UDP");
        return -1;
    }
    
    // Ustawianie wartości timeout dla gniazda na 5 sekund i włączenie ignorowania własnych wiadomości na multicaście
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(udp_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    int loop = 0;
    setsockopt(udp_socket, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
    
    // Dołączenie do grupy multicast zdefiniowanej w pliku nagłówkowym
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
    
    // Wysłanie wiadomości na multicast w celu odnalezienia serwera
    struct sockaddr_in send_addr;
    memset(&send_addr, 0, sizeof(send_addr));
    send_addr.sin_family = AF_INET;
    send_addr.sin_addr.s_addr = inet_addr(MULTICAST_GROUP);
    send_addr.sin_port = htons(DISCOVERY_PORT);
    
    sendto(udp_socket, &client_id, sizeof(client_id), 0,
           (struct sockaddr*)&send_addr, sizeof(send_addr));
    
    printf("Wysłano ID 0x%02X przez multicast\n", client_id);
    
    // Nasłuchiwanie odpowiedzi ze strony zerwera i odczytanie jego danych
    printf("Oczekiwanie na odpowiedź serwera...\n");
    
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);
    int n = recvfrom(udp_socket, buffer, BUFSIZE, 0,
                     (struct sockaddr*)&server_addr, &addr_len);
    
    if (n > 0) {
        printf("Otrzymano odpowiedź od serwera: %s:%d\n",
               inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));
        
        // Utworzenie gniazda TCP do komunikacja z serwerem oraz łączenie się na podstawie otrzymanych danych
        tcp_conn_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (tcp_conn_socket < 0) {
            perror("socket TCP");
            close(udp_socket);
            return -1;
        }
        
        server_addr.sin_port = htons(LISTEN_PORT);
        
        if (connect(tcp_conn_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            perror("connect TCP");
            close(tcp_conn_socket);
            close(udp_socket);
            return -1;
        }

        // Zamknięcie gniazda UDP po dokonaniu połączenia
        close(udp_socket);
        return 0;
    } else {
        printf("Timeout - nie otrzymano odpowiedzi od serwera\n");
        close(udp_socket);
        return -1;
    }
}

// Funkcja Disconnect, sprawdzenie istnienia gniazd i zamknięcie wszystkich
void Disconnect() {
    if (tcp_conn_socket >= 0) {
        if (!is_publisher && subscription_action == 0) {
            printf("Wysyłanie żądania rezygnacji z nasłuchu przed zamknięciem...\n");
            Subscribe(target_id, 1);
        }
        close(tcp_conn_socket);
        tcp_conn_socket = -1;
    }
    
    if (udp_socket >= 0) {
        close(udp_socket);
        udp_socket = -1;
    }
}

// Funkcja Publish, składanie danych z czujnika do wysłania na serwer
void Publish(uint16_t temp, uint16_t humidity) {
    if (tcp_conn_socket < 0) return;
    
    SensorData data;
    data.id = client_id;
    data.temperature = htons(temp);
    data.humidity = htons(humidity);
    
    uint8_t buffer[BUFSIZE];
    buffer[0] = data.id;
    memcpy(&buffer[1], &data.temperature, 2);
    memcpy(&buffer[3], &data.humidity, 2);
    
    send(tcp_conn_socket, buffer, sizeof(buffer), 0);
}

// Funkcja Subscribe, podjęcie roli subscribera i informoanie serwera o potrzebie zaopatryania w dane
void Subscribe(uint8_t target_id, uint8_t action) {
    if (tcp_conn_socket < 0) return;
    
    SubscriptionRequest req;
    req.special_id = SPECIAL_LISTEN_ID;
    req.target_id = target_id;
    req.action = action; // 0=start, 1=stop
    
    uint8_t buffer[BUFSIZE];
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

// Funkcja HandleIncomingData, odbieranie danych z serwera, funkcja dla dla subscriberów
void HandleIncomingData() {
    if (tcp_conn_socket < 0) return;
    
    SensorData data;
    fd_set readfds;
    struct timeval tv;
    
    // Sprawdzanie dostępności danych do odczytu
    FD_ZERO(&readfds);
    FD_SET(tcp_conn_socket, &readfds);
    tv.tv_sec = 0;
    tv.tv_usec = 10000; // 10ms

    // Czekanie i sprawdzanie deskryptorów czy są już gotowe gniazda do odczytu danych
    int activity = select(tcp_conn_socket + 1, &readfds, NULL, NULL, &tv);

    // Jeżeli deksryptor jest gotowy do odczytu i jest to nasze gniazdo TCP, to wykonujemy zaczyt do naszej struktury
    if (activity > 0 && FD_ISSET(tcp_conn_socket, &readfds)) {
        int n = recv(tcp_conn_socket, &data, sizeof(SensorData), 0);
        
        if (n == sizeof(SensorData)) {
            data.temperature = ntohs(data.temperature);
            data.humidity = ntohs(data.humidity);
            
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
