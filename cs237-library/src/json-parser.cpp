/*! \file json-parser.cpp
 *
 * Code for reading json files.
 *
 * CMSC 23700 Autumn 2022.
 *
 * \author John Reppy
 */

/*
 * This code is loosely based on that of the SimpleJSON Library (http://mjpa.in/json)
 * by Mike Anchor.
 *
 * COPYRIGHT (c) 2022 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "json.hpp"
#include <iostream>
#include <fstream>
#include <cctype>

namespace json {


class Input {
  public:
    Input (std::string filename);
    ~Input () { delete this->_buffer; }


    Input const &operator++ (int _unused) {
        if (this->_buffer[this->_i++] == '\n') this->_lnum++;
        return *this;
    }
    Input const &operator += (int n) { this->_i += n;  return *this; }
    const char *operator() () const { return &(this->_buffer[this->_i]); }
    char operator[] (int j) const { return this->_buffer[this->_i + j]; }
    char operator* () const { return this->_buffer[this->_i]; }
    int avail () const { return this->_len - this->_i; }
    bool eof () const { return this->_i >= this->_len; }

    void error (std::string msg)
    {
#ifndef NDEBUG
        std::cerr << "json::parseFile(" << this->_file << "): " << msg
            << " at line " << this->_lnum << std::endl;
        std::cerr << "    input = \"";
        int n = this->avail();
        if (20 < n) n = 20;
        for (int i = 0;  (i < 20);  i++) {
            if (isprint(this->_buffer[this->_i+i]))
                std::cerr << this->_buffer[this->_i+i];
            else
                std::cerr << ".";
        }
        std::cerr << " ...\n" << std::endl;
#endif
    }

  private:
    std::string _file;
    char        *_buffer;
    int         _i;     // character index
    int         _lnum;  // current line number
    int         _len;   // buffer size

};

Input::Input (std::string filename)
    : _file(filename), _buffer(nullptr), _i(0), _lnum(0), _len(0)
{
  // open the json file for reading
    std::ifstream inS(filename, std::ios::in);
    if (inS.fail())
        return;

  // figure out the size of the file
    inS.seekg (0, inS.end);
    int length = inS.tellg();
    inS.seekg (0, inS.beg);

  // read length bytes
    this->_lnum = 1;
    this->_buffer = new char[length];
    inS.read (this->_buffer, length);

    if (inS.fail()) {
        delete this->_buffer;
        this->_buffer = 0;
        return;
    }

    this->_len = length;
}

// forward decls
static bool skipWhitespace (Input &datap);
static Value *parse (Input &datap);

// parse a json file; this returns nullptr if there is a parsing error
Value *parseFile (std::string filename)
{
  // open the json file for reading
    Input datap(filename);
    if (datap.eof()) {
#ifndef NDEBUG
        std::cerr << "json::parseFile: unable to read \"" << filename << "\"" << std::endl;
#endif
        return nullptr;
    }

    if (! skipWhitespace (datap)) {
        return nullptr;
    }

    Value *value = parse (datap);

    return value;

}

static bool skipWhitespace (Input &datap)
{
    while ((! datap.eof()) && isspace(*datap))
        datap++;

    if (datap.eof()) {
        datap.error("unexpected eof");
        return false;
    }
    else
        return true;
}

static bool extractString (Input &datap, std::string &str)
{
    str = "";

    if (*datap != '\"')
        return false;
    datap++;

    while (! datap.eof()) {
        // Save the char so we can change it if need be
        char nextChar = *datap;

        // Escaping something?
        if (nextChar == '\\') {
            // Move over the escape char
            datap++;
            // Deal with the escaped char
            switch (*datap) {
                case '"': nextChar = '"'; break;
                case '\\': nextChar = '\\'; break;
                case '/': nextChar = '/'; break;
                case 'b': nextChar = '\b'; break;
                case 'f': nextChar = '\f'; break;
                case 'n': nextChar = '\n'; break;
                case 'r': nextChar = '\r'; break;
                case 't': nextChar = '\t'; break;
                case 'u': /* no UNICODE support */
                // By the spec, only the above cases are allowed
                default:
                    datap.error("invalid escape sequence in string");
                    return false;
            }
        }
      // End of the string?
        else if (nextChar == '"') {
            datap++;
            str.reserve(); // Remove unused capacity
            return true;
        }
      // Disallowed char?
        else if (! isprint(nextChar) && (nextChar != '\t')) {
          // SPEC Violation: Allow tabs due to real world cases
            datap.error("invalid character in string");
            return false;
        }
      // Add the next char
        str += nextChar;
      // Move on
        datap++;
    }

  // If we're here, the string ended incorrectly
    return false;
}

static int64_t parseInt (Input &datap)
{
    int64_t n = 0;
    while (*datap != 0 && isdigit(*datap)) {
        n = n * 10 + (*datap - '0');
        datap++;
    }

    return n;
}

static double parseDecimal (Input &datap)
{
    double decimal = 0.0;
    double factor = 0.1;

    while ((! datap.eof()) && isdigit(*datap)) {
        int digit = (*datap - '0');
        decimal = decimal + digit * factor;
        factor *= 0.1;
        datap++;
    }
    return decimal;
}

