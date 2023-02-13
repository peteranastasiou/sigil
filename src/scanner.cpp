
#include "scanner.hpp"

#include <string.h>
#include <stdio.h>


Scanner::Scanner() {
}

Scanner::~Scanner() {
}

void Scanner::init(Mem * mem, InputStream * stream) {
    mem_ = mem;
    stream_ = stream;
    line_ = 1;
    col_ = 0;
    tokenStrLen_ = 0;
}

char const * Scanner::getPath() {
    return stream_->getPath();
}

Token Scanner::peekToken() {
    // save state:
    auto pos = stream_->getPosition();
    auto line = line_;
    auto col = col_;

    Token t = scanToken();

    // restore state:
    stream_->setPosition(pos);
    line_ = line;
    col_ = col;

    return t;
}

Token Scanner::scanToken() {
    // first, gobble up whitespace and comments:
    skipWhitespace_();

    // reset token buffer:
    tokenStrLen_ = 0;

    // check for EOF:
    if( isAtEnd_() ) return makeToken_(Token::END);

    char c = nextChar_();

    // check for indentifier/keyword:
    if( isAlpha_(c) ) return makeIdentifierToken_();

    // check for number:
    if( isDigit_(c) ) return makeNumberToken_();

    // check for symbol:
    switch (c) {
        case '(': return makeToken_(Token::LEFT_PAREN);
        case ')': return makeToken_(Token::RIGHT_PAREN);
        case '{': return makeToken_(Token::LEFT_BRACE);
        case '}': return makeToken_(Token::RIGHT_BRACE);
        case '[': return makeToken_(Token::LEFT_BRACKET);
        case ']': return makeToken_(Token::RIGHT_BRACKET);
        case ';': return makeToken_(Token::SEMICOLON);
        case ',': return makeToken_(Token::COMMA);
        case '-': return makeToken_(Token::MINUS);
        case '+': return makeToken_(Token::PLUS);
        case '/': return makeToken_(Token::SLASH);
        case '*': return makeToken_(Token::STAR);
        case '!': return makeToken_(matchNext_('=') ? Token::BANG_EQUAL : Token::BANG);
        case '=': return makeToken_(matchNext_('=') ? Token::EQUAL_EQUAL : Token::EQUAL);
        case '<': return makeToken_(matchNext_('=') ? Token::LESS_EQUAL : Token::LESS);
        case '>': return makeToken_(matchNext_('=') ? Token::GREATER_EQUAL : Token::GREATER);
        case '"': return makeStringToken_();
    }

    printf("Unexpected '%c'\n", c);
    return makeErrorToken_("Unexpected character.");
}

void Scanner::incrementLine_() {
    // clamp at maximum value
    if( line_ < MAX_LINES ){
        line_++;
    }
    col_ = 0;
}

void Scanner::skipWhitespace_() {
    for(;;){
        switch( peek_() ){
            case '\n':
                incrementLine_();
                // Fall-through
            case ' ':
            case '\r':
            case '\t':
                nextChar_();
                break;

            case '#':
                // comment out the rest of the line:
                while( peek_()!='\n' && !isAtEnd_() ){
                    nextChar_();
                }
                break;

            default:
                return;
        }
    }
}

bool Scanner::isAtEnd_() {
    return stream_->peek() == '\0';
}

char Scanner::peek_() {
    return stream_->peek();
}

char Scanner::nextChar_() {
    char c = stream_->next();
    col_++;
    if( tokenStrLen_ < MAX_TOKEN_LEN ){
        tokenStr_[tokenStrLen_++] = c;
    }
    return c;
}

bool Scanner::matchNext_(char expected) {
    if( isAtEnd_() ) return false;  // EOF

    if( stream_->peek() == expected ){
        stream_->next();
        return true;
    }
    return false;
}

Token Scanner::makeToken_(Token::Type type) {
  return Token(type, line_, col_);
}

Token Scanner::makeErrorToken_(const char* message) {
  return Token(Token::ERROR, line_, col_, ObjString::newString(mem_, message));
}

Token Scanner::makeStringToken_() {
    // TODO string escape characters
    while( peek_() != '"' && !isAtEnd_() ){
        if( peek_() == '\n' ) incrementLine_();
        nextChar_();
    }

    if( isAtEnd_() ) return makeErrorToken_("Unterminated string");

    // consume the closing quote:
    nextChar_();

    // Don't include the quotes when capturing the string value:
    ObjString * str = ObjString::newString(mem_, tokenStr_+1, tokenStrLen_-2);
    return Token(Token::STRING, line_, col_, str);
}

