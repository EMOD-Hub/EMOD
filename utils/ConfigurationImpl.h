
#pragma once

#include <string>
#include <list>
#include <vector>
#include <functional>
#include "ISupports.h"
#include "EnumSupport.h"

namespace Kernel
{
    namespace MetadataDescriptor 
    {
        struct Enum; // forward decl
    }

    struct ConfigurationImpl // overloaded functions wrapping config retrieval
    {
        static void get_config_value(const Configuration* config, std::string name, float *value)
        {
            *value = GET_CONFIG_NUMBER(config, name);
        }

        static void get_config_value(const Configuration* config, std::string name, std::string *value)
        {
            *value = GET_CONFIG_STRING(config, name);
        }

        static void get_config_value(const Configuration* config, std::string name, bool *value)
        {
            bool  v = GET_CONFIG_DOUBLE(config, name) != 0; // redundant conversion to get rid of inexplicable warning C4800 on msvc. 
            (*value) = v;
        }

        static void get_config_value(const Configuration* config, std::string name, int *value)
        {
            *value = GET_CONFIG_INTEGER(config, name);
        }

        static void get_config_value(const Configuration* config, std::string name, IConfigurable *value)
        {
            // TODO: error handling here?...could also pass back return codes
            // need to catch json lookup exceptions here and report errors
            value->Configure((const Configuration*)&((*config)[name]));
        }

        static void get_config_value(const Configuration* config, std::string name, Configuration ** pp_cached_config)
        {
            // TODO: error handling here?...could also pass back return codes
            // need to catch json lookup exceptions here and report errors

            *pp_cached_config = Configuration::CopyFromElement( (*config)[name], config->GetDataLocation() );
        }
    };

    class ModeConfigure { };
    class ModeGetSchema { };

    namespace MetadataDescriptor
    {
        struct Base
        {
        public:
            std::string name;
            std::string description;
            virtual Element GetSchemaElement() = 0;

        protected: // Don't want outsiders constructing this
            Base(std::string _name, std::string _desc) : name(_name), description(_desc) {}
        };

        struct Enum : public Base
        {
            // convenience macro to save typing of these args
#define MDD_ENUM_ARGS(enum_name) enum_name::pairs::count(), enum_name::pairs::get_keys(), enum_name::pairs::get_values()

            Enum(std::string _name, std::string _desc, int count, const char ** strings, const int *values) : Base(_name, _desc)
            {
                for (int k = 0; k < count; k++)
                {
                    enum_value_specs.push_back(pair<std::string,int>(strings[k], values[k]));
                }
            }

            typedef pair<std::string,int> enum_value_spec_t;
            std::vector<enum_value_spec_t> enum_value_specs;

            virtual const char *GetTypeString()
            {
                return "enum";
            }

            virtual Element GetSchemaElement()
            {
                Element member = Object();
                QuickBuilder qb(member);
                qb["type"] = json::String(GetTypeString());

                for (int k = 0; k < enum_value_specs.size(); k++)
                {
                    qb["enum"][k] = json::String(enum_value_specs[k].first);
                }

                qb["description"] = json::String(description);
                qb["default"] = json::String(enum_value_specs[0].first);
                return member;
            }
        };

        struct VectorOfEnum : public Enum
        {
            VectorOfEnum(std::string _name, std::string _desc, int count, const char ** strings, const int *values) :
                Enum(_name, _desc, count, strings, values)
            {
            }
            // for a list of enums, the default is emtpy list 
            virtual Element GetSchemaElement()
            {
                Element member = Object();
                QuickBuilder qb(member);
                qb["type"] = json::String(GetTypeString());

                for (int k = 0; k < enum_value_specs.size(); k++)
                {
                    qb["enum"][k] = json::String(enum_value_specs[k].first);
                }

                qb["description"] = json::String(description);
                qb["default"] = json::Array();
                return member;
            }

            virtual const char *GetTypeString()
            {
                return "Vector Enum";
            }
        };

        struct Bool : public Base
        {
            Bool(std::string _name, std::string _desc) : Base(_name, _desc) {}

            virtual Element GetSchemaElement()
            {
                Element member = Object();
                QuickBuilder qb(member);
                qb["name"] = json::String(name);
                qb["type"] = json::String("bool");
                qb["description"] = json::String(description);

                return member;
            }
        };

        struct String : public Base
        {
            String(std::string _name, std::string _desc) : Base(_name, _desc) {}

            virtual Element GetSchemaElement()
            {
                Element member = Object();
                QuickBuilder qb(member);
                qb["name"] = json::String(name);
                qb["type"] = json::String("string");
                qb["description"] = json::String(description);

                return member;
            }
        };

        struct Integer : public Base
        {
            Integer(std::string _name, std::string _desc) : Base(_name, _desc), has_min(false), has_max(false) {}
            Integer(std::string _name, std::string _desc, int _min, int _max) : Base(_name, _desc), m_min(_min), m_max(_max), has_min(true), has_max(true) {}

            bool has_min, has_max;
            int m_min, m_max;
            virtual Element GetSchemaElement()
            {
                Element member = Object();
                QuickBuilder qb(member);
                qb["name"] = json::String(name);
                qb["type"] = json::String("int");
                if (has_min)
                    qb["min"] = Number(m_min);
                if (has_max)
                    qb["max"] = Number(m_max);

                qb["description"] = json::String(description);

                return member;
            }
        };

        struct Real : public Base
        {
            Real(std::string _name, std::string _desc) : Base(_name, _desc), has_min(false), has_max(false) {}
            Real(std::string _name, std::string _desc, float _min, float _max) : Base(_name, _desc), m_min(_min), m_max(_max), has_min(true), has_max(true) {}

            bool has_min, has_max;
            float m_min, m_max;
            virtual Element GetSchemaElement()
            {
                Element member = Object();
                QuickBuilder qb(member);
                qb["name"] = json::String(name);
                qb["type"] = json::String("real");
                if (has_min) 
                    qb["min"] = Number(m_min);
                if (has_max)
                    qb["max"] = Number(m_max);

                qb["description"] = json::String(description);

                return member;
            }
        };

        struct Configuration : public Base
        {
            Configuration(std::string _name, std::string _desc) : Base(_name, _desc) {}

            virtual Element GetSchemaElement()
            {
                Element member = Object();
                QuickBuilder qb(member);
                qb["name"] = json::String(name);
                qb["type"] = json::String("configuration");
                qb["description"] = json::String(description);

                return member;
            }
        };

        struct Configurable : public Base
        {
            Configurable(std::string _name, std::string _desc, IConfigurable* _object) : Base(_name, _desc), object(_object) {}

            IConfigurable *object;
            virtual Element GetSchemaElement()
            {
                Element member = Object();
                QuickBuilder qb(member);
                qb["name"] = json::String(name);
                qb["type"] = json::String("object");
                qb["description"] = json::String(description);
#ifdef _WIN32
                qb["schema"] = object->GetSchema();
#else
#warning "line of code for GetSchema commented out to get to build on linux"
#endif
                return member;
            }
        };
    }

}
