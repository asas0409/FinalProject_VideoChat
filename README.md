# FinalProject_VideoChat

## Introduction
This is final project of 2023-1 IoT practice course. In this project, we design our own server-client program that provides video chat function.

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
- Compile (you can also compile both server.cpp and client.cpp using makefile)
  > - server.cpp
  ```bash
  g++ server.cpp -o server -std=c++11
  ```
  > - client.cpp
  ```bash
  g++ client.cpp -o client $(pkg-config --cflags --libs portaudiocpp) $(pkg-config --cflags --libs opencv4) -std=c++11
  ```
- Run
  > - server
  ```bash
  ./server [port]
  ```
  > - client
  ```bash
  ./client [address] [port] [id]
  ```
