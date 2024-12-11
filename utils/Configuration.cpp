
#include "stdafx.h"

#include "Environment.h"
#include "Configuration.h"
#include "ConfigurationImpl.h"
#include "Sugar.h"
#include "CajunIncludes.h"
#include "Exceptions.h"
#include "Configure.h"
#include "FileSystem.h"
#ifdef EMBEDDED_PYTHON_DEMO
#include "Python.h"
#endif

#include "Serializer.h"

SETUP_LOGGING( "Configuration" )

//////////////////////////////////////////////////////////////////////////
// Implementation of basic Configuration wrapper

using namespace json;
using namespace std;

void JsonUtility::logJsonException( const json::ParseException &pe, std::string& err_msg )
{
    std::ostringstream errMsgStream;
    errMsgStream << "Json Parse error at: " << endl;
    errMsgStream << "Doc " <<pe.m_locTokenBegin.m_nDocOffset << "," << endl;
    errMsgStream << "Line " << pe.m_locTokenBegin.m_nLine << "," << endl;
    errMsgStream << "Char " << pe.m_locTokenBegin.m_nLineOffset << "," << endl;
    errMsgStream << pe.what() << endl;
    err_msg = errMsgStream.str();
}

void JsonUtility::logJsonException( const json::ScanException &se, std::string& err_msg )
{
    std::ostringstream errMsgStream;
    errMsgStream << "Json Scan error at: " << endl;
    errMsgStream << "Doc " <<se.m_locError.m_nDocOffset << "," << endl;
    errMsgStream << "Line " << se.m_locError.m_nLine << "," << endl;
    errMsgStream << "Char " << se.m_locError.m_nLineOffset<< "," << endl;
    errMsgStream << se.what() << endl;
    err_msg = errMsgStream.str();
}

Configuration::Configuration()
    : QuickInterpreter( json::Element() )
    , pElement( nullptr )
    , data_location("serialization")
{
    // this ctor only for serialization, the base class will be initialized to an invalid state which deserialization must repair
}

Configuration::Configuration( json::Element* element, const std::string& rDataLocation ) 
    : QuickInterpreter( *element )
    , pElement( element )
    , data_location( rDataLocation )
    , extendedConfig{}
{
}

void Configuration::Add(const std::string& elementName, int value)
{
    json::Number number(value);
    extendedConfig.insert(std::pair<std::string, json::Number>(elementName, number));
}

bool Configuration::Exist(const std::string& name) const{
    if (extendedConfig.count(name))
        return true;
    return QuickInterpreter::Exist(name);
}

QuickInterpreter Configuration::operator[] (const std::string& key) const {
    if (extendedConfig.count(key))
    {
        return extendedConfig.at(key);
    }
    return QuickInterpreter::operator[](key);
}

bool Configuration::CheckElementByName(const std::string& elementName) const
{
    return QuickInterpreter::Exist(elementName);
}

Configuration* Configuration::LoadFromPython(
    const std::string& filename
)
{
#ifdef EMBEDDED_PYTHON_DEMO
    PyObject * pName, *pModule, *pDict, *pFunc, *pValue;

    //std::cout << "Invoking python (hopefully)!" << std::endl;
    Py_Initialize();
    PyErr_Print();
    pName = PyUnicode_FromString( "emod_config" ); // need to have script emod_config.py in site-packages (or other path visible to python)
    PyErr_Print();
    assert( pName );
    pModule = PyImport_Import( pName );
    PyErr_Print();
    pDict = PyModule_GetDict( pModule );
    PyErr_Print();
    pFunc = PyDict_GetItemString( pDict, "doSomething" ); // need function in script called doSomething
    PyErr_Print();
    PyObject * param_str = PyString_FromString( filename.c_str() );
    PyObject * vars = PyTuple_New( 1 );
    PyTuple_SetItem( vars, 0, param_str );
    pValue = PyObject_CallObject( pFunc, vars );
    PyErr_Print();

    //std::cout << PyString_AsString(pValue) << std::endl;
    std::istringstream testJsonFromPy( PyString_AsString(pValue) );
    
    Py_DECREF( pValue );

    using namespace json;

    Element* elemRootFile = _new_ String(); // this object becomes owned by the configuration

    Reader::Read(*elemRootFile, testJsonFromPy);
    
    Configuration* conf = _new_ Configuration(elemRootFile); 

    return conf;
#else
    throw Kernel::IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Need to enable EMBEDDED_PYTHON_DEMO in compiler." );
    return nullptr;
#endif
}

