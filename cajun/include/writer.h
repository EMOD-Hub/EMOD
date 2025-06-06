/**********************************************

License: BSD
Project Webpage: http://cajun-jsonapi.sourceforge.net/
Author: Terry Caton, tcaton(a)hotmail.com

TODO: additional documentation. 

***********************************************/

#pragma once

#include "elements.h"
#include "visitor.h"

namespace json
{
    class Writer : private ConstVisitor
    {
        public:
            static void Write(const Element& elementRoot, std::ostream& ostr,
                              const std::string& indentChars="    ",
                              bool add_endl=true, bool sort_keys=false);

        private:
            Writer(std::ostream& ostr, const std::string& indentChars,
                   bool add_endl, bool sort_keys)
                : m_ostr(ostr)
                , m_indentChars(indentChars)
                , m_nTabDepth(0)
                , m_add_endl(add_endl)
                , m_sort_keys(sort_keys)
            { }

        std::string MultiIndent(int nReps);

        virtual void Visit(const Array& array);
        virtual void Visit(const Object& object);
        virtual void Visit(const Number& number);
        virtual void Visit(const Uint64& number);
        virtual void Visit(const String& string);
        virtual void Visit(const Boolean& boolean);
        virtual void Visit(const Null& null);

        std::ostream& m_ostr;
        std::string m_indentChars;
        int m_nTabDepth;
        bool m_add_endl;
        bool m_sort_keys;
    };
}
