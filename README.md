# Low-Latency Peer-to-Peer Voice Chat

A C++ implementation of a low-latency local network voice chat system using PortAudio, Opus, and ENet libraries. Made for direct peer-to-peer communication with minimal (I hope) latency.

## Prerequisites

- C++17 compiler
- Make
- PortAudio
- Opus codec library
- ENet networking library

### Installation Dependencies

#### Ubuntu/Debian
```bash
sudo apt-get install build-essential libportaudio2 libopus-dev libenet-dev
```

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