Configuration* Configuration::Load( string configFileName )
{
#ifdef EMBEDDED_PYTHON_DEMO
    if( getenv( "LOAD_FROM_PYTHON" ) )
    {
        return LoadFromPython( configFileName );
    }
    else
#endif
    {
        //using namespace json;
        //using namespace std;
        // open config file more gingerly for debugging on temperamental network file systems
        std::ifstream ifs_configfile;
        FileSystem::OpenFileForReading( ifs_configfile, configFileName.c_str() );

        Configuration* p_config = Load( ifs_configfile, configFileName );
        ifs_configfile.close();
        return p_config ;
    }
}

Configuration* Configuration::Load( std::istream& rInputStream, const std::string& rDataLocation )
{
    Configuration *conf = nullptr;
    std::ostringstream final_msg;
    try 
    {
        conf = loadInternal( rInputStream, rDataLocation );
    }
    catch (json::ParseException &pe)
    {
        std::string err_msg;
        JsonUtility::logJsonException(pe, err_msg);
        final_msg << "Caught the following json::ParseException while attempting to read data from: " << rDataLocation << ". " << err_msg;
        throw Kernel::GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, final_msg.str().c_str() );
    }
    catch (json::ScanException &se)
    {
        std::string err_msg;
        JsonUtility::logJsonException(se, err_msg);
        final_msg << "Caught the following json::ScanException while attempting to read data from: " << rDataLocation << ". " << err_msg;
        throw Kernel::GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, final_msg.str().c_str() );
    }
    catch( const json::Exception &e )
    {
        std::string raw_msg = std::string( e.what() );
        if( raw_msg.find( "Object name not found" ) != std::string::npos )
        {
            unsigned pos = raw_msg.find( ":" );
            std::string param_name = raw_msg.substr( pos+2 );
            throw Kernel::MissingParameterFromConfigurationException( __FILE__, __LINE__, __FUNCTION__, rDataLocation.c_str(), param_name.c_str() );
        }
        else
        {
            final_msg << "Caught the following json::Exception while attempting to read data from: " << rDataLocation << ". " << e.what();
            throw Kernel::GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, final_msg.str().c_str() );
        }
    }
    return conf;
}

Configuration *Configuration_Load( const std::string& rFilename )
{
    return Configuration::Load( rFilename );
}


/*
Configuration* Configuration::Load( istream &config_file )
{    
    return loadInternal(config_file);
}

Configuration* Configuration::loadInternalWrapper( std::istream &config_file )
{
    return loadInternal(config_file);
}
*/


Configuration* Configuration::loadInternal( istream &is_config_file, const std::string& rDataLocation )
{
    using namespace json;

    //Object objRoot;
    Element* elemRootFile = _new_ String(); // this object becomes owned by the configuration

    Reader::Read(*elemRootFile, is_config_file);

    // this constructor and base class constructor have no logic, so we shouldn't have any exceptions
    // we would need to clean up after

    Configuration* conf = _new_ Configuration( elemRootFile, rDataLocation );
    // (we keep a handle to the elemRootFile object in the instance object and clean it up when we're done)

    return conf;
}

Configuration* Configuration::CopyFromElement( const json::Element &elem, const std::string& rDataLocation )
{
    Element *elem_copy = _new_ Element(elem);
    return _new_ Configuration( elem_copy, rDataLocation );
}

