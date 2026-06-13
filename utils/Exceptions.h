
#pragma once

#include <stdexcept>
#include <iostream>
#include <sstream>
#include "IdmApi.h"
#include "CajunIncludes.h"
#include <vector>

// This is an exception library for the DTK. We wanted to keep it relatively flat and 
// use-case driven, not theoretical. We have a DetailedException class that we use as our base that
// let's you pass information from the site of the exception. We want to move away from user-facing
// error message strings being passed in by the caller.
//
// Here's the Exceptions TOC with some clues about their usage:
// * BadEnumInSwitchStatementException [switch...default]
// * BadMapKeyException [ some_map.find( key ) == some_map.end() ]
// * CalculatedValueOutOfRangeException
// * ConfigurationRangeException (should become used only in JsonConfigurable as heterogeneous intra-node transmission is moved to initConfig)
// * DllLoadingException
// * FactoryCreateFromJsonException
// * FileIOException
// * FileNotFoundException
// * GeneralConfigurationException (try to use a more specific Configuration exception)
// * IllegalOperationException (try to use a more specific exception)
// * IncoherentConfigurationException
// * InitializationException (try to use a more specific exception; why did the initialization fail?)
// * InvalidInputDataException (sort of like a parsing exception, may create specific XXXParserExceptions)
// * MPIException (limited lifespan)
// * NotYetImplementedException (need to use more of these)
// * NullPointerException (really try to avoid using this; why is a pointer null?)
// * OutOfRangeException (consider whether should be ConfigurationRangeEx or CalculatedValueRangeEx; do we need an ArrayIndexOutOfBoundsEx?)
// * QueryInterfaceException
// * SerializationException
//

namespace Kernel {

    class IDMAPI std::runtime_error ;

    class IDMAPI DetailedException : public std::runtime_error
    {
    public:
        // WARNING: CODE IN HEADER FILE!!!
        DetailedException( const char * file_name, int line_num, const char * func_name );
        ~DetailedException() throw() {};
        // This used to return what(), but now it returns our customized string.
        // what() can always be called directly in case our carefully formed message
        // from parameters lost something really useful! Callers should not assume
        // _msg includes what().
        virtual const char * GetMsg() const;
        virtual const char * GetFilename() const;
        virtual int GetLineNumber() const;
        virtual const char* GetFunction() const;
        virtual const std::string& GetStackTrace() const ;

    protected:
        std::string _msg;
        std::string _stackTrace ;
        const char * _fileName;
        const char * _funcName;
        int _lineNum;
    };

    // This is to be used in the default section of most switch statements.
    class IDMAPI BadEnumInSwitchStatementException: public DetailedException
    {
    public:
        BadEnumInSwitchStatementException( const char* file_name, int line_num, const char* function_name, const char* var_name, int bad_value, const char* as_string );
    };

    // This is to be used when a map is searched for a key that is believed
    // to exist but is not found.
    class IDMAPI BadMapKeyException : public DetailedException
    {
    public:
        BadMapKeyException( const char* file_name, int line_num, const char* function_name, const char* var_name, const char* value );
    };

    // CalculatedValueOutOfRangeException is for variables with values that are the
    // result of mathematical operation that go outside some range.
    class IDMAPI CalculatedValueOutOfRangeException: public DetailedException
    {
    public:
        CalculatedValueOutOfRangeException( const char* file_name, int line_num, const char* function_name, const char* var_name, float bad_value, float range_violated );
    };

    // Similar to OutOfRangeException but specifically for when a value in the config.json has been detected
    // to fall outside the valid range.
    class IDMAPI ConfigurationRangeException : public DetailedException
    {
    public:
        ConfigurationRangeException( const char * file_name, int line_num, const char * function_name, const char* var_name, float var_value, float test_value );
    };

    // All failures for dll loading operations.
    class IDMAPI DllLoadingException : public DetailedException
    {
    public:
        DllLoadingException( const char * file_name, int line_num, const char * func_name, const char * msg );
    };

    // In various places we have class factories that instantiate classes from a json blob. 
    // If the json is valid json, but the data within it is problematic, the factory cannot
    // proceed.
    class IDMAPI FactoryCreateFromJsonException : public DetailedException
    {
    public:
        FactoryCreateFromJsonException( const char* file_name, int line_num, const char* function_name, const char* note );
    };

    // Any exception during file reading or writing that is not a FileNotFoundException 
    // and not an InvalidInputDataException.
    class IDMAPI FileIOException : public DetailedException
    {
    public:
        FileIOException( const char* sourccode_filename, int line_num, const char* function_name, const char* filename, const char* note = "" );
    };

    // Hopefully this is self-explanatory.
    class IDMAPI FileNotFoundException : public DetailedException
    {
    public:
        FileNotFoundException( const char * src_file_name, int line_num, const char* func_name, const char * missing_file_name );
    };

    // We have some specific ConfigurationExceptions below but this is for when there has been a configuration
    // error detected that doesn't fall into any of the other categories. Throwing an exception usually means
    // this is a non-recoverable configuration error. You should assume the program will exit.
    class IDMAPI GeneralConfigurationException : public DetailedException
    {
    public:
        GeneralConfigurationException( const char * file_name, int line_num, const char* func_name, const char * msg );
    };

    // An IllegalOperationException is appropriate for those code paths which should never be executed.
    // Often these are the silent unchecked "else"-es of an "if". Often it's better to add the 
    // "else throw new IllegalOperationException()".
    class IDMAPI IllegalOperationException : public DetailedException
    {
    public:
        IllegalOperationException( const char * file_name, int line_num, const char* func_name, const char * msg );
    };
    
