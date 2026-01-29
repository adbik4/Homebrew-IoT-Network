# Makefile dla klienta DHT11-MQTT z GPIO5
CC = gcc
CFLAGS = -I.. -Wall -O2 -DDEBUG_DHT11 -I.
LDFLAGS = -lwiringPi -lpthread
TARGET = dht11_client

# Pliki źródłowe - ZAKTUALIZOWANE NAZWY PLIKÓW
SRCS = main.c dht11.c simple_MQTT.c client_util.c \
       driver_dht11.c driver_dht11_interface_template.c

# Generuj nazwy plików obiektowych
OBJS = $(SRCS:.c=.o)

# Cel domyślny
all: $(TARGET)

# Kompilacja główna
$(TARGET): $(OBJS)
	@echo "Łączenie plików obiektowych..."
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)
	@echo "Kompilacja zakończona: $(TARGET)"

# Reguła kompilacji dla każdego pliku .c
.c.o:
	@echo "Kompilowanie $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Alternatywnie, możesz użyć dokładniejszych reguł dla każdego pliku:

# main.o: main.c dht11.h constants.h data_structs.h simple_MQTT.h
# 	$(CC) $(CFLAGS) -c main.c -o main.o

# dht11.o: dht11.c dht11.h driver_dht11.h driver_dht11_interface.h
# 	$(CC) $(CFLAGS) -c dht11.c -o dht11.o

# driver_dht11.o: driver_dht11.c driver_dht11.h
# 	$(CC) $(CFLAGS) -c driver_dht11.c -o driver_dht11.o

# driver_dht11_interface_template.o: driver_dht11_interface_template.c driver_dht11_interface.h dht11.h
# 	$(CC) $(CFLAGS) -c driver_dht11_interface_template.c -o driver_dht11_interface_template.o

# simple_MQTT.o: simple_MQTT.c simple_MQTT.h constants.h data_structs.h client_util.h
# 	$(CC) $(CFLAGS) -c simple_MQTT.c -o simple_MQTT.o

# client_util.o: client_util.c client_util.h data_structs.h
# 	$(CC) $(CFLAGS) -c client_util.c -o client_util.o

# Czyszczenie
clean:
	@echo "Czyszczenie plików..."
	rm -f $(OBJS) $(TARGET) test_dht11
	@echo "Czyszczenie zakończone"

# Instalacja zależności
install-deps:
	@echo "Instalowanie zależności..."
	sudo apt-get update
	sudo apt-get install wiringpi
	@echo "Zależności zainstalowane pomyślnie"

# Test kompilacji
test-compile:
	@echo "Testowanie kompilacji..."
	$(CC) $(CFLAGS) -c dht11.c -o dht11_test.o
	@echo "  dht11.c: OK"
	$(CC) $(CFLAGS) -c main.c -o main_test.o
	@echo "  main.c: OK"
	rm -f dht11_test.o main_test.o
	@echo "Kompilacja testowa zakończona pomyślnie"

# Test czujnika
test-sensor:
	@echo "Kompilacja testu czujnika..."
	$(CC) $(CFLAGS) -o test_dht11 test_dht11.c dht11.c driver_dht11.c driver_dht11_interface_template.c -lwiringPi
	@echo "Uruchamianie testu czujnika..."
	sudo ./test_dht11

# Uruchomienie w trybie publikacji
run-publish:
	@echo "Uruchamianie w trybie publikacji z ID 0x01..."
	sudo ./$(TARGET) 0x01

# Uruchomienie w trybie nasłuchu
run-listen:
	@echo "Uruchamianie w trybie nasłuchu dla ID 0x01..."
	sudo ./$(TARGET) 0xFF 0x01 0

# Uruchomienie w trybie rezygnacji z nasłuchu
run-unsubscribe:
	@echo "Uruchamianie rezygnacji z nasłuchu dla ID 0x01..."
	sudo ./$(TARGET) 0xFF 0x01 1

# Pomoc
help:
	@echo "Dostępne komendy:"
	@echo "  make all               - kompiluje cały projekt"
	@echo "  make clean             - czyści pliki kompilacji"
	@echo "  make install-deps      - instaluje zależności (wiringPi)"
	@echo "  make test-compile      - testuje kompilację"
	@echo "  make test-sensor       - testuje czujnik DHT11"
	@echo "  make run-publish       - uruchamia w trybie publikacji"
	@echo "  make run-listen        - uruchamia w trybie nasłuchu"
	@echo "  make run-unsubscribe   - uruchamia rezygnację z nasłuchu"
	@echo ""
	@echo "Przykłady ręcznego uruchomienia:"
	@echo "  sudo ./dht11_client 0x01           # Publikuj z ID 0x01"
	@echo "  sudo ./dht11_client 0xFF 0x01 0    # Nasłuchuj ID 0x01"
	@echo "  sudo ./dht11_client 0xFF 0x01 1    # Zrezygnuj z nasłuchu ID 0x01"

.PHONY: all clean install-deps test-compile test-sensor run-publish run-listen run-unsubscribe help
