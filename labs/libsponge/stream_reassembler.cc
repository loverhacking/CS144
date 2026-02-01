#include "stream_reassembler.hh"
#include <limits>

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity)
    : _output(capacity)
    , _capacity(capacity)
    , _nextIndex(0)
    , _unassembledBytes(0)
    , _eofIndex(numeric_limits<size_t>::max())
    , _buffer(capacity, '\0')
    , _valid(capacity, false)
    {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    // Update EOF index
    if (eof) {
        _eofIndex = min(_eofIndex, index + data.size());
    }

    // If the segment ends before what we expect, ignore it
    if (index + data.size() <= _nextIndex) {
        // Already assembled
        if (_nextIndex >= _eofIndex && empty()) {
            _output.end_input();
        }
        return;
    }

    size_t bytes_in_stream = _output.buffer_size(); 
    // Calculate the remaining capacity for the reassembler
    size_t effective_capacity = _capacity - bytes_in_stream;

    // If the segment starts beyond our current window, ignore it
    if (index >= _nextIndex + effective_capacity) {
        return;
    }

    // Determine the portion of data that fits in our buffer
    size_t startPos = max(index, _nextIndex);
    size_t endPos = min(index + data.size(), _nextIndex + effective_capacity);

    // Copy relevant data to buffer
    for (size_t i = startPos; i < endPos; i++) {
        size_t bufferIndex = i % _capacity;
        // If the position is not valid, mark it as valid and add the data to the buffer
        // and increment the unassembled bytes
        if (!_valid[bufferIndex]) {
            _valid[bufferIndex] = true;
            _buffer[bufferIndex] = data[i - index];
            _unassembledBytes++;
        }
    }

    string assembled;
    size_t bufferIndex = _nextIndex % _capacity;
    size_t availableBytes = _output.remaining_capacity();

    // Assemble contiguous bytes
    while (_valid[bufferIndex] && availableBytes > 0) {
        assembled.push_back(_buffer[bufferIndex]);
        _valid[bufferIndex] = false;
        _nextIndex++;
        _unassembledBytes--;
        bufferIndex = _nextIndex % _capacity;

        availableBytes--;
    }

    // Write the assembled bytes to the output stream
    if (!assembled.empty()) {
        _output.write(assembled);
    }

    // Check for EOF condition
    if (_eofIndex <= _nextIndex && empty()) {
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { return _unassembledBytes; }

bool StreamReassembler::empty() const { return _unassembledBytes == 0; }
