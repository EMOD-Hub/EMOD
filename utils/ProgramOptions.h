
#pragma once

#include <string>
#include <vector>
#include <map>

#include "IdmApi.h"
#include "Debug.h"

// ProgramOptions is a utility for to specify and process command line arguments.
// It allows the developer to define a collection of command line arguments and then
// alleviates the developer's need to write his own parsing code.
//
// This class was designed to assert if an option is used inappropriately
// (i.e. asserts are used for the developer).  However, the ParseCommandLine() method
// will return an error message if the user enters something incorrectly.
// The object should not throw an exception.
class IDMAPI ProgramOptions
{
public:
    // Construct an object with the default or header message shown when the options are printed.
    ProgramOptions( const std::string& rDefaultMessage );
    ~ProgramOptions();

    // Present/Boolean Options
    // These are used when the developer wants an option that turns something on due
    // to the presence of the option.
    // rLongName    - This is the main name of the option and is the main way for the developer
    //                to refer to the option.
    // rShortName   - This is the short hand way for the user to specify the option.
    //                It is usually one letter.
    // rDescription - This is the description of the option that is displayed to the user
    //                when the options are printed.
    void AddOption( const std::string& rLongName,                                const std::string& rDescription );
    void AddOption( const std::string& rLongName, const std::string& rShortName, const std::string& rDescription );

    // Integer Options
    // These are used when the developer wants to allow the user to enter an integer value
    // for a particular option.  When this option is present on the command line, it is assumed
    // that a value will also be present.  However, if an 'implicit value' is defined for the
    // option, the user can specify just the option and the implicit value will be used.
    // rLongName     - This is the main name of the option and is the main way for the developer
    //                 to refer to the option.
    // defaultValue  - When the option is not present on the command line, the application will
    //                 use this default value.
    // implicitValue - If the option is defined with an 'implicitVal', the user can specify just
    //                 the option and the application will use the implicit value.
    // rDescription  - This is the description of the option that is displayed to the user
    //                 when the options are printed.
    void AddOptionWithValue( const std::string& rLongName, int defaultValue,                  const std::string& rDescription );
    void AddOptionWithValue( const std::string& rLongName, int defaultValue, int implicitVal, const std::string& rDescription );

    // String Options
    // These are used when the developer wants to allow the user to enter a string value
    // for a particular option.  When this option is present on the command line, it is assumed
    // that a value will be present.
    // rLongName     - This is the main name of the option and is the main way for the developer
    //                 to refer to the option.
    // rShortName    - This is the short hand way for the user to specify the option.
    //                 It is usually one letter.
    // rDefaultValue - When the option is not present on the command line, the application will
    //                 use this default value.
    // rDescription  - This is the description of the option that is displayed to the user
    //                 when the options are printed.
    void AddOptionWithValue( const std::string& rLongName,                                const std::string& rDefaultValue, const std::string& rDescription );
    void AddOptionWithValue( const std::string& rLongName, const std::string& rShortName, const std::string& rDefaultValue, const std::string& rDescription );

    // Possible String Values Option
    // This option is used when the developer has a predefined set of values that the option
    // is allowed to be.  If the user enters a value for this option that is not one of the
    // allowable values, then the parsing of the command line will fail.
    // rLongName       - This is the main name of the option and is the main way for the developer
    //                   to refer to the option.
    // rPossibleValues - The list of possible/allowable values for the option.  The first value
    //                   in the list is assumed to be the default value.
    // rDescription    - This is the description of the option that is displayed to the user
    //                   when the options are printed.
    void AddOptionWithValue( const std::string& rLongName, const std::vector<std::string>& rPossibleValues,  const std::string& rDescription );

    // Return true if the command line contained this option or if the option has
    // a default value.
    bool CommandLineHas( const std::string& rLongName );

    // Return the value of an option that has a string representation (i.e. not boolean or integer)
    std::string GetCommandLineValueString( const std::string& rLongName );

    // If the option has a default, return the default, else return empty string.
    std::string GetCommandLineValueDefaultString( const std::string& rLongName );

