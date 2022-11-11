/*! \file json.hpp
 *
 * Support code for loading and processing JSON files.
 *
 * \author John Reppy
 *
 */

/*
 * This code is loosely based on that of the SimpleJSON Library (http://mjpa.in/json)
 * by Mike Anchor.
 *
 * COPYRIGHT (c) 2015-2022 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _JSON_HPP_
#define _JSON_HPP_

#include <vector>
#include <string>
#include <map>

namespace json {

  //! the types of JSON values
    enum Type {
        T_OBJECT,       //!< object consisting of name-value pairs
        T_ARRAY,        //!< arrays of JSON values
        T_INTEGER,      //!< integer numbers (represented as int64_t)
        T_REAL,         //!< real numbers (represented as doubles)
        T_STRING,       //!< strings
        T_BOOL,         //!< booleans
        T_NULL          //!< the null value
    };

    class Value;
    class Object;
    class Array;
    class Number;
    class Integer;
    class Real;
    class String;
    class Bool;
    class Null;

  // parse a JSON file; this returns nullptr if there is a parsing error
    Value *parseFile (std::string filename);

  // virtual base class of JSON values
    class Value {
      public:

      //! return the type of this JSON value
        Type type() const { return this->_ty; }

      //! return true if this value is an object
        bool isObject() const { return (this->type() == T_OBJECT); }

      //! return true if this value is an array
        bool isArray() const { return (this->type() == T_ARRAY); }

      //! return true if this value is a number
        bool isNumber() const
        {
            return (this->type() == T_REAL) || (this->type() == T_INTEGER);
        }

      //! return true if this value is an integer
        bool isInteger() const { return (this->type() == T_INTEGER); }

      //! return true if this value is a real number
        bool isReal() const { return (this->type() == T_REAL); }

      //! return true if this value is a string
        bool isString() const { return (this->type() == T_STRING); }

      //! return true if this value is a boolean
        bool isBool() const { return (this->type() == T_BOOL); }

      //! return true if this value is the null value
        bool isNull() const { return (this->type() == T_NULL); }

      //! dynamic cast to JSON object
        const Object *asObject () const;

      //! dynamic cast to JSON array
        const Array *asArray () const;

      //! dynamic cast to JSON number
        const Number *asNumber () const;

      //! dynamic cast to JSON integer
        const Integer *asInteger () const;

      //! dynamic cast to JSON real number
        const Real *asReal () const;

      //! dynamic cast to JSON string
        const String *asString () const;

      //! dynamic cast to JSON boolean
        const Bool *asBool () const;

        virtual ~Value();
        virtual std::string toString() = 0;

      protected:

        explicit Value (Type ty) : _ty(ty) { };

        Type    _ty;
    };

    inline std::ostream& operator<< (std::ostream& s, Value *v)
    {
        return s << v->toString();
    }

  //! JSON objects
    class Object : public Value {
      public:
        Object () : Value(T_OBJECT), _value() { };
        ~Object ();

      //! return the number of fields in the object
        int size () const { return this->_value.size(); }

      //! insert a key-value pair into the object
        void insert (std::string key, Value *val);

      //! return the value corresponding to the given key.
      //! \returns nil if the key is not defined in the object
        Value *operator[] (std::string key) const;

      //! return an object-valued field
      //! \returns nullptr if the field is not present or is not an object
        const Object *fieldAsObject (std::string key) const
        {
            const Value *v = (*this)[key];
            return (v != nullptr) ? v->asObject() : nullptr;
        }

      //! return an array-valued field
      //! \returns nullptr if the field is not present or is not an array
        const Array *fieldAsArray (std::string key) const
        {
            const Value *v = (*this)[key];
            return (v != nullptr) ? v->asArray() : nullptr;
        }

      //! return a number-valued field
      //! \returns nullptr if the field is not present or is not a number
        const Number *fieldAsNumber (std::string key) const
        {
            const Value *v = (*this)[key];
            return (v != nullptr) ? v->asNumber() : nullptr;
        }

      //! return an integer-valued field
      //! \returns nullptr if the field is not present or is not an integer
        const Integer *fieldAsInteger (std::string key) const
        {
            const Value *v = (*this)[key];
            return (v != nullptr) ? v->asInteger() : nullptr;
        }

      //! return an real-valued field
      //! \returns nullptr if the field is not present or is not a real
        const Real *fieldAsReal (std::string key) const
        {
            const Value *v = (*this)[key];
            return (v != nullptr) ? v->asReal() : nullptr;
        }

      //! return an string-valued field
      //! \returns nullptr if the field is not present or is not a string
        const String *fieldAsString (std::string key) const
        {
            const Value *v = (*this)[key];
            return (v != nullptr) ? v->asString() : nullptr;
        }

      //! return an bool-valued field
      //! \returns nullptr if the field is not present or is not a bool
        const Bool *fieldAsBool (std::string key) const
        {
            const Value *v = (*this)[key];
            return (v != nullptr) ? v->asBool() : nullptr;
        }

        std::string toString();

      private:
        std::map<std::string, Value *> _value;
    };

  //! JSON arrays
    class Array : public Value {
      public:
        Array () : Value(T_ARRAY), _value() { };
        ~Array ();

        int length () const { return static_cast<int>(this->_value.size()); }
        void add (Value *v) { this->_value.push_back(v); }

        Value *operator[] (int idx) const { return this->_value[idx]; }

        std::string toString();

      private:
        std::vector<Value *> _value;
    };

  //! base class for JSON numbers
    class Number : public Value {
      public:
        virtual ~Number ();

        virtual std::string toString() = 0;
        virtual double realVal () const = 0;

      protected:
        explicit Number (Type ty) : Value(ty) { };

    };

    class Integer : public Number {
      public:
        Integer (int64_t v) : Number(T_INTEGER), _value(v) { }
        ~Integer ();

        int64_t intVal () const { return this->_value; }
        double realVal () const { return static_cast<double>(this->_value); }

        std::string toString();

      private:
        int64_t _value;
    };

    class Real : public Number {
      public:
        Real (double v) : Number(T_REAL), _value(v) { }
        ~Real ();

        double realVal () const { return this->_value; }

        std::string toString();

      private:
        double _value;
    };

    class String : public Value {
      public:
        String (std::string v) : Value(T_STRING), _value(v) { };
        ~String ();

        std::string value () const { return this->_value; }

        std::string toString();

      private:
        std::string _value;
    };

    class Bool : public Value {
      public:
        Bool (bool v) : Value(T_BOOL), _value(v) { };
        ~Bool ();

        bool value () const { return this->_value; }

        std::string toString();

      private:
        bool _value;
    };

    class Null : public Value {
      public:
        Null () : Value(T_NULL) { };
        ~Null ();

        std::string toString();

    };

} // namespace json

#endif // !_JSON_HPP_
