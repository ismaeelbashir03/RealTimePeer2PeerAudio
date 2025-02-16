#include "VoiceChat.hpp"
#include <iostream>


/*
 -------------------
 | DEBUG IP ADDRESS |
 -------------------
 */
#include <iostream>
#ifdef _WIN32
    #include <winsock2.h>
    #include <iphlpapi.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "Ws2_32.lib")
    #pragma comment(lib, "Iphlpapi.lib")
#else
    #include <sys/types.h>
    #include <ifaddrs.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <netdb.h>
#endif

void PrintLocalIPs() {
#ifdef _WIN32
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return;
    }

    // Retrieve adapter addresses
    ULONG flags = GAA_FLAG_INCLUDE_PREFIX;
    ULONG family = AF_INET;
    ULONG bufferSize = 15000;
    PIP_ADAPTER_ADDRESSES adapterAddresses = (IP_ADAPTER_ADDRESSES*)malloc(bufferSize);
    if (adapterAddresses == nullptr) {
        std::cerr << "Memory allocation failed\n";
        WSACleanup();
        return;
    }

    DWORD dwRetVal = GetAdaptersAddresses(family, flags, NULL, adapterAddresses, &bufferSize);
    if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
        free(adapterAddresses);
        adapterAddresses = (IP_ADAPTER_ADDRESSES*)malloc(bufferSize);
        dwRetVal = GetAdaptersAddresses(family, flags, NULL, adapterAddresses, &bufferSize);
    }

    if (dwRetVal == NO_ERROR) {
        for (PIP_ADAPTER_ADDRESSES adapter = adapterAddresses; adapter != nullptr; adapter = adapter->Next) {
            for (PIP_ADAPTER_UNICAST_ADDRESS ua = adapter->FirstUnicastAddress; ua != nullptr; ua = ua->Next) {
                if (ua->Address.lpSockaddr->sa_family == AF_INET) {
                    sockaddr_in* sa_in = reinterpret_cast<sockaddr_in*>(ua->Address.lpSockaddr);
                    char ipAddress[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &(sa_in->sin_addr), ipAddress, INET_ADDRSTRLEN);
                    std::cout << "Interface: " << adapter->FriendlyName 
                              << " - IP: " << ipAddress << "\n";
                }
            }
        }
    } else {
        std::cerr << "GetAdaptersAddresses() failed with error: " << dwRetVal << "\n";
    }
    
    free(adapterAddresses);
    WSACleanup();
#else
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return;
    }
    
    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr) continue;
        if (ifa->ifa_addr->sa_family == AF_INET) { // IPv4
            char addrBuffer[INET_ADDRSTRLEN];
            void* addrPtr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            inet_ntop(AF_INET, addrPtr, addrBuffer, INET_ADDRSTRLEN);
            std::cout << "Interface: " << ifa->ifa_name << " - IP: " << addrBuffer << "\n";
        }
    }
    freeifaddrs(ifaddr);
#endif
}

/*
 -------------------
 | DE/CONSTRUCTORS |
 -------------------
 */
 VoiceChat::VoiceChat(const char* remoteIp, int port, bool isServer, bool debug) 
 : enetHost(nullptr), enetPeer(nullptr), isServer(isServer), running(false), debug(debug) {
    InitializeNetwork();
    
    if (isServer) {
        ENetAddress address;
        address.host = ENET_HOST_ANY;
        address.port = port;
        enetHost = enet_host_create(&address, 2, 2, 0, 0);
        if (!enetHost) throw std::runtime_error("ENet host creation failed");
        PrintLocalIPs();
    } else {
        enetHost = enet_host_create(nullptr, 1, 2, 0, 0);
        if (!enetHost) throw std::runtime_error("ENet host creation failed");
        ENetAddress address;
        enet_address_set_host(&address, remoteIp);
        address.port = port;
        enetPeer = enet_host_connect(enetHost, &address, 2, 0);
        if (!enetPeer) throw std::runtime_error("ENet peer connection failed");
    }

    // Opus initialization remains the same
    int error;
    opusEncoder = opus_encoder_create(sampleRate, channels, OPUS_APPLICATION_VOIP, &error);
    opus_encoder_ctl(opusEncoder, OPUS_SET_BITRATE(bitrate));
    opusDecoder = opus_decoder_create(sampleRate, channels, &error);
}

VoiceChat::~VoiceChat() {
    // stop everything and kill it
    Stop();
    opus_encoder_destroy(opusEncoder);
    opus_decoder_destroy(opusDecoder);
    if (enetHost) enet_host_destroy(enetHost);
}



/*
 ----------------
 | INITIALIZERS |
 ----------------
 */
void VoiceChat::InitializeNetwork() {
    if (enet_initialize() != 0) {
        throw std::runtime_error("ENet initialization failed");
    }
}

