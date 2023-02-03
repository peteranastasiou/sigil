
#include "fileinputstream.hpp"

#include <stdio.h>
#include <string.h>


FileInputStream::FileInputStream() {
  file_ = nullptr;
  bufIndex_ = 0;
  bufLen_ = 0;
}

FileInputStream::~FileInputStream() {
  close();
}

bool FileInputStream::open(char const * path) {
  file_ = fopen(path, "rb");
  return file_ != nullptr;
}

void FileInputStream::close() {
  if( file_ != nullptr ){
    fclose(file_);
    file_ = nullptr;
  }
}

char FileInputStream::next() {
  // pop one character
  int c = fgetc(file_);
  if( c == EOF ){  // Error or End of file
    close();
    file_ = nullptr;
    return '\0';
  }
  return (char)c;
}

void FileInputStream::rewind(int i) {
  fseek(file_, -i, SEEK_CUR);
}
