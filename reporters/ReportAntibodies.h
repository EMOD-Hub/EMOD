/***************************************************************************************************

Copyright (c) 2021 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/


#pragma once

#include "BaseTextReport.h"
#include "ReportFactory.h"

namespace Kernel
{
    class ReportAntibodies: public BaseTextReport
    {
        DECLARE_FACTORY_REGISTERED( ReportFactory, ReportAntibodies, IReport )
    public:
        ReportAntibodies();
        virtual ~ReportAntibodies();

        // ISupports
        virtual QueryResult QueryInterface( iid_t iid, void** pinstance ) override;
        virtual int32_t AddRef() override { return BaseTextReport::AddRef(); }
        virtual int32_t Release() override { return BaseTextReport::Release(); }

        // BaseEventReport
        virtual bool Configure( const Configuration* ) override;

        virtual void Initialize( unsigned int nrmSize ) override;
        virtual std::string GetHeader() const override;
        virtual void UpdateEventRegistration( float currentTime, 
                                              float dt, 
                                              std::vector<INodeEventContext*>& rNodeEventContextList,
                                              ISimulationEventContext* pSimEventContext ) override;
        virtual bool IsCollectingIndividualData( float currentTime, float dt ) const override;
        virtual void LogIndividualData( IIndividualHuman* individual ) override;

    protected:
        ReportAntibodies( const std::string& rReportName );

        float m_StartDay;
        float m_EndDay;
        float m_ReportingInterval;
        bool  m_IsCapacityData;
        float m_NextDayToCollectData;
        bool  m_IsCollectingData;
    };
}