void VoiceChat::InitializeAudio() {
    PaError err = Pa_Initialize();
    if (err != paNoError) throw std::runtime_error("PortAudio initialization failed");

    if (debug) {
        int numDevices = Pa_GetDeviceCount();
        for (int i = 0; i < numDevices; i++) {
            const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
            std::cout << "Device #" << i << ": " << info->name 
                    << ", Input channels: " << info->maxInputChannels
                    << ", Output channels: " << info->maxOutputChannels << "\n";
        }
    }

    // below is from google, I don't know what it does exactly but it works
    PaStreamParameters inputParams, outputParams;
    
    inputParams.device = Pa_GetDefaultInputDevice();
    inputParams.channelCount = channels;
    inputParams.sampleFormat = paFloat32;
    inputParams.suggestedLatency = Pa_GetDeviceInfo(inputParams.device)->defaultLowInputLatency;
    inputParams.hostApiSpecificStreamInfo = nullptr;

    outputParams.device = Pa_GetDefaultOutputDevice();
    outputParams.channelCount = channels;
    outputParams.sampleFormat = paFloat32;
    outputParams.suggestedLatency = Pa_GetDeviceInfo(outputParams.device)->defaultLowOutputLatency;
    outputParams.hostApiSpecificStreamInfo = nullptr;

    err = Pa_OpenStream(&inputStream, &inputParams, nullptr, sampleRate,
                       frameSize, paClipOff, AudioInputCallback, this);
    if (err != paNoError) throw std::runtime_error("Input stream open failed");

    err = Pa_OpenStream(&outputStream, nullptr, &outputParams, sampleRate,
                       frameSize, paClipOff, AudioOutputCallback, this);
    if (err != paNoError) throw std::runtime_error("Output stream open failed");
}



/*
 ----------------
 | MAIN METHODS |
 ----------------
 */
void VoiceChat::Start() {
    InitializeAudio();
    running = true;
    networkThread = std::thread(&VoiceChat::NetworkThread, this); // continously receive packets
    Pa_StartStream(inputStream);
    Pa_StartStream(outputStream);
}

void VoiceChat::Stop() {
    running = false;
    if (networkThread.joinable()) networkThread.join(); // join network thread
    Pa_StopStream(inputStream);
    Pa_CloseStream(inputStream);
    Pa_StopStream(outputStream);
    Pa_CloseStream(outputStream);
    Pa_Terminate();
}

void VoiceChat::SetVolume(float distance) {
    distance = std::clamp(distance, 0.0f, maxDistance);
    
    // exp fall off: x^2 curve (to make 'walking away' sound more natural)
    float normalized = distance / maxDistance;
    float vol = 1.0f - (normalized * normalized);
    
    vol = std::clamp(vol, minVolume, 1.0f);
    
    std::lock_guard<std::mutex> lock(volumeMutex);
    volume = vol;
    
    if (debug) std::cout << "Volume: " << vol << "\n";
}



/*
 --------------------
 | PACKET INGESTION |
 --------------------
 */
 void VoiceChat::NetworkThread() {
    while (running) {
        ENetEvent event;
        while (enet_host_service(enetHost, &event, 10) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_CONNECT:
                    std::cout << "Connected to peer\n";
                    if (isServer) {
                        enetPeer = event.peer; // Store the connected peer for the server
                        std::cout << "Server registered peer\n";
                    }
                    break;
                
                case ENET_EVENT_TYPE_RECEIVE: {
                    std::vector<float> pcm(frameSize);
                    int decoded = opus_decode_float(opusDecoder, 
                        event.packet->data, event.packet->dataLength,
                        pcm.data(), frameSize, 0);
                    
                    if (decoded > 0) {
                        std::lock_guard<std::mutex> lock(audioMutex);
                        audioBuffer.push(std::move(pcm));
                    }
                    enet_packet_destroy(event.packet);
                    break;
                }
                
                default:
                    break;
            }
        }
    }
}



/*
 -------------
 | CALLBACKS |
 -------------
 */
int VoiceChat::AudioInputCallback(const void* inputBuffer, void* outputBuffer,
                                 unsigned long framesPerBuffer,
                                 const PaStreamCallbackTimeInfo* timeInfo,
                                 PaStreamCallbackFlags statusFlags, void* userData) {
    
    // cast user data to our voice chat instance
    VoiceChat* instance = static_cast<VoiceChat*>(userData);
    
    // encode the input using opus and send it to the peer
    std::vector<unsigned char> encodedData(4000);
    int encodedBytes = opus_encode_float(instance->opusEncoder,
        static_cast<const float*>(inputBuffer), framesPerBuffer,
        encodedData.data(), encodedData.size());
    
    if (encodedBytes > 0 && instance->enetPeer) {
        ENetPacket* packet = enet_packet_create(encodedData.data(), encodedBytes, 
            ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
        enet_peer_send(instance->enetPeer, 0, packet);
    }
    
    return paContinue; // continue the stream
}

int VoiceChat::AudioOutputCallback(const void* inputBuffer, void* outputBuffer,
                                  unsigned long framesPerBuffer,
                                  const PaStreamCallbackTimeInfo* timeInfo,
                                  PaStreamCallbackFlags statusFlags, void* userData) {
    
    // cast user data to our voice chat instance
    VoiceChat* instance = static_cast<VoiceChat*>(userData);
    float* out = static_cast<float*>(outputBuffer);
    
    // get the next audio packet from the queue and copy it to the output buffer
    std::lock_guard<std::mutex> lock(instance->audioMutex);
    if (!instance->audioBuffer.empty()) {
        const std::vector<float>& pcm = instance->audioBuffer.front();
        
        float currentVolume; {
            std::lock_guard<std::mutex> lock(instance->volumeMutex);
            currentVolume = instance->volume;
        }
        
        // apply volume to audio stream
        for (size_t i = 0; i < pcm.size(); ++i) {
            out[i] = pcm[i] * currentVolume;
        }
        
        instance->audioBuffer.pop();
    } else {
        std::fill(out, out + framesPerBuffer, 0.0f);
    }
    
    return paContinue;
}