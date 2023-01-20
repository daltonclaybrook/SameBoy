#include <stdio.h>
#include <thread>
#include <mutex>
#include <grpcpp/grpcpp.h>
#include <google/protobuf/empty.pb.h>

#include "control.h"
#include "control.pb.h"
#include "control.grpc.pb.h"

using std::string;
using grpc::Channel;
using grpc::ClientContext;
using google::protobuf::Empty;

const string GRPC_SERVER = "localhost:8080";

// State variables

std::thread listenerThread;
std::shared_ptr<Channel> channel;
std::mutex setWRAMInfoStackMutex;
std::vector<SetWRAMInfo> setWRAMInfoStack;

// Functions

void _StartListeningOnThread(std::unique_ptr<ControlService::Stub> service) {
    ClientContext context;
    auto reader = service->ListenSetWRAM(&context, Empty());

    WRAMByteRange msg;
    while (reader->Read(&msg)) {
        std::lock_guard<std::mutex> lock(setWRAMInfoStackMutex);
        auto bytesString = msg.bytes();
        SetWRAMInfo info;
        info.bank = msg.bank();
        info.byteCount = bytesString.length();
        info.byteOffset = msg.byte_offset();
        info.bytes = (uint8_t *)malloc(sizeof(uint8_t) * info.byteCount);
        memcpy(info.bytes, bytesString.c_str(), info.byteCount);
        setWRAMInfoStack.push_back(info);
    }
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

/// Pops an instance of `SetWRAMInfo` off the stack if one is available.
/// If a null pointer is returned, there are none left on the stack.
SetWRAMInfo* PopAndCopySetWRAMStack() {
    std::lock_guard<std::mutex> lock(setWRAMInfoStackMutex);
    if (setWRAMInfoStack.empty()) {
        return nullptr;
    }

    SetWRAMInfo *info = (SetWRAMInfo *)malloc(sizeof(SetWRAMInfo));
    memcpy(info, &setWRAMInfoStack[0], sizeof(SetWRAMInfo));
    setWRAMInfoStack.erase(setWRAMInfoStack.begin());
    return info;
}

/// Release an instance of `SetWRAMInfo` returned by `PopAndCopySetWRAMStack`
void ReleaseSetWRAMInfo(SetWRAMInfo *info) {
    free(info->bytes);
    free(info);
}
