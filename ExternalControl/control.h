#ifndef CONTROL_H
#define CONTROL_H

#include <stdio.h>
#include <stdint.h>

typedef struct SetWRAMInfo {
    uint64_t byteCount;
    uint64_t byteOffset;
    uint8_t *bytes;
    uint32_t bank;
} SetWRAMInfo;

/// @brief A handler function for setting bytes in WRAM
typedef void (*SetWRAMHandler)(SetWRAMInfo bytes);

/// Register the handler that is called when WRAM should be set
void RegisterSetWRAMHandler(SetWRAMHandler handler);
/// Deregister the handler that was previously set
void DeregisterSetWRAMHandler();
/// Open a connection to the gRPC server and listen for updates to WRAM
void StartListeningForWRAMUpdates();
/// Close any open gRPC connections
void StopListeningForWRAMUpdates();

#endif /* CONTROL_H */
