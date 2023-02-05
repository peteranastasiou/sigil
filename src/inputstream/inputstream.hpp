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
   * return the next input character, without advancing
   * null value (0) indicates end of stream
   */
  virtual char peek() = 0;

  /**
   * Rewind input stream by i characters
   */
  virtual void rewind(int i) = 0;

  /**
   * file path or nullptr if not applicable
   */
  virtual char const * getPath() = 0;
};
