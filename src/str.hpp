
#pragma once

#include "object.hpp"

#include <stdio.h>
#include <stdint.h>

// predeclare Mem
class Mem;

/**
 * Interface for strings
 */
class String {
public:
    virtual ~String() {}
    virtual char const * get() const = 0;
    virtual uint32_t getHash() const = 0;
    virtual int getLength() const = 0;
    virtual int type() const = 0;
};

/**
 * A StringView class which doesn't own its string
 */
class StringView: public String {
public:
    StringView(char const * c);
    StringView(char const * c, int len);

    virtual ~StringView() {}

    virtual char const * get() const override { return chars_; }
    virtual uint32_t getHash() const override { return hash_; }
    virtual int getLength() const override { return length_; }
    virtual int type() const override { return 0; };

private:
    char const * chars_;
    int length_;
    uint32_t hash_;
};

/**
 * Garbage-Collected String Object
*/
class ObjString : public Obj, public String {
public:
    /**
     * Constructor helpers - copies string memory into this class
     */
    static ObjString * newString(Mem * mem, char const * str);
    static ObjString * newString(Mem * mem, char const * str, int length);

    /**
     * Constructor helper to make a new formatted string
     */
    static ObjString * newStringFmt(Mem * mem, char const * format, ...);

    /**
     * Constructor helper to make a string from two other strings
     */
    static ObjString * concatenate(Mem * mem, ObjString * a, ObjString * b);

    /**
     * Indexing into string:
     */
    bool get(int index, char & value);

    char const * getCString() { return chars_; }

    virtual ~ObjString();

    // implment Obj interface (trivial for strings)
    virtual ObjString * toString() override { return this; }
    virtual void print(bool verbose) override;

    // implement String interface:
    virtual char const * get() const override { return chars_; }
    virtual uint32_t getHash() const override { return hash_; }
    virtual int getLength() const override { return length_; }
    virtual int type() const override { return 1; };
private:
    // Private constructor: must construct with helper!
    // Takes ownership of str
    ObjString(Mem * mem, char const * str, int length);

    char const * chars_;  // null terminated sequence
    int length_;          // number of characters, NOT including null terminator
    uint32_t hash_;
};
