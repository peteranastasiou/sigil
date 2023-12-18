#pragma once

#include "inputstream.hpp"

#include <stdlib.h>
#include <stdio.h>


class FileInputStream : public InputStream {
public:
  FileInputStream();

  ~FileInputStream();

  bool open(char const * path);

  void close();

  virtual char peek() override;

  virtual char next() override;

  virtual char const * getPath() override;

private:
  bool next_();

  static uint16_t const BUF_SIZE = 256;

  FILE * file_;
  char const * path_;
  char current_;
};
