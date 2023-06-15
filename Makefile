CC = g++
CFLAGS = -g -Wall

OPENCV = $$(pkg-config --cflags --libs opencv4)
PORTAUDIO = $$(pkg-config --cflags --libs portaudiocpp)
LIBS = $(OPENCV) $(PORTAUDIO)

all: 
	$(CC) $(CFLAGS) -o server server.cpp -std=c++11
	$(CC) $(CFLAGS) -o client client.cpp $(LIBS) -std=c++11

clean:
	rm -f ./server ./client
