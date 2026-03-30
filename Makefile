# IP over Sound - Compilation automatisée
# Dépendances : libportaudio-dev (Ubuntu : sudo apt install libportaudio2 libportaudiocpp0 libportaudio-dev)
# Si déjà installé, pkg-config peut fournir les options de compilation/édition de liens PortAudio

CC     = gcc
CFLAGS = -Wall -Wextra -g -I include $(shell pkg-config --cflags portaudio-2.0 2>/dev/null || echo "")
LDFLAGS = -lpthread -lm $(shell pkg-config --libs portaudio-2.0 2>/dev/null || echo "-lportaudio")

SRC = src/main.c src/tun_dev.c src/audio_dev.c src/modem.c src/protocol.c src/utils.c
OBJ = $(SRC:.c=.o)
TARGET = ipo_sound

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJ) $(TARGET)
