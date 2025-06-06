/******************************************************************************

License: BSD
Project Webpage: http://cajun-jsonapi.sourceforge.net/
Author: Terry Caton, tcaton(a)hotmail.com

classes: QuickInterpreter
         QuickBuilder

The classes in this file provide an alternative interface for very quickly 
extracting or  compiling data into a JSON document structure. They are useful 
when the structure of a document is rigid and well-known, which is often the 
case. 

QuickInterpreter allows quick, read-only access to an existing document 
structure. For examples, given the document...

{
   "XYZ" : {
         "ABC" : [ 1 ]
   }
}

QuickInterpreter interpreter(elemRoot); // elemRoot containing above structure
const Object& object = interpreter["XYZ"];
const Array& array = interpreter["XYZ"]["ABC"];
const Number& num = interpreter["XYZ"]["ABC"][0];

QuickBuilder allows building the above structure with one line of code:

QuickBuilder builder(elemRoot); // elemRoot being an empty Element
builder["XYZ"][ABC][0] = Number(1);

******************************************************************************/

#pragma once

#include "elements.h"


namespace json
{


class QuickInterpreter
{
public:
   QuickInterpreter(const Element& element) :
      m_Element(element) {}

   virtual QuickInterpreter operator[] (const std::string& key) const {
      const Object& obj = As<Object>();
      return obj[key];
   }

   QuickInterpreter operator[] (int index) const {
      const Array& array = As<Array>();
      return array[index];
   }

   virtual bool Exist(const std::string& name) const {
       const Object& obj = As<Object>();
       return obj.Exist(name);
   }

   const Element& GetElement() const { return m_Element; }

   operator const Element& () const { return m_Element; }

   template <typename ElementTypeT>
   const ElementTypeT& As() const { return json_cast<const ElementTypeT&>(m_Element); }

private:
   const Element& m_Element;
};


class QuickBuilder
{
public:
   QuickBuilder(Element& element) :
      m_Element(element) {}

   QuickBuilder operator[] (const std::string& key) {
      Object& obj = Convert<Object>();
      return obj[key];
   }

   // how should we handle out of bounds? error? string [] implicitly 
   //  creates a new key/value. we'll implicitly resize
   QuickBuilder operator[] (size_t index) {
      Array& array = Convert<Array>();
      size_t nMinSize = index + 1; // zero indexed
      if (array.Size() < nMinSize)
         array.Resize(nMinSize);
      return array[index];
   }

   QuickBuilder& operator = (const QuickBuilder& builder) {
       m_Element = builder.m_Element;
       return *this;
   }

   QuickBuilder& operator = (const Element& element) {
      m_Element = element;
      return *this;
   }

   operator Element& () { return m_Element; }

   template <typename ElementTypeT>
   ElementTypeT& As() { return json_cast<ElementTypeT&>(m_Element); }

   template <typename ElementTypeT>
   ElementTypeT& Convert() {
      // we want an ElementTypeT. make it one if it's not.
      try {
         return As<ElementTypeT>();
      }
      catch (Exception&) {
         m_Element = ElementTypeT();
         return As<ElementTypeT>();
      }
   }

private:
   Element& m_Element;
};

}