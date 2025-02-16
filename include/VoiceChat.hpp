#pragma once
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <portaudio.h>
#include <opus/opus.h>
#include <enet/enet.h>

/**
 * Wrapper class for voice chat using Opus (audio codec) and ENet (networking/streaming).
*/
class VoiceChat {
public:
    /**
     * Create a new voice chat instance.
     * @param remoteIp The IP address of the remote host.
     * @param port The port to connect to.
     * @param isServer Whether this instance should act as a server.
     * @param debug Whether to print debug messages (stupid bug).
    */
    VoiceChat(const char* remoteIp, int port, bool isServer, bool debug = false);
    ~VoiceChat();

    /**
     * Start the voice chat.
    */
    void Start();
    
    /**
     * Stop the voice chat.
    */
    void Stop();

    /**
     * Set the volume of the audio stream (for proximity chat).
     * @param distance 0.0 = closest (loudest), 1.0 = farthest (quietest)
    */
    void SetVolume(float volume);

private:
    void InitializeAudio();
    void InitializeNetwork();
    void NetworkThread();
    static int AudioInputCallback(const void* inputBuffer, void* outputBuffer,
                                  unsigned long framesPerBuffer,
                                  const PaStreamCallbackTimeInfo* timeInfo,
                                  PaStreamCallbackFlags statusFlags, void* userData);
    static int AudioOutputCallback(const void* inputBuffer, void* outputBuffer,
                                   unsigned long framesPerBuffer,
                                   const PaStreamCallbackTimeInfo* timeInfo,
                                   PaStreamCallbackFlags statusFlags, void* userData);

    ENetHost* enetHost;
    ENetPeer* enetPeer;
    PaStream* inputStream;
    PaStream* outputStream;
    
    OpusEncoder* opusEncoder;
    OpusDecoder* opusDecoder;
    
    std::thread networkThread;
    bool running;
    
    std::mutex audioMutex;
    std::queue<std::vector<float>> audioBuffer;

    // volume control
    std::mutex volumeMutex;
    float volume = 1.0f;
    float minVolume = 0.1f;
    float maxDistance = 1.0f;

    bool debug;
    
    /**
     * AUDIO CONFIGURATION

        sampleRate:
            - The audio sampling rate (48000 Hz) affecting audio quality and processing speed during streaming.
    
        channels: 
            - Number of audio channels (1 for mono) used, impacting the spatial quality of the audio stream.
        
        frameSize: 
            - Number of audio frames per buffer (e.g., 960 for 20ms frames), which influences latency and streaming smoothness.
    
        bitrate: 
            - The audio bitrate (e.g., 64000 bps) that balances between compressed audio quality and the network bandwidth required.
    */
    const int sampleRate = 48000;
    const int channels = 1;
    const int frameSize = 960; //= 20ms frames
    const int bitrate = 64000;
};