#if 0
template<class Archive>
void Configuration::serialize( Archive &ar, const unsigned int v )
{
    // hacked serialization....converts to string then serializes that, rather than trying to do an efficient serialization of the cajun objects themselves

    if (typename Archive::is_loading())
    {
        string data;
        ar & data;

        istringstream iss(data, istringstream::in);

        Element *elem = _new_ String();
        Reader::Read(*elem, iss);

        pElement = elem;
#ifdef WIN32
        this->QuickInterpreter::QuickInterpreter(*pElement); // reinit base class
#else
#warning "code commented out to get linux build to work"
#endif

    }
    else
    {
        ostringstream oss(ostringstream::out);
        Writer::Write(*pElement, oss);
        // Trying 2-step for linux
        std::string stringToSerialize = oss.str();
        ar & stringToSerialize;
    }
}
#endif

/////////////////////////////////////////////////////////////////////////////////////////
// config/json loading wrappers

vector<int> GET_CONFIG_VECTOR_INT(const QuickInterpreter* parameter_source, const char *name)
{
    vector<int> values;

    if(parameter_source == nullptr)
    {
        if( Kernel::JsonConfigurable::_dryrun )
        {
            return values;
        }
        else
        {
            throw std::runtime_error("Null pointer!  Invalid config passed for parsing");
        }
    }
    try
    {
        json::QuickInterpreter json_array( (*parameter_source)[name].As<json::Array>() );
        for( unsigned int idx = 0; idx < (*parameter_source)[name].As<json::Array>().Size(); idx++ )
        {
            double jsonValueAsDouble = json_array[idx].As<json::Number>();
            int value = ConvertIntegerValue<int>( name, jsonValueAsDouble );
            values.push_back(value);
        }
    }
    catch( json::Exception )
    {
        if( Kernel::JsonConfigurable::_dryrun )
        {
            return values;
        }
        else
        {
            throw Kernel::JsonTypeConfigurationException( __FILE__, __LINE__, __FUNCTION__, name, (*parameter_source), "Expected INT VECTOR/ARRAY" );
        }
    }

    return values;
}

vector<float> GET_CONFIG_VECTOR_FLOAT(const QuickInterpreter* parameter_source, const char *name)
{
    vector<float> values;

    if(parameter_source == nullptr)
    {
        if( Kernel::JsonConfigurable::_dryrun )
        {
            return values;
        }
        else
        {
            throw std::runtime_error("Null pointer!  Invalid config passed for parsing");
        }
    }
    try
    {
        json::QuickInterpreter json_array( (*parameter_source)[name].As<json::Array>() );
        for( unsigned int idx = 0; idx < (*parameter_source)[name].As<json::Array>().Size(); idx++ )
        {
            float value = (float)json_array[idx].As<json::Number>();
            values.push_back(value);
        }
    }
    catch( json::Exception )
    {
        if( Kernel::JsonConfigurable::_dryrun )
        {
            return values;
        }
        else
        {
            throw Kernel::JsonTypeConfigurationException( __FILE__, __LINE__, __FUNCTION__, name, (*parameter_source), "Expected FLOAT VECTOR/ARRAY" );
        }
    }

    return values;
}

vector<vector<float>> GET_CONFIG_VECTOR2D_FLOAT(const QuickInterpreter* parameter_source, const char *name)
{
    vector<vector<float>> matrix;

    if(parameter_source == nullptr)
    {
        if( Kernel::JsonConfigurable::_dryrun )
        {
            return matrix;
        }
        else
        {
            throw std::runtime_error("Null pointer!  Invalid config passed for parsing");
        }
    }
    try
    {
        unsigned int num_elements_x = (*parameter_source)[name].As<json::Array>().Size();

        json::QuickInterpreter json_array_of_arrays( (*parameter_source)[name].As<json::Array>() );
        for( unsigned int idx = 0; idx < num_elements_x; idx++ )
        {
            json::QuickInterpreter json_array( json_array_of_arrays[idx].As<json::Array>() );

            unsigned int num_elements_y = json_array_of_arrays[idx].As<json::Array>().Size() ;
            std::vector<float> values;

            for( unsigned int idy = 0; idy < num_elements_y; idy++ )
            {
                float value = (float)json_array[idy].As<json::Number>();
                values.push_back(value);
            }
            matrix.push_back( values );
        }
    }
    catch( json::Exception )
    {
        if( Kernel::JsonConfigurable::_dryrun )
        {
            return matrix;
        }
        else
        {
            throw Kernel::JsonTypeConfigurationException( __FILE__, __LINE__, __FUNCTION__, name, (*parameter_source), "Expected FLOAT 2D VECTOR/ARRAY" );
        }
    }

    return matrix;
}

