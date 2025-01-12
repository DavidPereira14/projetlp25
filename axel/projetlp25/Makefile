# Définition du compilateur et des options de compilation
CC = gcc
CFLAGS = -Wall -Wextra -I./src -I/usr/include/openssl -Wno-deprecated-declarations -Wunused-but-set-variable -Wformat-truncation
LDFLAGS = -lssl -lcrypto

# Définition des fichiers source, objets et cible
SRC = src/main.c src/file_handler.c src/deduplication.c src/backup_manager.c
OBJ = $(SRC:.c=.o)
TARGET = lp25_borgbackup

# Règle par défaut : construire le programme cible
all: $(TARGET)

# Règle pour lier les objets en un exécutable
$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

# Règle pour compiler les fichiers .c en fichiers .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Règle pour nettoyer les fichiers générés
clean:
	rm -f $(OBJ) $(TARGET) src/*.o
