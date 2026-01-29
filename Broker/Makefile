CC = gcc
CFLAGS = -I. -I..
ELF_NAME = broker-daemon

SRCS = main.c setup.c database.c ../utilities.c
OBJS = $(SRCS:.c=.o)

all: $(ELF_NAME)

$(ELF_NAME): $(OBJS)
	mkdir -p build
	$(CC) $(OBJS) -o build/$(ELF_NAME)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) build/$(ELF_NAME)