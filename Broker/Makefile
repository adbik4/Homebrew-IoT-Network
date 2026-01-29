CC = gcc
CFLAGS = -I.
ELF_NAME = broker-daemon
SRCS = main.c setup.c utilities.c database.c constants.h data_structs.h wiringPi.h
all: $(ELF_NAME)

$(ELF_NAME): $(SRCS)
	mkdir -p build
	$(CC) $(CFLAGS) $(SRCS) -o build/$(ELF_NAME)

clean:
	rm -f build/$(ELF_NAME)
