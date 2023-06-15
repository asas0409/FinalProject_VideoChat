# FinalProject_VideoChat
## Requirements
- c++ (11)
- opencv4
- portaudio

## How to run (on Mac)
- Install Libraries
  > - opencv
  > > brew install opencv 
  > - portaudio
  > > brew install portaudio
- Compile
  > - server.cpp
  ```bash
  g++ server.cpp -o server -std=c++11
  ```
  > - client.cpp
  ```bash
  g++ client.cpp -o client '$'(pkg-config --cflags --libs portaudiocpp) \$(pkg-config --cflags --libs opencv4) -std=c++11
  ```
- Run
  > - server
  > > ./server [port]
  > - client
  > > ./client [address] [port] [id] 
