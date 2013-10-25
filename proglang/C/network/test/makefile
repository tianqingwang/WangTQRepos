
CFLAGS=-g
LIBS=-lpthread

SRC=test_main.c workqueue.c data.c sock.c client.c

TARGET=nettest

CC=gcc

$(TARGET):$(SRC)
	$(CC) $(CFLAGS) $(SRC)  -o $(TARGET) $(LIBS)

clean:
	rm -rf $(TARGET)
