CC = g++
CFLAGS = -g -Wall

OPENCV = `pkg-config opencv4 —cflags —libs`
PORTAUDIO = `pkg-config portaudiocpp —cflags —libs`
LIBS = $(OPENCV) $(PORTAUDIO)

all: 
	$(CC) $(CFLAGS) -o server server.cpp -std=c++11
	$(CC) $(CFLAGS) -o client client.cpp $(LIBS) -std=c++11

clean:
	rm -f ./server ./client
