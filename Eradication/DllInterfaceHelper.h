
#pragma once

#include "DllDefs.h"
#include "ProgVersion.h"
#include "Environment.h"
#include "RANDOM.h"
#include "IReport.h"
#include "FactorySupport.h"
#include "NoCrtWarnings.h"

#define DLL_LOG(lvl, x, ...)   do { if((EnvPtr !=nullptr) && (EnvPtr->Log !=nullptr) && EnvPtr->Log->CheckLogLevel(Logger::lvl, "DllInterfaceHelper"))  EnvPtr->Log->Log(Logger::lvl, "DllInterfaceHelper", x, ##__VA_ARGS__); } while(0)


namespace Kernel
{
    class DllInterfaceHelper
    {
    public:
        DllInterfaceHelper(const char* rTypeName, const char** simTypes,
                            instantiator_function_t rif = nullptr )
            : m_TypeName(rTypeName)
            , m_SupportedSimTypes(simTypes)
            , m_ReportInstantiatorFunc( rif )
        { };

        char* GetEModuleVersion( char* sVer, const Environment* pEnv )
        {            
            // The sharedlib needs to set its copy of the Environment to the one from the exe passed in to the first function called.
            // But it really seems like this should be in the instantiator, not these Getter functions.
            Environment::setInstance(const_cast<Environment*>(pEnv));
            ProgDllVersion pv;
            DLL_LOG( INFO, "%s: Version=%s  Branch=%s  SccsDate=%s  BuilderName=%s  BuildDate=%s\n",
                     m_TypeName, 
                     pv.getVersion(),
                     pv.getSccsBranch(),
                     pv.getSccsDate(),
                     pv.getBuilderName(),
                     pv.getBuildDate() );
            fflush(stdout);

            if (sVer)
            {
                int length = strlen(pv.getVersion()) + 1 ;
                strcpy_s(sVer, length, pv.getVersion());
            }
            return sVer;
        };

        void GetSupportedSimTypes( char* simTypes[] )
        {
            int i=0;
            while (m_SupportedSimTypes[i] != NULL && i < SIMTYPES_MAXNUM )
            {
                // allocation will be freed by the caller
                int length = strlen(m_SupportedSimTypes[i]) + 1 ;
                simTypes[i] = new char[length];
                strcpy_s(simTypes[i], length, m_SupportedSimTypes[i]);
                i++;
            }
            simTypes[i] = NULL;
        };

        const char* GetType()
        {
            DLL_LOG( INFO, "GetType called for %s\n", m_TypeName );
            return m_TypeName;
        };

        void GetReportInstantiator( Kernel::instantiator_function_t* pif )
        {
            *pif = m_ReportInstantiatorFunc ;
        };

    private:
        const char*   m_TypeName;
        const char**  m_SupportedSimTypes;
        instantiator_function_t m_ReportInstantiatorFunc ;
    };

}
