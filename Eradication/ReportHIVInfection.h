
#pragma once

#include <sstream>
#include "BaseTextReport.h"

namespace Kernel {
    struct ISimulation;

    class ReportHIVInfection : public BaseTextReport
    {
        GET_SCHEMA_STATIC_WRAPPER(ReportHIVInfection)
    public:
        ReportHIVInfection( const ISimulation * sim = nullptr );
        static IReport* ReportHIVInfection::Create(const ISimulation * parent) { return new ReportHIVInfection( parent ); }

        virtual bool Configure( const Configuration* inputJson ) override;
        virtual bool IsCollectingIndividualData( float currentTime, float dt ) const ;
        virtual void LogIndividualData( IIndividualHuman* individual );

    protected:
        virtual std::string GetHeader() const ;

    private:
        const ISimulation * _parent;
        float startYear ; // Year to start collecting data
        float stopYear ;  // Year to stop  collecting data
    };

}

