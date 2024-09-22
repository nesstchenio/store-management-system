# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -Wextra -pthread -Iinclude

# Source files
SRCS = client.c serveur.c $(wildcard lib/*.c) $(wildcard module/*.c)

# Object files
OBJS = $(SRCS:.c=.o)

# Executable names
SERVER = serveur
CLIENT = client

# Default target
all: $(SERVER) $(CLIENT)

# Rule to build the server
$(SERVER): $(filter-out client.o, $(OBJS))
	$(CC) $(CFLAGS) -o $@ $^

# Rule to build the client
$(CLIENT): $(filter-out serveur.o, $(OBJS))
	$(CC) $(CFLAGS) -o $@ $^

# Rule to build object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up
clean:
	rm -f $(OBJS) $(SERVER) $(CLIENT)

# Run the server (assumes port number is passed as an argument)
run_server: $(SERVER)
	./$(SERVER) $(PORT)

# Run the client (assumes server address and port number are passed as arguments)
run_client: $(CLIENT)
	./$(CLIENT) localhost $(PORT)

.PHONY: all clean run_server run_client

