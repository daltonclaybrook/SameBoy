#ifndef CONTROL_H
#define CONTROL_H

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

#include <stdio.h>
#include <stdint.h>

typedef struct SetWRAMInfo {
    uint64_t byteCount;
    uint64_t byteOffset;
    uint8_t *bytes;
    uint32_t bank;
} SetWRAMInfo;

/// Open a connection to the gRPC server and listen for updates to WRAM
EXTERNC void StartListeningForWRAMUpdates();
/// Close any open gRPC connections
EXTERNC void StopListeningForWRAMUpdates();
/// Pops an instance of `SetWRAMInfo` off the stack if one is available.
/// If a null pointer is returned, there are none left on the stack. If a
/// non-null pointer is returned, the caller is responsible for managing
/// the memory of this pointer.
EXTERNC SetWRAMInfo* PopAndCopySetWRAMStack();
/// Release an instance of `SetWRAMInfo` returned by `PopAndCopySetWRAMStack`
EXTERNC void ReleaseSetWRAMInfo(SetWRAMInfo *info);

#undef EXTERNC
#endif /* CONTROL_H */
