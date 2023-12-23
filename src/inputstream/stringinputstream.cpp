
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

char StringInputStream::peek() {
  if( index_ >= len_ ){
    return '\0';
  } else {
    return line_[index_];
  }
}

char StringInputStream::next() {
  char c = peek();
  index_++;
  return c;
}

char const * StringInputStream::getPath() {
  return nullptr;
}

InputStream * StringInputStream::newCopy() {
    return new StringInputStream(line_);
}

void StringInputStream::close() {
}
