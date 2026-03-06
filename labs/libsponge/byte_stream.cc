#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in
// `byte_stream.hh`

template <typename... Targs> void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity)
    : _buffer(capacity), _capacity(capacity), _head(0), _tail(0),
      _write_bytes(0), _read_bytes(0), _size(0), _input_ended(false) {}

size_t ByteStream::write(const string &data) {
  size_t n = min(data.size(), remaining_capacity());
  // First block: How much can be written from _tail to the end of the buffer?
  size_t first_chunk = min(n, _capacity - _tail);
  std::copy(data.begin(), data.begin() + first_chunk, _buffer.begin() + _tail);

  // Second block: If there's still more to write, wrap around to the beginning of the buffer
  if (first_chunk < n) {
    std::copy(data.begin() + first_chunk, data.begin() + n, _buffer.begin());
  }
  _tail = (_tail + n) % _capacity;
  _write_bytes += n;
  _size += n;
  return n;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
  size_t n = min(len, buffer_size());
  string s;
  s.resize(n); // allocate space in advance!

  // First block: How much can be read from _tail to the end of the buffer?
  size_t first_chunk = min(n, _capacity - _head);
  std::copy(_buffer.begin() + _head, _buffer.begin() + _head + first_chunk, s.begin());

  // Second block: If there's still more to read, wrap around to the beginning of the buffer
  if (first_chunk < n) {
    std::copy(_buffer.begin(), _buffer.begin() + (n - first_chunk), s.begin() + first_chunk);
  }
  return s;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
  int n = min(len, buffer_size());
  _head = (_head + n) % _capacity;
  _read_bytes += n;
  _size -= n;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
  string s = peek_output(len);
  pop_output(s.size());
  return s;
}

// Signal that the byte stream has reached its ending
void ByteStream::end_input() { _input_ended = true; }

// `true` if the stream input has ended
bool ByteStream::input_ended() const { return _input_ended; }

// the maximum amount that can currently be peeked/read
size_t ByteStream::buffer_size() const { return _size; }

// `true` if the buffer is empty
bool ByteStream::buffer_empty() const { return buffer_size() == 0; }

// `true` if the output has reached the ending
bool ByteStream::eof() const {
  return input_ended() &&
         buffer_empty(); // true if input ended and no more data to read
}

size_t ByteStream::bytes_written() const { return _write_bytes; }

size_t ByteStream::bytes_read() const { return _read_bytes; }

size_t ByteStream::remaining_capacity() const {
  return _capacity - buffer_size();
}
