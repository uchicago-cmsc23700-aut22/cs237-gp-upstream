/*! \file json.pp
 *
 * Implementation of the classes that represent json values.
 *
 * CMSC 23700 Autumn 2022.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2022 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "json.hpp"

namespace json {

/***** class Value member functions *****/

Value::~Value () { }

const Object *Value::asObject () const
{
    return dynamic_cast<const Object *>(this);
}

const Array *Value::asArray () const
{
    return dynamic_cast<const Array *>(this);
}

const Number *Value::asNumber () const
{
    return dynamic_cast<const Number *>(this);
}

const Integer *Value::asInteger () const
{
    return dynamic_cast<const Integer *>(this);
}

const Real *Value::asReal () const
{
    return dynamic_cast<const Real *>(this);
}

const String *Value::asString () const
{
    return dynamic_cast<const String *>(this);
}

const Bool *Value::asBool () const
{
    return dynamic_cast<const Bool *>(this);
}

/***** class Object member functions *****/

Object::~Object ()
{
  // need to delete contents
}

void Object::insert (std::string key, Value *val)
{
    this->_value.insert (std::pair<std::string, Value *>(key, val));
}

Value *Object::operator[] (std::string key) const
{
    std::map<std::string, Value *>::const_iterator got = this->_value.find(key);
    if (got == this->_value.end())
        return nullptr;
    else
        return got->second;
}

std::string Object::toString() { return std::string("<object>"); }

/***** class Array member functions *****/

Array::~Array ()
{
  // need to delete contents
}

std::string Array::toString() { return std::string("<array>"); }

/***** class Number member functions *****/

Number::~Number () { }

/***** class Integer member functions *****/

Integer::~Integer () { }

std::string Integer::toString() { return std::string("<integer>"); }

/***** class Real member functions *****/

Real::~Real () { }

std::string Real::toString() { return std::string("<real>"); }

/***** class String member functions *****/

String::~String () { }

std::string String::toString () { return this->_value; }

/***** class Bool member functions *****/

Bool::~Bool () { }

std::string Bool::toString()
{
    if (this->_value) {
        return std::string("true");
    } else {
        return std::string("false");
    }
}

/***** class Null member functions *****/

Null::~Null () { }

std::string Null::toString() { return std::string("null"); }

} // namespace json
