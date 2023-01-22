#include <stdio.h>
#include <thread>
#include <mutex>
#include <grpcpp/grpcpp.h>
#include <google/protobuf/empty.pb.h>
#include <fstream>

#include "control.h"
#include "control.pb.h"
#include "control.grpc.pb.h"

using std::string;
using grpc::Channel;
using grpc::ClientContext;
using google::protobuf::Empty;

const string GRPC_SERVER = "localhost:8081";
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
static const std::string slash="\\";
#else
static const std::string slash="/";
#endif

struct BankAndByteOffset {
    uint32_t bank;
    uint32_t byteOffset;

    bool operator < (const BankAndByteOffset &other) const {
        if (bank != other.bank) {
            return bank < other.bank;
        } else {
            return byteOffset < other.byteOffset;
        }
    }
};

// Inline helpers

// trim from start (in place)
static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s) {
    rtrim(s);
    ltrim(s);
}

// State variables

std::thread listenerThread;
std::shared_ptr<Channel> channel;

std::mutex bytesMutex;
std::vector<WatchedWRAMRange> currentWatchedRanges;
std::map<BankAndByteOffset, std::vector<uint8_t>> latestBytesForOffset;

// Functions

void _StartListeningOnThread(std::unique_ptr<ControlService::Stub> service) {
    ClientContext context;
    auto reader = service->ListenWatchWRAM(&context, Empty());

    WatchedWRAM msg;
    while (reader->Read(&msg)) {
        std::lock_guard<std::mutex> lock(bytesMutex);
        std::vector<WatchedWRAMRange> watchedRanges;
        for (auto range : msg.ranges()) {
            watchedRanges.push_back(range);
        }
        currentWatchedRanges = watchedRanges;
    }
}

/// Open the emulator config file at the path in the same directory as `romPath`
void OpenEmulatorConfigNearRomPath(const char *romPath) {
    std::string _romPath(romPath);
    auto tokenPath = _romPath.substr(0, _romPath.find_last_of(slash));
    tokenPath.append(slash);
    tokenPath.append("emulator_token.txt");

    std::ifstream tokenStream(tokenPath);
    if (!tokenStream.good()) {
        printf("Cannot open file at path: %s\n", tokenPath.c_str());
        return;
    }

    std::string tokenString((std::istreambuf_iterator<char>(tokenStream)), (std::istreambuf_iterator<char>()));
    trim(tokenString);
    printf("Token string: %s\n", tokenString.c_str());
}

/// Open a connection to the gRPC server and listen for updates to WRAM
void StartListeningForWRAMUpdates() {
    auto _channel = grpc::CreateChannel(GRPC_SERVER, grpc::InsecureChannelCredentials());
    channel.swap(_channel);

    auto service = ControlService::NewStub(channel);
    std::thread _listenerThread(_StartListeningOnThread, std::move(service));
    listenerThread = std::move(_listenerThread);
}

/// Close any open gRPC connections
void StopListeningForWRAMUpdates() {
    // Replace the channel with a null one
    std::shared_ptr<Channel> nullChannel;
    channel.swap(nullChannel);
}

size_t CountOfWatchedRanges() {
    std::lock_guard<std::mutex> lock(bytesMutex);
    return currentWatchedRanges.size();
}

WatchedByteRange GetWatchedByteRange(size_t index) {
    std::lock_guard<std::mutex> lock(bytesMutex);

    auto _byteRange = currentWatchedRanges[index];
    WatchedByteRange result;
    result.bank = _byteRange.bank();
    result.byteOffset = _byteRange.byte_offset();
    result.byteLength = _byteRange.byte_length();
    return result;
}

void _SendByteRangeOnThread(std::unique_ptr<ControlService::Stub> service, WRAMByteRange byteRange) {
    ClientContext context;
    auto status = service->WatchedWRAMDidChange(&context, byteRange, nullptr);
    if (status.ok() == false) {
        printf("Invalid status from sending watched WRAM\n");
    }
}

void UpdateByteRange(size_t index, WatchedByteRange byteRange, uint8_t *bytes) {
    std::lock_guard<std::mutex> lock(bytesMutex);

    std::vector<uint8_t> bytesToSend(bytes, bytes + byteRange.byteLength);
    BankAndByteOffset offset;
    offset.bank = byteRange.bank;
    offset.byteOffset = byteRange.byteOffset;

    if (latestBytesForOffset.count(offset) > 0) {
        auto latestBytes = latestBytesForOffset.at(offset);
        if (bytesToSend == latestBytes) {
            // We have already sent these same bytes. Return early
            return;
        }
    }
    
    WRAMByteRange _byteRange;
    _byteRange.set_bank(byteRange.bank);
    _byteRange.set_byte_offset(byteRange.byteOffset);
    _byteRange.set_bytes(bytes, byteRange.byteLength);

    auto service = ControlService::NewStub(channel);
    std::thread _senderThread(_SendByteRangeOnThread, std::move(service), _byteRange);
    _senderThread.detach();
    latestBytesForOffset[offset] = bytesToSend;
}
