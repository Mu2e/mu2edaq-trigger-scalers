#pragma once

#include <cstdint>

// Wire format for trigger scalar messages (12 bytes, little-endian).
// Broadcast by the DAQ system via UDP or ZeroMQ PUB/PUSH socket.
// Each message reports the cumulative count for one trigger category.
struct TriggerMessage {
    uint32_t category;  // Trigger category index (0-based, matches YAML config)
    uint64_t count;     // Cumulative trigger count for this category
} __attribute__((packed));

static_assert(sizeof(TriggerMessage) == 12, "TriggerMessage must be 12 bytes");
