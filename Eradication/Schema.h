
#pragma once

#include <string>
#include <vector>
#include "CajunIncludes.h"

const std::vector<std::string> getSimTypeList();
const std::string getSupportedSimsString();

void writeInputSchemas( const char* output_path );

namespace json
{
    class SchemaUpdater : public Visitor
    {
        public:
            SchemaUpdater(Object& idmt_schema)
                : m_idmt_schema(idmt_schema)
            { }

        private:
            virtual void Visit(Array& array);
            virtual void Visit(Object& object);
            virtual void Visit(Number& number);
            virtual void Visit(Uint64& number);
            virtual void Visit(String& string);
            virtual void Visit(Boolean& boolean);
            virtual void Visit(Null& null);

            Object& m_idmt_schema;
    };
}
