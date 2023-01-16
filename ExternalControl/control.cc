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

const string GRPC_SERVER = "localhost:50051";

void doSomethingInteresting() {
    ClientContext context;
    printf("This is a test...\n");
}

SetWRAMHandler setWRAMHandler = nullptr;
std::mutex handlerMutex;
std::thread listenerThread;

void _StartListeningOnThread() {
    auto channel = grpc::CreateChannel(GRPC_SERVER, grpc::InsecureChannelCredentials());
    auto service = ControlService::NewStub(channel);

    ClientContext context;
    auto reader = service->ListenSetWRAM(&context, Empty());
    while (true) {
        SetWRAM msg;
        bool success = reader->Read(&msg);
        if (!success) {
            printf("Failed to read message. Sleeping...\n");
            std::this_thread::sleep_for(std::chrono::seconds(10));
            continue;
        }

        std::lock_guard<std::mutex> lock(handlerMutex);
        if (setWRAMHandler != nullptr) {
            SetWRAMInfo info;
            info.bank = msg.bank();
            info.byteCount = msg.bytes().length();
            info.byteOffset = msg.byte_offset();
            info.bytes = (uint8_t *)msg.bytes().c_str();
            setWRAMHandler(info);
        }
    }
}

/// Register the handler that is called when WRAM should be set
void RegisterSetWRAMHandler(SetWRAMHandler handler) {
    std::lock_guard<std::mutex> lock(handlerMutex);
    setWRAMHandler = handler;
}

/// Deregister the handler that was previously set
void DeregisterSetWRAMHandler() {
    setWRAMHandler = nullptr;
}

/// Open a connection to the gRPC server and listen for updates to WRAM
void StartListeningForWRAMUpdates() {
    std::thread _listenerThread(_StartListeningOnThread, nullptr);
    listenerThread = std::move(_listenerThread);
}

/// Close any open gRPC connections
void StopListeningForWRAMUpdates() {
    // Replace the thread with a null one, causing the old one to be terminated
    listenerThread = std::thread();
}
