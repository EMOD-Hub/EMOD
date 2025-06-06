
#pragma once

#include <functional>
#include "FactorySupport.h"
#include "IReport.h"
#include "ObjectFactory.h"

namespace Kernel
{

    class ReportFactory : public ObjectFactory<IReport,ReportFactory>
    {
    public:
        std::vector<IReport*> Load( const std::string& rFilename );

    protected:
        template<class IObject, class Factory> friend class Kernel::ObjectFactory;

        ReportFactory();

        std::string m_ArrayElementName;
        bool        m_UseDefaults;
    };
}