vector<vector<vector<float>>> GET_CONFIG_VECTOR3D_FLOAT(const QuickInterpreter* parameter_source, const char *name)
{
    vector<vector<vector<float>>> matrix;

    if (parameter_source == nullptr)
    {
        if (Kernel::JsonConfigurable::_dryrun)
        {
            return matrix;
        }
        else
        {
            throw std::runtime_error("Null pointer!  Invalid config passed for parsing");
        }
    }
    try
    {
        unsigned int num_elements_x = (*parameter_source)[name].As<json::Array>().Size();

        json::QuickInterpreter json_array_of_array_of_arrays((*parameter_source)[name].As<json::Array>());
        for (unsigned int idx = 0; idx < num_elements_x; idx++)
        {
            unsigned int num_elements_y = json_array_of_array_of_arrays[idx].As<json::Array>().Size();
            
            json::QuickInterpreter json_array_of_arrays(json_array_of_array_of_arrays[idx].As<json::Array>());

            std::vector<vector<float>> values;

            for (unsigned int idy = 0; idy < num_elements_y; idy++)
            {
                unsigned int num_elements_z = json_array_of_arrays[idy].As<json::Array>().Size();
                
                json::QuickInterpreter json_array(json_array_of_arrays[idy].As<json::Array>());

                std::vector<float> values_inner;

                for (unsigned int idz = 0; idz < num_elements_z; idz++)
                {
                    float value = (float)json_array[idz].As<json::Number>();
                    values_inner.push_back(value);
                }
                values.push_back(values_inner);
            }
            matrix.push_back(values);
        }
    }
    catch (json::Exception)
    {
        if (Kernel::JsonConfigurable::_dryrun)
        {
            return matrix;
        }
        else
        {
            throw Kernel::JsonTypeConfigurationException(__FILE__, __LINE__, __FUNCTION__, name, (*parameter_source), "Expected FLOAT 3D VECTOR/ARRAY");
        }
    }

    return matrix;
}

vector<vector<int>> GET_CONFIG_VECTOR2D_INT(const QuickInterpreter* parameter_source, const char *name)
{
    vector<vector<int>> matrix;

    if(parameter_source == nullptr)
    {
        if( Kernel::JsonConfigurable::_dryrun )
        {
            return matrix;
        }
        else
        {
            throw std::runtime_error("Null pointer!  Invalid config passed for parsing");
        }
    }
    try
    {
        unsigned int num_elements_x = (*parameter_source)[name].As<json::Array>().Size();

        json::QuickInterpreter json_array_of_arrays( (*parameter_source)[name].As<json::Array>() );
        for( unsigned int idx = 0; idx < num_elements_x; idx++ )
        {
            json::QuickInterpreter json_array( json_array_of_arrays[idx].As<json::Array>() );

            unsigned int num_elements_y = json_array_of_arrays[idx].As<json::Array>().Size() ;
            std::vector<int> values;

            for( unsigned int idy = 0; idy < num_elements_y; idy++ )
            {
                double jsonValueAsDouble = json_array[idy].As<json::Number>();
                int value = ConvertIntegerValue<int>( name, jsonValueAsDouble );
                values.push_back(value);
            }
            matrix.push_back( values );
        }
    }
    catch( json::Exception )
    {
        if( Kernel::JsonConfigurable::_dryrun )
        {
            return matrix;
        }
        else
        {
            throw Kernel::JsonTypeConfigurationException( __FILE__, __LINE__, __FUNCTION__, name, (*parameter_source), "Expected STRING VECTOR/ARRAY" );
        }
    }

    return matrix;
}

