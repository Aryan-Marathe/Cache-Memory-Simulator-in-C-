#include <iostream>
#include <vector>
#include <cmath>
#include <list>
#include <algorithm> // for find_if
#include <cstdint>   // for uint64_t
#include <iomanip>


using namespace std;

// --- Configuration ---
// constexpr ensures these are known at compile-time
constexpr int CACHE_SIZE = 32 * 1024; // 32KB
constexpr int BLOCK_SIZE = 64;        // 64 Bytes
constexpr int WAYS = 4;               // 4-Way Associative
constexpr int NUM_SETS = CACHE_SIZE / (BLOCK_SIZE * WAYS);

// --- Type Aliases for Clarity ---
using Address = uint64_t;

// --- Compile-time Math Helper ---
// A constexpr function to calculate log2 at compile time
constexpr int get_log2(int n) {
    int ret = 0;
    while (n > 1) {
        n >>= 1;
        ret++;
    }
    return ret;
}

// Calculate bit widths once at compile time
constexpr int OFFSET_BITS = get_log2(BLOCK_SIZE);
constexpr int INDEX_BITS  = get_log2(NUM_SETS);

struct CacheLine {
    bool valid = false; // Default member initialization
    Address tag = 0;
};

class CacheSimulator {
private:
    // Alias for the Set type to make code cleaner
    using Set = list<CacheLine>;
    vector<Set> sets;
    
    // Statistics
    long long accesses = 0;
    long long hits = 0;
    long long misses = 0;

public:
    CacheSimulator() {
        sets.resize(NUM_SETS);
    }

    void accessMemory(Address address) {
        accesses++;
        
        // Bit manipulation constants are now calculated at compile-time (no runtime log2)
        // 
        const Address index_mask = (1ULL << INDEX_BITS) - 1;
        
        const Address index = (address >> OFFSET_BITS) & index_mask;
        const Address tag   = address >> (OFFSET_BITS + INDEX_BITS);

        auto& current_set = sets[index];

        // Modern C++: Use std::find_if with a lambda instead of a raw loop
        auto it = find_if(current_set.begin(), current_set.end(), 
            [tag](const CacheLine& line) {
                return line.valid && line.tag == tag;
            });

        if (it != current_set.end()) {
            // --- HIT ---
            hits++;
            // LRU Update: Move the found element to the front (splice is O(1))
            current_set.splice(current_set.begin(), current_set, it);
        } else {
            // --- MISS ---
            misses++;
            // Emplace front creates the object directly in the list (more efficient than push_front)
            current_set.emplace_front(CacheLine{true, tag});

            // Eviction: Enforce Set size
            if (current_set.size() > WAYS) {
                current_set.pop_back();
            }
        }
    }

    void printStats() const { // marked const as it doesn't modify state
        cout << "--- Cache Configuration ---\n";
        cout << "Size: " << CACHE_SIZE / 1024 << "KB | Ways: " << WAYS << '\n';
        
        cout << "--- Simulation Results ---\n";
        cout << "Total Accesses: " << accesses << '\n';
        cout << "Hits:           " << hits << '\n';
        cout << "Misses:         " << misses << '\n';
        
        double hitRate = (accesses > 0) ? (static_cast<double>(hits) / accesses) * 100.0 : 0.0;
        
        cout << "Hit Rate:       " << fixed << setprecision(2) 
             << hitRate << "%\n";
    }
};

int main() {
    CacheSimulator sim;
    
    // Initializer list for vector (cleaner than C-style array)
    const vector<Address> trace = {
        0x1000, 0x1004, 0x1008, // Spatial locality
        0x2000, 0x2004,         
        0x1000,                 // Temporal locality
        0x3000, 0x4000, 0x5000, 0x6000, 0x7000 // Thrashing
    };

    // Range-based for loop
    for (const auto& addr : trace) {
        sim.accessMemory(addr);
    }

    sim.printStats();

    return 0;
}