Token Scanner::makeNumberToken_() {
    while( isDigit_(peek_()) ){
        nextChar_();
    }

    // Look for a fractional part.
    if( peek_() == '.' ) {
        // Consume the ".".
        nextChar_();

        // Must have a digit following '.':
        if( !isDigit_(peek_()) ){
            return makeErrorToken_("Malformed number");
        }

        while( isDigit_(peek_()) ){
            nextChar_();
        }
    }
    // For now, simply capture the string and convert to number later:
    ObjString * str = ObjString::newString(mem_, tokenStr_, tokenStrLen_);
    return Token(Token::NUMBER, line_, col_, str);
}

Token Scanner::makeIdentifierToken_() {
    while( isAlpha_(peek_()) || isDigit_(peek_()) ){
        nextChar_();
    }
    Token::Type type = identifierType_();
    // Only capture the source string if its an identifier:
    if( type == Token::IDENTIFIER ){
        ObjString * str = ObjString::newString(mem_, tokenStr_, tokenStrLen_);
        return Token(type, line_, col_, str);
    }
    return Token(type, line_, col_);
}

Token::Type Scanner::identifierType_() {
    // Use a trie to determine if the identifier is a keyword:
    switch( tokenStr_[0] ){
        case 'a': return checkKeyword_(1, 2, "nd", Token::AND);
        case 'b': return checkKeyword_(1, 3, "ool", Token::BOOL);
        case 'c': return checkKeyword_(1, 4, "onst", Token::CONST);
        case 'e': {
            // "e..." might be "echo", "else" or "elif":
            // check correct number of chars, and that next char is l:
            if( tokenStrLen_ == 4 ){
                if( tokenStr_[1] == 'c' ){
                    return checkKeyword_(2, 2, "ho", Token::ECHO);
                } else if( tokenStr_[1] == 'l' ){
                    if( tokenStr_[2] == 's' && tokenStr_[3] == 'e' ) return Token::ELSE;
                    if( tokenStr_[2] == 'i' && tokenStr_[3] == 'f' ) return Token::ELIF;
                }
            }
            break;
        }
        case 'f': {
            // "f..." might be "false", "for" or "fn":
            // first check if identifier is longer than 1 char:
            if( tokenStrLen_ > 1 ){
                switch( tokenStr_[1] ){
                    case 'a': return checkKeyword_(2, 3, "lse", Token::FALSE);
                    case 'o': return checkKeyword_(2, 1, "r", Token::FALSE);
                    case 'l': return checkKeyword_(2, 3, "oat", Token::FLOAT);
                    case 'n': return Token::FN;
                }
            }
            break;
        }
        case 'i': return checkKeyword_(1, 1, "f", Token::IF);
        case 'n': return checkKeyword_(1, 2, "il", Token::NIL);
        case 'o': {
            if( tokenStrLen_ > 1 ){
                switch( tokenStr_[1] ){
                    case 'b': return checkKeyword_(2, 4, "ject", Token::OBJECT);
                    case 'r': return Token::OR;
                }
            }
            break;
        }
        case 'p': return checkKeyword_(1, 4, "rint", Token::PRINT);
        case 'r': return checkKeyword_(1, 5, "eturn", Token::RETURN);
        case 's': return checkKeyword_(1, 5, "tring", Token::STRING_TYPE);
        case 't': {
            // could be "true", "type" or "typeid"
            if( tokenStrLen_ > 1 ){
                switch( tokenStr_[1] ){
                    case 'r': return checkKeyword_(2, 2, "ue", Token::TRUE);
                    case 'y': {
                        if( tokenStrLen_ == 4 ){
                            return checkKeyword_(2, 2, "pe", Token::TYPE);
                        }else{
                            return checkKeyword_(2, 4, "peid", Token::TYPEID);
                        }
                    }
                }
            }
            break;
        }
        case 'v': return checkKeyword_(1, 2, "ar", Token::VAR);
        case 'w': return checkKeyword_(1, 4, "hile", Token::WHILE);
    }
    // Not a keyword:
    return Token::IDENTIFIER;
}

Token::Type Scanner::checkKeyword_(int offset, int len, char const * rest, Token::Type type) {
    // check length is correct:
    if( tokenStrLen_ == offset + len ){
        // check string matches:
        if( memcmp(tokenStr_ + offset, rest, len) == 0 ){
            return type;
        }
    }
    // Not a keyword:
    return Token::IDENTIFIER;
}
