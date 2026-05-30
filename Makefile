CC     = gcc
CFLAGS = -Wall -Wextra -g
LIBS   = -lpthread -lrt

SRCS = main.c data.c simulation.c ipc.c socket_server.c
OBJS = $(SRCS:.c=.o)
TARGET = tumor_sim

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
