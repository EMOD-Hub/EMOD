
#include "stdafx.h"
#include "HIVSigmoidByYearAndSexDiagnostic.h"

#include "NodeEventContext.h"  // for INodeEventContext
#include "SimulationEnums.h"
#include "Sigmoid.h"
#include "IIndividualHumanContext.h"
#include "IdmDateTime.h"
#include "RANDOM.h"

SETUP_LOGGING( "HIVSigmoidByYearAndSexDiagnostic" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(HIVSigmoidByYearAndSexDiagnostic)
        HANDLE_INTERFACE(IDistributableIntervention)
        //HANDLE_INTERFACE(IHealthSeekingBehavior)
        HANDLE_INTERFACE(IBaseIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
    END_QUERY_INTERFACE_BODY(HIVSigmoidByYearAndSexDiagnostic)

    IMPLEMENT_FACTORY_REGISTERED(HIVSigmoidByYearAndSexDiagnostic)

    HIVSigmoidByYearAndSexDiagnostic::HIVSigmoidByYearAndSexDiagnostic()
        : AbstractDecision( true )
        , rampMin(0)
        , rampMax(1)
        , rampMidYear(2000)
        , rampRate(1)
        , femaleMultiplier(1)
    {
        initSimTypes(1, "HIV_SIM" ); // just limiting this to HIV for release
    }

    HIVSigmoidByYearAndSexDiagnostic::HIVSigmoidByYearAndSexDiagnostic( const HIVSigmoidByYearAndSexDiagnostic& master )
        : AbstractDecision( master )
        , rampMin( master.rampMin )
        , rampMax( master.rampMax )
        , rampMidYear( master.rampMidYear )
        , rampRate( master.rampRate )
        , femaleMultiplier( master.femaleMultiplier )
    {
    }

    bool HIVSigmoidByYearAndSexDiagnostic::Configure( const Configuration* inputJson )
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

        initConfigTypeMap( "Ramp_Min", &rampMin, HIV_Ramp_Min_DESC_TEXT, -1, 1, 0 );
        initConfigTypeMap( "Ramp_Max", &rampMax, HIV_Ramp_Max_DESC_TEXT, -1, 1, 1 );
        initConfigTypeMap( "Ramp_MidYear", &rampMidYear, HIV_Ramp_MidYear_DESC_TEXT, MIN_YEAR, MAX_YEAR, 2000 );
        initConfigTypeMap( "Ramp_Rate", &rampRate, HIV_Ramp_Rate_DESC_TEXT, -100, 100, 1 );
        initConfigTypeMap( "Female_Multiplier", &femaleMultiplier, HIV_Female_Multiplier_DESC_TEXT, 0, FLT_MAX, 1 );

        return AbstractDecision::Configure( inputJson );
    }

    bool HIVSigmoidByYearAndSexDiagnostic::MakeDecision( float dt )
    {
        float year = parent->GetEventContext()->GetNodeEventContext()->GetTime().Year();
        auto gender = parent->GetEventContext()->GetGender();

        float valueMultiplier = (gender == Gender::FEMALE) ? femaleMultiplier : 1;
        float value_tmp = Sigmoid::variableWidthAndHeightSigmoid( year, rampMidYear, rampRate, rampMin, rampMax );
        float value = max( 0.0f, value_tmp ) * valueMultiplier;

        LOG_DEBUG_F("min=%f, max=%f, rate=%f, midYear=%f, multiplier=%f, year=%f\n", rampMin, rampMax, rampRate, rampMidYear, valueMultiplier, year);
        LOG_DEBUG_F("rampMin + (rampMax-rampMin)/(1.0 + exp(-rampRate*(year-rampMidYear))) = %f\n", value_tmp );

        bool testResult = (parent->GetRng()->SmartDraw( value) );
        LOG_DEBUG_F("Individual %d: sex=%d, year=%f, value=%f, returning %d.\n", parent->GetSuid().data, gender, year, value, testResult);
 
        return testResult;
    }

    REGISTER_SERIALIZABLE(HIVSigmoidByYearAndSexDiagnostic);

    void HIVSigmoidByYearAndSexDiagnostic::serialize(IArchive& ar, HIVSigmoidByYearAndSexDiagnostic* obj)
    {
        AbstractDecision::serialize( ar, obj );
        HIVSigmoidByYearAndSexDiagnostic& diag = *obj;

        ar.labelElement("rampMin"         ) & diag.rampMin;
        ar.labelElement("rampMax"         ) & diag.rampMax;
        ar.labelElement("rampMidYear"     ) & diag.rampMidYear;
        ar.labelElement("rampRate"        ) & diag.rampRate;
        ar.labelElement("femaleMultiplier") & diag.femaleMultiplier;
    }
}
