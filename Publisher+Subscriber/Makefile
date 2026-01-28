CC = gcc
CFLAGS = -I.
ELF_NAME = client
SRCS = main.c

all: $(ELF_NAME)

$(ELF_NAME): $(SRCS)
	mkdir -p build
	$(CC) $(CFLAGS) $(SRCS) -o build/$(ELF_NAME)

clean:
	rm -f build/$(ELF_NAME)
