CC      = clang
CFLAGS  = -std=c11 -Wall -Wextra -Wpedantic -g
SRC     = src/main.c src/decoder.c
OUT     = build/8086sim

all:
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)

run: all
	./$(OUT)

clean:
	rm -f $(OUT)
