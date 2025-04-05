
#include "stdafx.h"
#include <math.h> // for std::floor
#include "Exceptions.h"
#include "HIVPiecewiseByYearAndSexDiagnostic.h"

#include "InterventionEnums.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)
#include "IIndividualHumanHIV.h"
#include "IIndividualHumanContext.h"
#include "IHIVInterventionsContainer.h"
#include "SimulationEnums.h"
#include "IdmDateTime.h"
#include "RANDOM.h"

SETUP_LOGGING( "HIVPiecewiseByYearAndSexDiagnostic" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(HIVPiecewiseByYearAndSexDiagnostic)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_INTERFACE(IBaseIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
    END_QUERY_INTERFACE_BODY(HIVPiecewiseByYearAndSexDiagnostic)

    IMPLEMENT_FACTORY_REGISTERED(HIVPiecewiseByYearAndSexDiagnostic)

    HIVPiecewiseByYearAndSexDiagnostic::HIVPiecewiseByYearAndSexDiagnostic()
        : AbstractDecision(true)
        , interpolation_order(0)
        , female_multiplier(1)
        , default_value(0)
        , year2ValueMap()
    {
        initSimTypes(1, "HIV_SIM" ); // just limiting this to HIV for release
    }

    HIVPiecewiseByYearAndSexDiagnostic::HIVPiecewiseByYearAndSexDiagnostic( const HIVPiecewiseByYearAndSexDiagnostic& master )
        : AbstractDecision( master )
        , interpolation_order( master.interpolation_order )
        , female_multiplier( master.female_multiplier )
        , default_value( master.default_value )
        , year2ValueMap( master.year2ValueMap )
    {
    }

    bool
    HIVPiecewiseByYearAndSexDiagnostic::Configure(const Configuration* inputJson)
    {
        // This used to be available.  I saw it used in test but never by researchers
        // This is just a double check
        if( inputJson->Exist( "Days_To_Diagnosis" ) )
        {
            std::stringstream ss;
            ss << "'Days_To_Diagnosis' is no longer supported.\n";
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        if( inputJson->Exist( "Event_Or_Config" ) )
        {
            std::stringstream ss;
            ss << "'Event_Or_Config' is no longer needed.  Only events are supported.";
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        initConfigTypeMap("Interpolation_Order", &interpolation_order, HIV_PBYASD_Interpolation_Order_DESC_TEXT, 0, 1, 0);
        initConfigTypeMap("Female_Multiplier", &female_multiplier, HIV_PBYASD_Female_Multiplier_DESC_TEXT, 0, FLT_MAX, 1);
        initConfigTypeMap("Default_Value", &default_value, HIV_PBYASD_Default_Value_DESC_TEXT, 0, 1, 0);
        initConfigTypeMap("Time_Value_Map", &year2ValueMap, HIV_PBYASD_Time_Value_Map_DESC_TEXT);

        bool ret = AbstractDecision::Configure(inputJson);
        return ret;
    }

    bool HIVPiecewiseByYearAndSexDiagnostic::MakeDecision( float dt )
    {
        bool testResult = 0;
        float value = 0;

        float year = parent->GetEventContext()->GetNodeEventContext()->GetTime().Year();
        auto gender = parent->GetEventContext()->GetGender();

        switch (interpolation_order)
        {
        case 0:
            value = year2ValueMap.getValuePiecewiseConstant(year, default_value);
            break;
        case 1:
            value = year2ValueMap.getValueLinearInterpolation(year, default_value);
            break;
        default:
            LOG_WARN_F("Invalid selection for interpolation order.  Using linear interpolation as default.");
            value = year2ValueMap.getValueLinearInterpolation(year, default_value);
        }

        value = (gender == Gender::FEMALE) ? value * female_multiplier : value;
        testResult = parent->GetRng()->SmartDraw( value );

        LOG_DEBUG_F("Individual %d: sex=%d, year=%f, returned %d.\n", parent->GetSuid().data, gender, year, testResult);

        return testResult;
    }

    REGISTER_SERIALIZABLE(HIVPiecewiseByYearAndSexDiagnostic);

    void HIVPiecewiseByYearAndSexDiagnostic::serialize(IArchive& ar, HIVPiecewiseByYearAndSexDiagnostic* obj)
    {
        AbstractDecision::serialize( ar, obj );
        HIVPiecewiseByYearAndSexDiagnostic& diag = *obj;

        ar.labelElement("interpolation_order"  ) & diag.interpolation_order;
        ar.labelElement("female_multiplier"    ) & diag.female_multiplier;
        ar.labelElement("default_value"        ) & diag.default_value;
        ar.labelElement("year2ValueMap"        ) & diag.year2ValueMap;
    }
}
