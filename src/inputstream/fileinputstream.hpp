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

  virtual char next() override;

  virtual void rewind(int i) override;

private:
  static uint16_t const BUF_SIZE = 256;

  FILE * file_;
  char buffer_[BUF_SIZE];
  uint16_t bufIndex_;
  uint16_t bufLen_;
};
