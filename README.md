# Low-Latency Peer-to-Peer Voice Chat

I needed a real time audio chat for a project I was working on, so I decided to make one myself. This is a simple C++ implementation of a low-latency local network voice chat system using PortAudio, Opus, and ENet libraries. It is made for peer-to-peer communication with minimal (I hope) latency.

## Prerequisites

- C++17 compiler
- Make
- PortAudio
- Opus codec library
- ENet networking library

### Installation Dependencies

#### macOS (using Homebrew)
```bash
brew install portaudio opus enet
```

## Building the Project
```
git clone https://github.com/ismaeelbashir03/p2p-voice-chat.git
cd p2p-voice-chat
make
```

## Usage
Running the server:
```
./bin/voice_chat server <PORT>
```

Running the client:
```
./bin/voice_chat client <SERVER_IP> <PORT>
```