    // It's perfectly possible to pass in values in the config that are mutually incompatible. This exception
    // is for when such a case is detected.
    class IDMAPI IncoherentConfigurationException : public DetailedException
    {
    public:
        IncoherentConfigurationException( const char * file_name, int line_num, const char* func_name, const char* existing_label, unsigned long existing_value, const char* test_label, unsigned long test_value, const char* details = "" );
        IncoherentConfigurationException( const char * file_name, int line_num, const char* func_name, const char* existing_label, signed int    existing_value, const char* test_label, signed int    test_value, const char* details = "" );
        IncoherentConfigurationException( const char * file_name, int line_num, const char* func_name, const char* existing_label, double        existing_value, const char* test_label, double        test_value, const char* details = "" ); 
        IncoherentConfigurationException( const char * file_name, int line_num, const char* func_name, const char* existing_label, float         existing_value, const char* test_label, float         test_value, const char* details = "" );
        IncoherentConfigurationException( const char * file_name, int line_num, const char* func_name, const char* existing_label, const char *  existing_value, const char* test_label, const char*   test_value, const char* details = "" );
        IncoherentConfigurationException( const char * file_name, int line_num, const char* func_name, const char* existing_label, signed int    existing_value, const char* test_label, const char*   test_value, const char* details = "" );

    protected:
        void IncoherentConfigurationException::createICEMessage(
            const char* existing_label,
            const char* existing_value,
            const char* test_label,
            const char* test_value,
            const char* details
        );
    };

    // Most initialization failures happen because of a bad configuration and so usually we will be able
    // to throw a Configuration related exception. But at some higher application levels, the relationship
    // of the configuration to the error is opaque and so this exception can be thrown.
    class IDMAPI InitializationException : public DetailedException
    {
    public:
        InitializationException( const char * file_name, int line_num, const char * func_name, const char * msg );
    };

    // This exception is to be used kind of like a parsing exception when data is being read in 
    // that violates the expected pattern and is hence unusable. 
    class IDMAPI InvalidInputDataException: public DetailedException
    {
    public:
        InvalidInputDataException( const char* file_name, int line_num, const char* function_name, const char* note );
        InvalidInputDataException( const char* file_name, int line_num, const char* function_name, const std::string & config_filename, const char* note = "" );
    };

    // This exception is thrown when a formatting error occurs in the demographics files.
    // This could be missing attributes or other rules.
    class IDMAPI NodeDemographicsFormatErrorException: public DetailedException
    {
    public:
        NodeDemographicsFormatErrorException( const char* file_name, int line_num, const char* function_name, const char* demographicsFilename, const char* note = "" );
    };

    // At time of creation, just one use of this and may be on the way out...
    class IDMAPI MPIException : public DetailedException
    {
    public:
        MPIException( const char* filename, int line_num, const char* function_name, const char* notes );
    };

    // Hopefully this is self-explanatory.
    class IDMAPI NotYetImplementedException : public DetailedException
    {
    public:
        NotYetImplementedException( const char * file_name, int line_num, const char* func_name, const char * msg );
    };

    // Usually NullPointerExceptions are thrown by the system not by the application. At the app
    // level a pointer is usually null because of an initialization failure, a file not being 
    // found, or something else that has its own exception. But I'm not willing to throw this 
    // away entirely because I'm pretty certain there will be cases where a null pointer will
    // be detectable at a level of code that has no knowledge as to why the pointer was null
    // (e.g., a function passed in a null pointer) and the correct solution may not be to return
    // false.
    class IDMAPI NullPointerException : public DetailedException
    {
    public:
        NullPointerException( const char * file_name, int line_num, const char * func_name, const char* var_name, const char* type_name );
    };

    // There is a std::out_of_range but it doesn't have the capability to provide a message,
    // file name and line number, so we provide our own. Use this exception when a value 
    // is outside a valid range. This a numeric, or boolean. If the value that's out of 
    // range is in a config param, use ConfigRangeException. A good example is an array index.
    // We have recently created ranged types (e.g., PercentType, NaturalNumber) that are 
    // inherently ranged, that can throw this exception. Also, there is a separate 
    // CalculatedValueOutOfRangeException which is for variables with values that are the
    // result of mathematical operation that go outside some range.
    class IDMAPI OutOfRangeException : public DetailedException
    {
    public:
        OutOfRangeException( const char * file_name, int line_num, const char* func_name, const char* var_name, float value, float value_violated );
    };

    // QueryInterfaceException is for anywhere that a variable is QI-ed for an interface
    // but returns false. This exception lets you report the variable, the interface queried
    // for, and the datatype of the variable.
    class IDMAPI QueryInterfaceException: public DetailedException
    {
    public:
        QueryInterfaceException( const char* file_name, int line_num, const char* function_name, const char* var_name, const char* queried_for_type, const char* variable_type );
    };

    // The SerializationException is used any time we get an unrecoverable failure during
    // serializing or deserializing.
    class IDMAPI SerializationException : public DetailedException
    {
    public:
        SerializationException( const char* filename, int line_num, const char* function_name, const char* notes );
    };

    class IDMAPI WarningException : public DetailedException
    {
    public:
        WarningException( const char* filename, int line_num, const char* function_name );
    };

    class IDMAPI MissingParameterFromConfigurationException : public DetailedException
    {
    public:
        MissingParameterFromConfigurationException( const char* filename, int line_num, const char* function_name, const char * config_filename, const char * param_name );
        MissingParameterFromConfigurationException( const char* filename, int line_num, const char* function_name, const char * config_filename, std::vector<std::string> param_name, const char * details = "" );
    };

    class IDMAPI JsonTypeConfigurationException : public DetailedException
    {
    public:
        JsonTypeConfigurationException( const char* filename, int line_num, const char* function_name, const char * param_name, const json::QuickInterpreter&, const char * caught_msg );
    };
}
