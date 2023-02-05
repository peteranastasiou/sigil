#pragma once

#include <stdint.h>
#include "inputstream.hpp"


class StringInputStream : public InputStream {
public:
  StringInputStream(char const * line);

  ~StringInputStream();

  virtual char peek() override;

  virtual char next() override;

  virtual void rewind(int i) override;

  virtual char const * name() override;

private:
  char const * line_;
  int index_;
  int len_;
};
