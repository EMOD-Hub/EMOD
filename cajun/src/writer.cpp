/**********************************************

License: BSD
Project Webpage: http://cajun-jsonapi.sourceforge.net/
Author: Terry Caton, tcaton(a)hotmail.com

TODO: additional documentation. 

***********************************************/

#include "stdafx.h"

#include "../include/writer.h"
#include <iostream>
#include <algorithm>

namespace json
{
    void Writer::Write(const Element& elementRoot, std::ostream& ostr,
                       const std::string& indentChars, bool add_endl,
                       bool sort_keys)
    {
        Writer writer(ostr, indentChars, add_endl, sort_keys);
        elementRoot.Accept(writer);
        ostr.flush(); // all done
    }

    std::string Writer::MultiIndent(int nReps)
    {
         std::string total_sep;
         for (int k1 = 0; k1 < nReps; k1++)
         {
             total_sep += m_indentChars;
         }

         return total_sep;
    }

    void Writer::Visit(const Array& array)
    {
        if (array.Empty())
        {
            m_ostr << "[]";
        }
        else
        {
            m_ostr << '[';
            if(m_add_endl)
            {
                m_ostr << std::endl;
            }
            ++m_nTabDepth;

            Array::const_iterator it(array.Begin());
            Array::const_iterator itEnd(array.End());
            while (it != itEnd)
            {
                m_ostr << MultiIndent(m_nTabDepth);
                it->Accept(*this); 

                if (++it != itEnd)
                {
                    m_ostr << ',';
                }
                if(m_add_endl)
                {
                    m_ostr << std::endl;
                }
            }

            --m_nTabDepth;
            m_ostr << MultiIndent(m_nTabDepth) << ']';
        }
    }

    void Writer::Visit(const Object& object)
    {
        if (object.Empty())
        {
            m_ostr << "{}";
        }
        else
        {
            m_ostr << '{';
            if(m_add_endl)
            {
                m_ostr << std::endl;
            }
            ++m_nTabDepth;

            if(!m_sort_keys)
            {
                Object::const_iterator it(object.Begin());
                Object::const_iterator itEnd(object.End());
                while (it != itEnd)
                {
                    m_ostr << MultiIndent(m_nTabDepth)
                           << '"' << it->name << "\": ";
                    it->element.Accept(*this); 

                    if (++it != itEnd)
                    {
                        m_ostr << ',';
                    }
                    if(m_add_endl)
                    {
                        m_ostr << std::endl;
                    }
                }
            }
            else
            {
                Object::const_iterator it(object.Begin());
                Object::const_iterator itEnd(object.End());
                std::vector<std::string> key_vec;
                while (it != itEnd)
                {
                    key_vec.push_back(it->name);
                    it++;
                }
                std::sort(key_vec.begin(), key_vec.end());

                std::vector<std::string>::iterator itv(key_vec.begin());
                std::vector<std::string>::iterator itvEnd(key_vec.end());
                while (itv != itvEnd)
                {
                    m_ostr << MultiIndent(m_nTabDepth)
                           << '"' << *itv << "\": ";

                    Object::const_iterator itSorted(object.Find(*itv));
                    itSorted->element.Accept(*this); 

                    if (++itv != itvEnd)
                    {
                        m_ostr << ',';
                    }
                    if(m_add_endl)
                    {
                        m_ostr << std::endl;
                    }
                }
            }

            --m_nTabDepth;
            m_ostr << MultiIndent(m_nTabDepth) << '}';
        }
    }

    void Writer::Visit(const Number& number)
    {
        m_ostr << number;
    }

    void Writer::Visit(const Uint64& number)
    {
        m_ostr << number;
    }

    void Writer::Visit(const Boolean& booleanElement)
    {
        m_ostr << (booleanElement ? "true" : "false");
    }

    void Writer::Visit(const Null& nullElement)
    {
        m_ostr << "null";
    }

    void Writer::Visit(const String& stringElement)
    {
        m_ostr << '"';

        const std::string& s = stringElement;

        std::string::const_iterator it(s.begin()), itEnd(s.end());

        for (; it != itEnd; ++it)
        {
            switch (*it)
            {
                case '"':         m_ostr << "\\\"";    break;
                case '\\':        m_ostr << "\\\\";    break;
                case '\b':        m_ostr << "\\b";     break;
                case '\f':        m_ostr << "\\f";     break;
                case '\n':        m_ostr << "\\n";     break;
                case '\r':        m_ostr << "\\r";     break;
                case '\t':        m_ostr << "\\t";     break;
                //case '\u':        m_ostr << "";        break;
                default:          m_ostr << *it;       break;
            }
        }

        m_ostr << '"';
    }
}
