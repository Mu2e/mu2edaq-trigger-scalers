#pragma once

// Default trigger set matching config/scalars.yaml.
// Each entry gives the category id, display name, and simulated rate in Hz.
// Both sender tools share this header.

#include <cstdint>
#include <random>

struct TriggerSim {
    uint32_t    id;
    const char* name;
    double      rate_hz;   // Expected triggers per second
};

// clang-format off
inline const TriggerSim kTriggers[] = {
    // Beam & timing (ids 0-5)
    { 0,  "Mu2e Spill",       0.6  },
    { 1,  "Booster Spill",   15.0  },
    { 2,  "Cosmics Pulser",  10.0  },
    { 3,  "Pulser (Rand)",   10.0  },
    { 4,  "Manual",           0.1  },
    { 5,  "Accelerator Ext",  1.0  },
    // Data-driven triggers (ids 6-9)
    { 6,  "Helix: Base",      5.0  },
    { 7,  "Helix: Tight",     2.0  },
    { 8,  "Helix: Prescale",  1.0  },
    { 9,  "Activity: Base",   5.0  },
    // Summary aggregates (ids 16-17)
    { 16, "Helix: All",       8.0  },
    { 17, "All Triggers",    40.0  },
};
// clang-format on

inline constexpr int kNumTriggers = static_cast<int>(sizeof(kTriggers) / sizeof(kTriggers[0]));

// Simulate one time step of duration dt_s seconds.
// Returns the number of new events for trigger at index i (Poisson-distributed).
inline uint64_t poissonFire(int i, double dt_s, std::mt19937_64& rng) {
    const double lambda = kTriggers[i].rate_hz * dt_s;
    if (lambda <= 0.0) return 0;
    std::poisson_distribution<uint64_t> dist(lambda);
    return dist(rng);
}
