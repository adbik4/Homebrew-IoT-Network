CC = gcc
CFLAGS = -I. -I..
ELF_NAME = broker-daemon

SRCS = main.c dht11.c simple_MQTT.c ../utilities.c

all: $(ELF_NAME)

$(ELF_NAME): $(SRCS)
	mkdir -p build
	$(CC) $(CFLAGS) $(SRCS) -o build/$(ELF_NAME)

clean:
	rm -f build/$(ELF_NAME)

.PHONY: all clean
