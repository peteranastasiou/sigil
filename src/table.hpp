#pragma once

#include "str.hpp"
#include "value.hpp"

#include "string.h"
#include <unordered_set>
#include <unordered_map>

/**
 * Hash value of a String (for HashSet/HashMap impl)
 */
struct StringHash {
    std::size_t operator()(String const * key) const;
};

/**
 * Compare two Strings (for HashSet/HashMap impl)
 */
struct StringEqual {
    bool operator()(String const * lhs, String const * rhs) const;
};

/**
 * Set of strings. All elements must be ObjStrings
 * Key interface is used so we can do cheap lookups
 */
class StringSet {
public:
    StringSet();
    ~StringSet();

    ObjString * find(char const * chars, int len);
    void add(ObjString * ostr);

    void debug();

    void gcSweep();

private:
    std::unordered_set<String*, StringHash, StringEqual> set_;
};

/**
 * HashMap of (ObjString*) keys, and <V> values
 */
template<class V>
class HashMap {
public:
    HashMap() {}
    ~HashMap() {}

    /**
     * Set a value for the given key
     * @return true if the key is new
     */
    bool set(ObjString * key, V value) {
        // returns pair of iterator and bool isNew:
        return map_.insert_or_assign(key, value).second;
    }

    /**
     * Add a new key/value pair.
     * Doesn't update existing key values.
     * @return false if key already exists
     */
    bool add(ObjString * key, V value) {
        // returns pair of iterator and bool successfull:
        return map_.insert({key, value}).second;
    }

    /**
     * Look up a value for the given key
     * @return true if the key/value exists
     */
    bool get(ObjString * key, V & value) {
        auto search = map_.find(key);
        if( search == map_.end() ) return false;
        value = (*search).second;
        return true;
    }

    /**
     * Remove entry
     * @return true if an entry was deleted
     */
    bool remove(ObjString * key) {
        return map_.erase(key) == 1;
    }

    void gcMark() {
        // Mark all keys and values as in-use for the garbage collector:
        for( auto & [key, value] : map_ ){

#ifdef DEBUG_GC
            printf( "Mark key-value entry for key:" );
            ((ObjString *)key)->print(true);
            printf("\n");
#endif

            ((ObjString *)key)->gcMark();
            value.gcMark();
        }
    }

    // void debug() {
    //     for( const auto & [key, value] : map_ ){
    //         printf("  '%s': ", key->get());
    //         value.print();
    //         printf("\n");
    //     }
    // }

private:
    std::unordered_map<String*, V, StringHash, StringEqual> map_;
};
