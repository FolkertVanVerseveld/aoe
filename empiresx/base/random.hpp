#pragma once

/*
 * Provides random devices that are guaranteed to yield the same sequence on all
 * platforms if initialised with the same seed.
 */

#include <cstdint>

namespace genie {

/**
 * Linear Congruent Generator
 * Generic interface for non-cryptographically secure pseudo random number generators (PRNGs)
 * See also https://en.wikipedia.org/wiki/Linear_congruential_generator
 */
class LCG {
    const uint64_t m, a, c;
    uint64_t v, mask;
    /** Range of bits [start, end) that is provided from the seed when next() is called */
    unsigned start, end;
public:
    LCG(uint64_t modulo, uint64_t multiplier, uint64_t increment, unsigned start, unsigned end, uint64_t seed=1)
        : m(modulo), a(multiplier), c(increment), start(start), end(end), v(seed), mask(0)
    {
        for (unsigned i = 0; i < end - start; ++i)
            mask = (mask << 1) | 1;
    }

    void seed(uint64_t v) { this->v = v; }

    uint64_t next() {
        v = (a * v + c) % m;
        return (v >> start) & mask;
    }

    const unsigned bits() const { return end - start; }

    static LCG ansi_c(uint64_t seed=1) {
        return LCG(1UL << 31, 0x41C64E6D, 0x3039, 16, 31, seed);
    }
};

}
