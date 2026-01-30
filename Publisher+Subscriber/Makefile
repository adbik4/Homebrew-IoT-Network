# Makefile dla klienta DHT11-MQTT z GPIO5

CC      := gcc
CFLAGS  := -I.. -Wall -O2 -DDEBUG_DHT11 -I.
LDFLAGS := -lwiringPi -lpthread
TARGET  := dht11_client

# Pliki źródłowe
SRCS := \
	main.c \
	dht11.c \
	simple_MQTT.c \
	client_util.c \
	driver_dht11.c \
	driver_dht11_interface_template.c

OBJS := $(SRCS:.c=.o)

# Cel domyślny
all: $(TARGET)

# Linkowanie
$(TARGET): $(OBJS)
	@echo "Łączenie plików obiektowych..."
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)
	@echo "Kompilacja zakończona: $@"

# Reguła automatyczna: .c -> .o
%.o: %.c
	@echo "Kompilowanie $<..."
	$(CC) $(CFLAGS) -c $< -o $@

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
test-compile: clean
	@echo "Testowanie kompilacji..."
	$(CC) $(CFLAGS) -c dht11.c -o dht11_test.o
	$(CC) $(CFLAGS) -c main.c -o main_test.o
	@echo "  dht11.c & main.c: OK"
	rm -f dht11_test.o main_test.o
	@echo "Kompilacja testowa zakończona pomyślnie"

# Test czujnika
test-sensor: clean
	@echo "Kompilacja testu czujnika..."
	$(CC) $(CFLAGS) -o test_dht11 \
		test_dht11.c dht11.c driver_dht11.c driver_dht11_interface_template.c \
		$(LDFLAGS)
	@echo "Uruchamianie testu czujnika..."
	@sudo ./test_dht11

# Uruchomienie w trybach
run-publish:
	@echo "Uruchamianie w trybie publikacji z ID 0x01..."
	@sudo ./$(TARGET) 0x01

run-listen:
	@echo "Uruchamianie w trybie nasłuchu dla ID 0x01..."
	@sudo ./$(TARGET) 0xFF 0x01 0

run-unsubscribe:
	@echo "Uruchamianie rezygnacji z nasłuchu dla ID 0x01..."
	@sudo ./$(TARGET) 0xFF 0x01 1

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

.PHONY: all clean install-deps test-compile test-sensor run-publish run-listen run-unsubscribe help
