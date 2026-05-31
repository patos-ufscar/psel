# MAKEFILE - FIREWALL

CC = gcc
CFLAGS = -Wall -Wextra -g -O2 -Iincludes
TARGET = fwall
SRCS = src/main.c src/checksum.c src/rules.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	@echo "[+] Linkando o executável: $(TARGET)"
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)
	@echo "[✓] Compilação concluída com sucesso!"

%.o: %.c
	@echo "[-] Compilando: $<"
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo "[!] Limpando arquivos de compilação..."
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean