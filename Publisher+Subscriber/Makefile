CC = gcc
CFLAGS = -I. -I..
ELF_NAME = client

SRCS = main.c dht11.c simple_MQTT.c ../utilities.c
OBJS = $(SRCS:.c=.o)

all: $(ELF_NAME)

$(ELF_NAME): $(OBJS)
	mkdir -p build
	$(CC) $(OBJS) -o build/$(ELF_NAME) -l wiringPi

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) build/$(ELF_NAME)