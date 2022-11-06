
#include "table.hpp"

// ----------------------------------------------------------------------------
// String Comparison Helpers
// ----------------------------------------------------------------------------
std::size_t StringHash::operator()(String const * key) const {
    return key->getHash();
}

bool StringEqual::operator()(String const * lhs, String const * rhs) const {
    return (lhs->getLength() == rhs->getLength()) && 
            (memcmp(lhs->get(), rhs->get(), lhs->getLength()) == 0);
}

// ----------------------------------------------------------------------------
// InternedStringSet
// ----------------------------------------------------------------------------
StringSet::StringSet() {
}

StringSet::~StringSet() {
}

ObjString * StringSet::find(char const * chars, int len) {
    // Search if string is already interned:
    StringView lookup(chars, len);
    auto key = set_.find(&lookup);
    if( key == set_.end() )  return nullptr;  // not found
    // Found it:
    // Must be an ObjString because thats all we ever add to the set
    return (ObjString*) *key;
}

void StringSet::add(ObjString * ostr) {
    set_.emplace(ostr);
}

void StringSet::debug() {
    printf("Interned string set:\n");
    for( auto & it : set_ ){
        printf("  %p: 0x%8x %3i '%s'\n", 
               (ObjString*) it, it->getHash(), it->getLength(), it->get());
    }
}
