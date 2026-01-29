CC = gcc
CFLAGS = -I.
ELF_NAME = client

SRCS = main.c dht11.c client_util.c simple_MQTT.c

all: $(ELF_NAME)

$(ELF_NAME): $(SRCS)
	mkdir -p build
	$(CC) $(CFLAGS) $(SRCS) -o build/$(ELF_NAME) -l wiringPi

clean:
	rm -f build/$(ELF_NAME)

.PHONY: all clean