vector<vector<vector<string>>> GET_CONFIG_VECTOR3D_STRING(const QuickInterpreter* parameter_source, const char *name)
{
    vector<vector<vector<string>>> matrix;

    if(parameter_source == NULL)
    {
        if( Kernel::JsonConfigurable::_dryrun )
        {
            return matrix;
        }
        else
        {
            throw std::runtime_error("Null pointer!  Invalid config passed for parsing");
        }
    }
    try
    {
        unsigned int num_elements_x = (*parameter_source)[name].As<json::Array>().Size();

        json::QuickInterpreter json_3d_array( (*parameter_source)[name].As<json::Array>() );
        for( unsigned int idx = 0; idx < num_elements_x; idx++ )
        {
            json::QuickInterpreter json_2d_array( json_3d_array[idx].As<json::Array>() );

            unsigned int num_elements_y = json_3d_array[idx].As<json::Array>().Size() ;
            std::vector<std::vector<string>> values_2d;

            for( unsigned int idy = 0; idy < num_elements_y; idy++ )
            {
                json::QuickInterpreter json_array( json_2d_array[ idy ].As<json::Array>() );

                unsigned int num_elements_z = json_2d_array[ idy ].As<json::Array>().Size();
                std::vector<string> values;

                for( unsigned int idz = 0; idz < num_elements_z; idz++ )
                {
                    string value = (string)json_array[ idz ].As<json::String>();
                    values.push_back( value );
                }
                values_2d.push_back( values );
            }
            matrix.push_back( values_2d );
        }
    }
    catch (json::Exception& e)
    {
        string full_description(e.what());
        if (strcmp(e.what(), "Bad json_cast") == 0)
        {
            full_description += ": ";
            full_description += name;
            full_description += " (expected JSON array of string)";
        }

        if( Kernel::JsonConfigurable::_dryrun )
        {
            return matrix;
        }
        else
        {
            throw json::Exception(full_description);
        }
    }

    return matrix;
}

vector<vector<string>> GET_CONFIG_VECTOR2D_STRING(const QuickInterpreter* parameter_source, const char *name)
{
    vector<vector<string>> matrix;

    if(parameter_source == NULL)
    {
        if( Kernel::JsonConfigurable::_dryrun )
        {
            return matrix;
        }
        else
        {
            throw std::runtime_error("Null pointer!  Invalid config passed for parsing");
        }
    }
    try
    {
        unsigned int num_elements_x = (*parameter_source)[name].As<json::Array>().Size();

        json::QuickInterpreter json_array_of_arrays( (*parameter_source)[name].As<json::Array>() );
        for( unsigned int idx = 0; idx < num_elements_x; idx++ )
        {
            json::QuickInterpreter json_array( json_array_of_arrays[idx].As<json::Array>() );

            unsigned int num_elements_y = json_array_of_arrays[idx].As<json::Array>().Size() ;
            std::vector<string> values;

            for( unsigned int idy = 0; idy < num_elements_y; idy++ )
            {
                string value = (string)json_array[idy].As<json::String>();
                values.push_back(value);
            }
            matrix.push_back( values );
        }
    }
    catch (json::Exception& e)
    {
        string full_description(e.what());
        if (strcmp(e.what(), "Bad json_cast") == 0)
        {
            full_description += ": ";
            full_description += name;
            full_description += " (expected JSON array of string)";
        }

        if( Kernel::JsonConfigurable::_dryrun )
        {
            return matrix;
        }
        else
        {
            throw json::Exception(full_description);
        }
    }

    return matrix;
}

