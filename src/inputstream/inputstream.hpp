#pragma once

#include <stdint.h>


class InputStream {
public:
  virtual ~InputStream() {}

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
   * file path or nullptr if not applicable
   */
  virtual char const * getPath() = 0;

  /**
   * construct a copy of the stream (reset to the start)
   */
  virtual InputStream * newCopy() = 0;

  /**
   * close the stream
   */
  virtual void close() = 0;
};
