#ifndef CONTROL_H
#define CONTROL_H

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

#include <stdio.h>
#include <stdint.h>

typedef struct WatchedByteRange {
    uint32_t bank;
    uint32_t byteOffset;
    uint32_t byteLength;
} WatchedByteRange;

/// Open the emulator config file at the path in the same directory as `romPath`
EXTERNC void OpenEmulatorConfigNearRomPath(const char *romPath);
/// Open a connection to the gRPC server and listen for updates to WRAM
EXTERNC void StartListeningForWRAMUpdates();
/// Close any open gRPC connections
EXTERNC void StopListeningForWRAMUpdates();
/// Get the count of byte ranges that the server is watching
EXTERNC size_t CountOfWatchedRanges();
/// Get the watched byte range at the provided index
EXTERNC WatchedByteRange GetWatchedByteRange(size_t index);
/// Update the server with the provided byte range if necessary
EXTERNC void UpdateByteRange(size_t index, WatchedByteRange byteRange, uint8_t *bytes);

#undef EXTERNC
#endif /* CONTROL_H */
