#pragma once

#include <stdint.h>
#include "inputstream.hpp"


class StringInputStream : public InputStream {
public:
  StringInputStream(char const * line);

  virtual ~StringInputStream();

  virtual char peek() override;

  virtual char next() override;

  virtual char const * getPath() override;

  virtual InputStream * newCopy() override;

  virtual void close() override;

private:
  char const * line_;
  int index_;
  int len_;
};