static Value *parse (Input &datap)
{
    if (datap.eof()) {
        datap.error("unexpected end of file");
        return nullptr;
    }

  // Is it a string?
    if (*datap == '"') {
        std::string str;
        if (! extractString(datap, str))
            return nullptr;
        else
            return new String(str);
    }
  // Is it a boolean?
    else if ((datap.avail() >= 4) && strncasecmp(datap(), "true", 4) == 0) {
        datap += 4;
        return new Bool(true);
    }
    else if ((datap.avail() >=  5) && strncasecmp(datap(), "false", 5) == 0) {
        datap += 5;
        return new Bool(false);
    }
  // Is it a null?
    else if ((datap.avail() >=  4) && strncasecmp(datap(), "null", 4) == 0) {
        datap += 4;
        return new Null();
    }
  // Is it a number?
    else if (*datap == '-' || isdigit(*datap)) {
      // Negative?
        bool neg = *datap == '-';
        bool isReal = false;
        if (neg) datap++;

        int64_t whole = 0;

      // parse the whole part of the number - only if it wasn't 0
        if (*datap == '0')
            datap++;
        else if (isdigit(*datap))
            whole = parseInt(datap);
        else {
            datap.error("invalid number");
            return nullptr;
        }

        double r;

      // Could be a decimal now...
        if (*datap == '.') {
            r = (double)whole;
            isReal = true;
            datap++;

            // Not get any digits?
            if (! isdigit(*datap)) {
                datap.error("invalid number");
                return nullptr;
            }

            // Find the decimal and sort the decimal place out
            // Use parseDecimal as parseInt won't work with decimals less than 0.1
            // thanks to Javier Abadia for the report & fix
            double decimal = parseDecimal(datap);

            // Save the number
            r += decimal;
        }

        // Could be an exponent now...
        if (*datap == 'E' || *datap == 'e') {
            if (!isReal) {
                r = (double)whole;
                isReal = true;
            }
            datap++;

            // Check signage of expo
            bool neg_expo = false;
            if (*datap == '-' || *datap == '+') {
                neg_expo = *datap == '-';
                datap++;
            }

            // Not get any digits?
            if (! isdigit(*datap)) {
                datap.error("invalid number");
                return nullptr;
            }

            // Sort the expo out
            double expo = parseInt(datap);
            for (double i = 0.0; i < expo; i++) {
                r = neg_expo ? (r / 10.0) : (r * 10.0);
            }
        }

        if (isReal) {
            return new Real (neg ? -r : r);
        }
        else {
            return new Integer (neg ? -whole : whole);
        }
    }
  // An object?
    else if (*datap == '{') {
        Object *object = new Object();

        datap++;

        while (!datap.eof()) {
          // Whitespace at the start?
            if (! skipWhitespace(datap)) {
                delete object;
                return nullptr;
            }

          // Special case: empty object
            if ((object->size() == 0) && (*datap == '}')) {
                datap++;
                return object;
            }

          // We want a string now...
            std::string name;
// CHECK: do we need to look for "?
            if (! extractString(datap, name)) {
                datap.error("expected label");
                delete object;
                return nullptr;
            }

          // More whitespace?
            if (! skipWhitespace(datap)) {
                delete object;
                return nullptr;
            }

          // Need a : now
            if (*datap != ':') {
                datap.error("expected ':'");
                delete object;
                return nullptr;
            }
            datap++;

          // More whitespace?
            if (! skipWhitespace(datap)) {
                delete object;
                return nullptr;
            }

          // The value is here
            Value *value = parse(datap);
            if (value == nullptr) {
                delete object;
                return nullptr;
            }

          // Add the name:value
            object->insert(name, value);

          // More whitespace?
            if (! skipWhitespace(datap)) {
                delete object;
                return nullptr;
            }

            // End of object?
            if (*datap == '}') {
                datap++;
                return object;
            }

            // Want a , now
            if (*datap != ',') {
                datap.error("expected ','");
                delete object;
                return nullptr;
            }

            datap++;
        }

      // Only here if we ran out of data
        datap.error("unexpected eof");
        delete object;
        return nullptr;
    }

    // An array?
    else if (*datap == '[') {
        Array *array = new Array();

        datap++;

        while (! datap.eof()) {
          // Whitespace at the start?
            if (! skipWhitespace(datap)) {
                delete array;
                return nullptr;
            }

          // Special case - empty array
            if ((array->length() == 0) && (*datap == ']')) {
                datap++;
                return array;
            }

          // Get the value
            Value *value = parse(datap);
            if (value == nullptr) {
                delete array;
                return nullptr;
            }

          // Add the value
            array->add(value);

          // More whitespace?
            if (! skipWhitespace(datap)) {
                delete array;
                return nullptr;
            }

          // End of array?
            if (*datap == ']') {
                datap++;
                return array;
            }

            // Want a , now
            if (*datap != ',') {
                datap.error("expected ','");
                delete array;
                return nullptr;
            }

            datap++;
        }

      // Only here if we ran out of data
        datap.error("unexpected eof");
        delete array;
        return nullptr;
    }
  // Ran out of possibilites, it's bad!
    else {
        datap.error("bogus input");
        return nullptr;
    }
}

} // namespace json
