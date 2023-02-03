#pragma once

#include <stdint.h>


class InputStream {
public:
  ~InputStream() {}

  /**
   * return the next input character
   * null value (0) indicates end of stream
   */
  virtual char next() = 0;

  /**
   * Rewind input stream by i characters
   */
  virtual void rewind(int i) = 0;
};
