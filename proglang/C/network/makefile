export LD_LIBRARY_PATH=/usr/local/lib

SRC=libevent_main.c libevent_event.c libevent_socket.c workqueue.c log.c

CFLAGS=-g
LDFLAGS=-L/usr/local/lib
LIBS = -lpthread
LIBS+= -levent

TARGET=eventserver

CC=gcc

$(TARGET):$(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS) $(LIBS)
clean:
	rm $(TARGET)
