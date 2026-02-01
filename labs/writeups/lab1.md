Lab 1 Writeup
=============

## Putting substrings in sequence
The core task of the lab is to implement `StreamReassembler` class
* I use **Circular Buffer** data structure to store the unassembled data.

* Filter data: Discard processed data (old data) and data exceeding the capacity range (overflow data).
* Write to buffer: Fill the corresponding position in the circular buffer with valid data and mark valid.
* Extract continuous data: Check if all data starting from `_nextIndex` is in the buffer. 
If so, write it to the `_output` stream and advance the window. 
* *Pay attention to the capacity restrictions.* (The core point of this lab)
  * assembled but unread data in ByteStream (Green) + unassembled data (Red) <= capacity according to the lab doc.
  * The `_nextIndex` is the boundary of assembled but unread data in ByteStream(Green) 
  and unassembled data(Red), i.e. the first index of unassembled data.
  * use **slide window** to manage the data in the buffer. Only update the window (i.e. update the `_nextIndex`) 
  when continuous data is extracted.
  * Only extract continuous data when the `_output` stream has enough capacity.
  

