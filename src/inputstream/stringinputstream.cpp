
#include "stringinputstream.hpp"

#include <string.h>
#include <algorithm>


StringInputStream::StringInputStream(char const * line) {
  line_ = line;
  index_ = 0;
  len_ = (int)strlen(line);
}

StringInputStream::~StringInputStream() {
}

char StringInputStream::next() {
  if( index_ >= len_ ){
    return '\0';
  } else {
    return line_[index_++];
  }
}

void StringInputStream::rewind(int i) {
  index_ = std::max(0, index_ - i);
}
