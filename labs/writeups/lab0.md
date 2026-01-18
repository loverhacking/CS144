Lab 0 Writeup
=============

## Writing webget
* follow the order:
  * build socket connection
  * send HTTP GET request by writing to socket (note the proper formatting of HTTP request)
  * read response from socket until EOF

## An in-memory reliable byte stream
* use a circular buffer implemented by `std::vector` under the hood to store bytes
* use `head` and `tail` to track the read and write positions
  * initially both `head` and `tail` are 0
  * use another variable `size` to track the number of bytes currently in the buffer
  to avoid ambiguity when `head == tail` and to check for full/empty conditions

