
#include "fileinputstream.hpp"

#include <string.h>


FileInputStream::FileInputStream() {
  file_ = nullptr;
  current_ = ' ';
}

FileInputStream::~FileInputStream() {
  close();
}

bool FileInputStream::open(char const * path) {
  file_ = fopen(path, "rb");
  path_ = path;
  if( file_ == nullptr ){
    return false;
  }
  // prime the pump:
  next_();
  return true;
}

void FileInputStream::close() {
  if( file_ != nullptr ){
    fclose(file_);
    file_ = nullptr;
  }
}

char FileInputStream::peek() {
  return current_;
}

char FileInputStream::next() {
  char c = current_;
  if( c != '\0' ){
    next_();
  }
  return c;
}

bool FileInputStream::next_() {
  long int i = ftell(file_);
  // pop one character
  int c = fgetc(file_);
  if( c == EOF ){  // Error or End of file
    close();
    file_ = nullptr;
    current_ = '\0';
  }else{
    current_ = (char)c;
  }
  return current_;
}

char const * FileInputStream::getPath() {
  return path_;
}
