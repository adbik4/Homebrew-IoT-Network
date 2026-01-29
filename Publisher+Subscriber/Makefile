CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lwiringPi

# Pliki źródłowe
SRCS = main.c dht11.c simple_MQTT.c client_util.c \
       driver_dht11.c driver_dht11_interface_template.c
OBJS = $(SRCS:.c=.o)

TARGET = client

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

# Kompilacja z wartościami testowymi (bez DHT11)
test:
	$(CC) $(CFLAGS) -DTEST_VALUES -c dht11.c -o dht11.o
	$(CC) $(CFLAGS) -o $(TARGET) main.c dht11.o simple_MQTT.c client_util.c $(LDFLAGS)
