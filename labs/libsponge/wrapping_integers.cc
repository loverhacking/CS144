#include "wrapping_integers.hh"
#include <cmath>
#include <cstdint>

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    constexpr uint64_t mod = 1ULL << 32;    // 2^32
    uint32_t ans = isn.raw_value() + static_cast<uint32_t>(n % mod);
    return WrappingInt32{ans};
}

static inline uint64_t abs_diff(uint64_t a, uint64_t b) {
    return (a > b) ? a - b : b - a;
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    constexpr uint64_t mod = 1ULL << 32;
    uint32_t offset = n.raw_value() - isn.raw_value(); // offset = (n - isn) mod 2^32

    // Align to checkpoint's high 32 bits
    const uint64_t base = (checkpoint & 0xFFFFFFFF00000000ULL) + offset;
    uint64_t best = base;
    uint64_t best_diff = abs_diff(best, checkpoint);

    // Candidate in next wrap period: base + 2^32
    if (base + mod <= UINT64_MAX) {
        uint64_t cand = base + mod;
        uint64_t cand_diff = abs_diff(cand, checkpoint);
        if (cand_diff < best_diff) {
            best = cand;
            best_diff = cand_diff;
        }
    }

    // Candidate in previous wrap period: base - 2^32 (only if no underflow)
    if (base >= mod) {
        uint64_t cand = base - mod;
        uint64_t cand_diff = abs_diff(cand, checkpoint);
        if (cand_diff < best_diff) {
            best = cand;
            best_diff = cand_diff;
        }
    }
    return best;
}


