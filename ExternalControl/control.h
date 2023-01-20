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

/// Open a connection to the gRPC server and listen for updates to WRAM
EXTERNC void StartListeningForWRAMUpdates();
/// Close any open gRPC connections
EXTERNC void StopListeningForWRAMUpdates();
EXTERNC size_t CountOfWatchedRanges();
EXTERNC WatchedByteRange GetWatchedByteRange(size_t index);
EXTERNC void UpdateByteRange(size_t index, uint32_t bank, uint32_t byteOffset, uint32_t byteCount, uint8_t *bytes);

#undef EXTERNC
#endif /* CONTROL_H */