vector<string> GET_CONFIG_VECTOR_STRING(const QuickInterpreter* parameter_source, const char *name)
{
    vector<string> values;

    if(parameter_source == nullptr)
    {
        //if( getenv( "DRYRUN" ) )
        if( Kernel::JsonConfigurable::_dryrun )
        {
            return values;
        }
        else
        {
            throw std::runtime_error("Null pointer!  Invalid config passed for parsing");
        }
    }
    try
    {
        json::QuickInterpreter json_array( (*parameter_source)[name].As<json::Array>() );
        for( unsigned int idx = 0; idx < (*parameter_source)[name].As<json::Array>().Size(); idx++ )
        {
            string value = (string)json_array[idx].As<json::String>();
            values.push_back(value);
        }
    }
    catch( json::Exception )
    {
        if( Kernel::JsonConfigurable::_dryrun )
        {
            return values;
        }
        else
        {
            throw Kernel::JsonTypeConfigurationException( __FILE__, __LINE__, __FUNCTION__, name, (*parameter_source), "Expected STRING VECTOR/ARRAY" );
        }
    }

    return values;
}

set<string> GET_CONFIG_STRING_SET(const QuickInterpreter* parameter_source, const char *name)
{
    set<string> values;

    if(parameter_source == nullptr)
    {
        //if( getenv( "DRYRUN" ) )
        if( Kernel::JsonConfigurable::_dryrun )
        {
            return values;
        }
        else
        {
            throw std::runtime_error("Null pointer!  Invalid config passed for parsing");
        }
    }
    try
    {
        json::QuickInterpreter json_array( (*parameter_source)[name].As<json::Array>() );
        for( unsigned int idx = 0; idx < (*parameter_source)[name].As<json::Array>().Size(); idx++ )
        {
            string value = (string)json_array[idx].As<json::String>();
            values.insert(value);
        }
    }
    catch( json::Exception )
    {
        if( Kernel::JsonConfigurable::_dryrun )
        {
            return values;
        }
        else
        {
            throw Kernel::JsonTypeConfigurationException( __FILE__, __LINE__, __FUNCTION__, name, (*parameter_source), "Expected STRING SET" );
        }
    }

    return values;
}


string GET_CONFIG_STRING(const QuickInterpreter* parameter_source, const char *name)
{
    string value = "";

    if(parameter_source == nullptr)
    {
        //if( getenv( "DRYRUN" ) )
        if( Kernel::JsonConfigurable::_dryrun )
        {
            return value;
        }
        else
        {
            throw std::runtime_error("Null pointer!  Invalid config passed for parsing");
        }
    }

    try {
        value = (string)((*parameter_source)[name].As<json::String>());
    }
    catch( json::Exception )
    {
        if( Kernel::JsonConfigurable::_dryrun )
        {
            return value;
        }
        else
        {
            throw Kernel::JsonTypeConfigurationException( __FILE__, __LINE__, __FUNCTION__, name, (*parameter_source), "Expected STRING" );
        }
    }

    return value;
}


double GET_CONFIG_DOUBLE(
    const QuickInterpreter* parameter_source,
    const char *name,
    int min
)
{
    double value = 0.0;

    if(parameter_source == nullptr)
    {
        //if( getenv( "DRYRUN" ) )
        if( Kernel::JsonConfigurable::_dryrun )
        {
            return 0.0f;
        }
        else
        {
            throw std::runtime_error("Null pointer!  Invalid config passed for parsing");
        }
    }
    try {
        value = (*parameter_source)[name].As<json::Number>();
    }
    catch( json::Exception )
    {
        if( Kernel::JsonConfigurable::_dryrun )
        {
            return 0.0f;
        }
        else
        {
            throw Kernel::JsonTypeConfigurationException( __FILE__, __LINE__, __FUNCTION__, name, (*parameter_source), "Expected FLOAT" );
        }
    }

    return value;
}


/*bool GET_CONFIG_BOOLEAN(const json::QuickInterpreter* parameter_source, const char *name)
{
    double d = GET_CONFIG_DOUBLE(parameter_source, name);

    if(d != 0.0 && d != 1.0)
        throw std::runtime_error(("Non-boolean value found for config parameter: " + std::string(name)).c_str());

    return (d == 1.0);
}*/
