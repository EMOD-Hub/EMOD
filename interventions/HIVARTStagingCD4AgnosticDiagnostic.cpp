
#include "stdafx.h"
#include "HIVARTStagingCD4AgnosticDiagnostic.h"

#include "InfectionHIV.h"
#include "IIndividualHumanHIV.h"
#include "InterventionEnums.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)
#include "IHIVInterventionsContainer.h" // for time-date util function
#include "IIndividualHumanContext.h"

SETUP_LOGGING( "HIVARTStagingCD4AgnosticDiagnostic" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(HIVARTStagingCD4AgnosticDiagnostic, HIVARTStagingAbstract)
    END_QUERY_INTERFACE_DERIVED(HIVARTStagingCD4AgnosticDiagnostic, HIVARTStagingAbstract)

    IMPLEMENT_FACTORY_REGISTERED(HIVARTStagingCD4AgnosticDiagnostic)

    HIVARTStagingCD4AgnosticDiagnostic::HIVARTStagingCD4AgnosticDiagnostic()
    : HIVARTStagingAbstract()
    , adultAge(5)
    {
        initSimTypes( 1, "HIV_SIM" );
    }

    HIVARTStagingCD4AgnosticDiagnostic::HIVARTStagingCD4AgnosticDiagnostic( const HIVARTStagingCD4AgnosticDiagnostic& master )
    : HIVARTStagingAbstract( master )
    {
        adultAge  = master.adultAge ;
        adultByWHOStage = master.adultByWHOStage;
        adultByTB = master.adultByTB;
        adultByPregnant = master.adultByPregnant;
        childTreatUnderAgeThreshold = master.childTreatUnderAgeThreshold;
        childByWHOStage = master.childByWHOStage;
        childByTB = master.childByTB;
    }

    bool HIVARTStagingCD4AgnosticDiagnostic::Configure( const Configuration* inputJson )
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

        initConfigTypeMap("Adult_Treatment_Age", &adultAge, HIV_Adult_Treatment_Age_DESC_TEXT, -1, FLT_MAX, 5);
        //initConfigTypeMap("Stable_Partner_Minimum_Duration", &stablePartnerMinimumDuration, HIV_Stable_Partner_Minimum_Duration_DESC_TEXT , -1, FLT_MAX, 365);
        
        initConfigTypeMap("Adult_By_WHO_Stage", &adultByWHOStage, HIV_Adult_By_WHO_Stage_DESC_TEXT);
        initConfigTypeMap("Adult_By_TB", &adultByTB, HIV_Adult_By_TB_DESC_TEXT);
        //initConfigTypeMap("Adult_By_Stable_Discordant_Partner", &adultByStableDiscodantPartner, HIV_Adult_By_Stable_Discordant_Partner_DESC_TEXT );
        initConfigTypeMap("Adult_By_Pregnant", &adultByPregnant, HIV_Adult_By_Pregnant_DESC_TEXT );

        initConfigTypeMap("Child_Treat_Under_Age_In_Years_Threshold", &childTreatUnderAgeThreshold, HIV_Child_Treat_Under_Age_In_Years_Threshold_DESC_TEXT );
        initConfigTypeMap("Child_By_WHO_Stage", &childByWHOStage, HIV_Child_By_WHO_Stage_DESC_TEXT );
        initConfigTypeMap("Child_By_TB", &childByTB, HIV_Child_By_TB_DESC_TEXT );

        return HIVARTStagingAbstract::Configure( inputJson );
    }

    // staged for ART via CD4 agnostic testing?
    bool HIVARTStagingCD4AgnosticDiagnostic::MakeDecision( IIndividualHumanHIV *pHIV, 
                                                           float year, 
                                                           float CD4count, 
                                                           bool hasActiveTB, 
                                                           bool isPregnant )
    {

        if( !pHIV->HasHIV() ) {
            return false;
        }

        IHIVMedicalHistory * med_parent = pHIV->GetMedicalHistory();

        float WHO_Stage = MIN_WHO_HIV_STAGE ;
        if( pHIV->HasHIV() )
        {
            WHO_Stage = pHIV->GetHIVInfection()->GetWHOStage();
        }

        med_parent->OnAssessWHOStage(WHO_Stage);

        float age_days = parent->GetEventContext()->GetAge();

        bool result = false ;
        if (age_days >= adultAge * DAYSPERYEAR)
        {
            result = TestAdult( WHO_Stage, year, hasActiveTB, isPregnant );
        }
        else
        {
            result = TestChild( WHO_Stage, year, hasActiveTB, age_days );
        }

        return result;
    }

    bool HIVARTStagingCD4AgnosticDiagnostic::TestAdult( float WHO_Stage,
                                                        float year, 
                                                        bool hasActiveTB, 
                                                        bool isPregnant )
    {
        bool result =
               (adultByWHOStage.getValuePiecewiseConstant(year, MAX_WHO_HIV_STAGE+1) <= WHO_Stage)
            || (hasActiveTB && adultByTB.getValuePiecewiseConstant(year, 0))
            || (isPregnant  && adultByPregnant.getValuePiecewiseConstant(year, 0));

        return result;
    }

    bool HIVARTStagingCD4AgnosticDiagnostic::TestChild( float WHO_Stage,
                                                        float year, 
                                                        bool hasActiveTB,
                                                        float ageDays )
    {
        bool result =  (ageDays <= childTreatUnderAgeThreshold.getValuePiecewiseConstant(year, -1)*DAYSPERYEAR)
                    || (childByWHOStage.getValuePiecewiseConstant(year, MAX_WHO_HIV_STAGE+1) <= WHO_Stage)
                    || ( hasActiveTB && childByTB.getValuePiecewiseConstant(year, 0));

        return result;
    }

    REGISTER_SERIALIZABLE(HIVARTStagingCD4AgnosticDiagnostic);

    void HIVARTStagingCD4AgnosticDiagnostic::serialize(IArchive& ar, HIVARTStagingCD4AgnosticDiagnostic* obj)
    {
        HIVARTStagingAbstract::serialize( ar, obj );
        HIVARTStagingCD4AgnosticDiagnostic& diag = *obj;

        ar.labelElement("adultAge"                    ) & diag.adultAge;
        ar.labelElement("adultByWHOStage"             ) & diag.adultByWHOStage;
        ar.labelElement("childByWHOStage"             ) & diag.childByWHOStage;
        ar.labelElement("adultByTB"                   ) & diag.adultByTB;
        ar.labelElement("childByTB"                   ) & diag.childByTB;
        ar.labelElement("adultByPregnant"             ) & diag.adultByPregnant;
        ar.labelElement("childTreatUnderAgeThreshold" ) & diag.childTreatUnderAgeThreshold;
    }
}