    // Return the value of an option that has an integer representation (i.e. has a defaultValue that is an integer)
    int GetCommandLineValueInt( const std::string& rLongName );

    // Parse the command line arguments and return an empty string if successful.
    // If the returned string is not empty, it contains an error message indicating
    // an issue that occurred parsing the line.  LongName options are entered with two
    // dashes ("--") while ShortName options are entered with one dash ("-").
    // A value for an option is entered with a space between the option name and the value
    // or an '=' sign.
    std::string ParseCommandLine( int argc, char *argv[] );

    // Print information about the options to the stream keeping the information to a maximum
    // of 80 characters per line.
    void Print( std::ostream& rOstream );

    // FOR TESTING - Allows the tester to reset the command line processing back
    // to what it was before a command line was parsed.
    void Reset();

private:
    // I want to use a forward declaration of this private struct and move these
    // structures to the CPP file,  but I can't find a good way to do it.

    // Option - Used for Present/Boolean options - Base class
    struct Option
    {
        std::string longName ;      // the long name of the option - the main way to refer to the option
        std::string shortName ;     // the short/single character the user can use to refer to the option
        std::string description ;   // printed info for the option
        bool        commandLineHas ;// true when the option was present on the command line or has a default value

        Option( const std::string& rLongName, 
                const std::string& rShortName, 
                const std::string& rDesc, 
                bool cmdLineHas = false )
        : longName( rLongName ), 
          shortName( rShortName ), 
          description( rDesc ), 
          commandLineHas( cmdLineHas )
        {
        };
        virtual ~Option(){};
    };

    // OptionInt - Used for Integer options
    struct OptionInt : public Option
    {
        int defaultValue ;      // value to use if the user doesn't change it on the command line
        int value ;             // value returned when requested
        bool hasImplicitValue ; // true if an implicit value is defined for the option
        int implicitValue ;     // value to use if the user provides the option string but no value

        OptionInt( const std::string& rLongName, int dv, const std::string& rDesc )
        : Option( rLongName, std::string(""), rDesc, true ),
          defaultValue( dv ),
          value( defaultValue ),
          hasImplicitValue( false ),
          implicitValue(0)
        {
        };
        virtual ~OptionInt(){};
    } ;

    // OptionString - Used for String options
    struct OptionString : public Option
    {
        std::string defaultValue ; // value to use if the user doesn't change it on the command line
        std::string value ;        // value returned when requested

        OptionString( const std::string& rLongName, 
                      const std::string& rShortName, 
                      const std::string& rDefaultValue, 
                      const std::string& rDesc )
        : Option( rLongName, rShortName, rDesc, true ),
          defaultValue( rDefaultValue ),
          value( defaultValue )
        {
        };
        virtual ~OptionString(){};
    } ;

    // OptionListString - Used when the option a predefined set of values
    struct OptionListString : public Option
    {
        std::vector<std::string> possibleValues ; // values that the option is allowed to be - first value is default
        std::string value ;                       // value returned when requested
         
        OptionListString( const std::string& rLongName, 
                          const std::vector<std::string>& rPossibleValues, 
                          const std::string& rDesc )
        : Option( rLongName, std::string(""), rDesc, true ),
          possibleValues( rPossibleValues ),
          value()
        {
            release_assert( rPossibleValues.size() > 0 );
            value = rPossibleValues[0] ;
        };
        virtual ~OptionListString(){};
    };

    Option* GetOption( const std::string& rLongName );
    bool ParseValue( int argc, char* argv[], int* pArgI, std::string* pValStr );
    std::string AddArgInfo( const Option* pOpt );
    std::vector<std::string> BreakDescriptionIntoChunks( const std::string& rDesc );

    std::string defaultMessage ;
    std::map<std::string,Option*> mapLongNameToOption ;
    std::map<std::string,Option*> mapShortNameToOption ;
    std::vector<std::string> insertionOrder ; // used to ensure the options are printed in the order they are added

    static const std::string SPACES_40 ;
};