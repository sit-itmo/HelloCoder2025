#pragma once

#include <chrono>
#include <unordered_map>
#include <cstdint>

class sProfiler
{
public:
    using Clock = std::chrono::steady_clock;

    struct CounterSnapshot
    {
        std::uint64_t Count = 0;      // How many times End() was called
        double        TotalMs = 0.0;  // Total accumulated time in milliseconds
    };

    // Start measuring for the given counter id
    void Begin(sID id)
    {
        auto& c = _counters[id];
        c.activeStarts.push_back(Clock::now());
    }

    // Stop measuring for the given counter id and accumulate time
    void End(sID id)
    {
        auto it = _counters.find(id);
        if (it == _counters.end())
            return; // or assert / throw

        Counter& c = it->second;
        if (c.activeStarts.empty())
            return; // or assert / throw

        auto start = c.activeStarts.back();
        c.activeStarts.pop_back();

        auto dt = Clock::now() - start;
        c.totalTime += std::chrono::duration_cast<std::chrono::nanoseconds>(dt);
        ++c.count;
    }

    // Reset a single counter
    void Reset(sID id)
    {
        _counters.erase(id);
    }

    // Reset all counters
    void ResetAll()
    {
        _counters.clear();
    }

    // Get snapshot for a single counter
    CounterSnapshot GetSnapshot(sID id) const
    {
        CounterSnapshot snap;

        auto it = _counters.find(id);
        if (it == _counters.end())
            return snap;

        const Counter& c = it->second;
        snap.Count = c.count;
        snap.TotalMs = c.totalTime.count() / 1'000'000.0; // ns -> ms
        return snap;
    }

    // Get snapshots for all counters
    std::unordered_map<sID, CounterSnapshot> GetAllSnapshots() const
    {
        std::unordered_map<sID, CounterSnapshot> result;
        result.reserve(_counters.size());

        for (const auto& kv : _counters)
        {
            const sID id = kv.first;
            const Counter& c = kv.second;

            CounterSnapshot snap;
            snap.Count = c.count;
            snap.TotalMs = c.totalTime.count() / 1'000'000.0; // ns -> ms

            result[id] = snap;
        }

        return result;
    }

private:
    struct Counter
    {
        std::uint64_t count = 0;
        std::chrono::nanoseconds totalTime{ 0 };
        // Allows nested Begin/End for same id (stack)
        std::vector<Clock::time_point> activeStarts;
    };

    std::unordered_map<sID, Counter> _counters;
};
