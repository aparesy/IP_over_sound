# tun_to_bits : module en 4 étapes (créer TUN, lire paquet, encapsuler en trame,
# puis convertir trame -> bits) + enchaînement via le programme principal.
# Dépend de include/ et de src/tun_dev.c, src/protocol.c, src/utils.c du répertoire parent.

CC = gcc
CFLAGS = -Wall -Wextra -I.. -I../include
LDFLAGS =

BIN = tun_to_bits
OBJS = tun_to_bits.o tun_create.o packet_read.o encapsulate.o frame_to_bits.o \
       tun_dev.o protocol.o utils.o

all: $(BIN)

tun_to_bits.o: tun_to_bits.c tun_create.h packet_read.h encapsulate.h frame_to_bits.h ../include/common.h ../include/tun_dev.h
	$(CC) $(CFLAGS) -c -o $@ tun_to_bits.c

tun_create.o: tun_create.c tun_create.h ../include/tun_dev.h
	$(CC) $(CFLAGS) -c -o $@ tun_create.c

packet_read.o: packet_read.c packet_read.h ../include/tun_dev.h
	$(CC) $(CFLAGS) -c -o $@ packet_read.c

encapsulate.o: encapsulate.c encapsulate.h ../include/protocol.h
	$(CC) $(CFLAGS) -c -o $@ encapsulate.c

frame_to_bits.o: frame_to_bits.c frame_to_bits.h
	$(CC) $(CFLAGS) -c -o $@ frame_to_bits.c

tun_dev.o: ../src/tun_dev.c ../include/tun_dev.h ../include/common.h
	$(CC) $(CFLAGS) -c -o $@ ../src/tun_dev.c

protocol.o: ../src/protocol.c ../include/protocol.h ../include/common.h ../include/utils.h
	$(CC) $(CFLAGS) -c -o $@ ../src/protocol.c

utils.o: ../src/utils.c ../include/utils.h
	$(CC) $(CFLAGS) -c -o $@ ../src/utils.c

$(BIN): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

clean:
	rm -f $(OBJS) $(BIN)

.PHONY: all clean test

test: $(BIN)
	./$(BIN) --test
	@echo "--- Check output/frame.bin and output/bits.bin ---"
