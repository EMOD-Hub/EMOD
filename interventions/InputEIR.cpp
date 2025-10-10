
#include "stdafx.h"
#include "InputEIR.h"

#include "Exceptions.h"
#include "NodeMalariaEventContext.h"  // for ISporozoiteChallengeConsumer methods
#include "SusceptibilityVector.h"     // for age-dependent biting risk static methods

SETUP_LOGGING( "InputEIR" )

namespace Kernel
{
    float ACTUALDAYSPERMONTH = float( DAYSPERYEAR ) / MONTHSPERYEAR;  // 30.416666

    BEGIN_QUERY_INTERFACE_BODY(InputEIR)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IBaseIntervention)
        HANDLE_INTERFACE(INodeDistributableIntervention)
        HANDLE_ISUPPORTS_VIA(INodeDistributableIntervention)
    END_QUERY_INTERFACE_BODY(InputEIR)

    IMPLEMENT_FACTORY_REGISTERED(InputEIR)

    InputEIR::InputEIR() 
    : BaseNodeIntervention()
    , eir_type( EIRType::MONTHLY )
    , monthly_EIR()
    , daily_EIR()
    , scaling_factor(1.0f)
    , today(0)
    {
        initSimTypes( 1, "MALARIA_SIM" ); // using sporozoite challenge
    }

    InputEIR::InputEIR( const InputEIR& master )
    : BaseNodeIntervention( master )
    , eir_type( master.eir_type )
    , monthly_EIR( master.monthly_EIR )
    , daily_EIR( master.daily_EIR )
    , scaling_factor( master.scaling_factor )
    , today( master.today )
    {
    }

    bool InputEIR::Configure( const Configuration * inputJson )
    {
        initConfig(        "EIR_Type",        eir_type,        inputJson, MetadataDescriptor::Enum( "EIR_Type", IE_EIR_Type_DESC_TEXT, MDD_ENUM_ARGS( EIRType ) ) );
        initConfigTypeMap( "Monthly_EIR",    &monthly_EIR,     IE_Monthly_EIR_DESC_TEXT,    0.0f, 1000.0f, false, "EIR_Type", "MONTHLY" );
        initConfigTypeMap( "Daily_EIR",      &daily_EIR,       IE_Daily_EIR_DESC_TEXT,      0.0f, 1000.0f, false, "EIR_Type", "DAILY" );
        initConfigTypeMap( "Scaling_Factor", &scaling_factor,  IE_Scaling_Factor_DESC_TEXT, 0.0f, 10000.0f, 1.0 );

        bool configured = BaseNodeIntervention::Configure( inputJson );
        if( configured && ! JsonConfigurable::_dryrun )
        {
            if(inputJson->Exist("Age_Dependence"))
            {
                std::stringstream ss;
                ss << "'Age_Dependence' is no longer a parameter in the 'InputEIR' intervention. ";
                ss << "'Age_Dependent_Biting_Risk_Type' configuration parameter is used to determine if there is any age-dependent biting risk.";
                throw GeneralConfigurationException(__FILE__, __LINE__, __FUNCTION__, ss.str().c_str());
            }
            if( !inputJson->Exist( "EIR_Type" ) )
            {
                std::stringstream ss;
                ss << "'EIR_Type' is not defined and must be.\n";
                ss << "'MONTHLY' will require you to define the 'Monthly_EIR' parameter with 12 monthly mean EIR values.\n";
                ss << "'DAILY' will require you to define the 'Daily_EIR' parameter with 365 daily mean EIR values.";
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }

            if( (eir_type == EIRType::MONTHLY) && (monthly_EIR.size() != MONTHSPERYEAR) )
            {
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, 
                    "'Monthly_EIR' parameterizes the mean number of infectious bites experienced by an individual for each month of the year.  As such, it must be an array of EXACTLY length 12." );
            }
            else if( (eir_type == EIRType::DAILY) && (daily_EIR.size() != DAYSPERYEAR) )
            {
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                     "'Daily_EIR' parameterizes the mean number of infectious bites experienced by an individual for each day of the year.  As such, it must be an array of EXACTLY length 365." );
            }
        }

        return configured;
    }

    bool InputEIR::Distribute( INodeEventContext *pNodeContext, IEventCoordinator2 *pEC )
    {
        pNodeContext->PurgeExistingByType( typeid( *this ).name() );
        return BaseNodeIntervention::Distribute( pNodeContext, pEC );
    }

    void InputEIR::Update( float dt )
    {
        if( !BaseNodeIntervention::UpdateNodesInterventionStatus() ) return;

        float eir_for_today = 0.0;
        if( eir_type == EIRType::MONTHLY )
        {
            int month_index = int( today / ACTUALDAYSPERMONTH ) % MONTHSPERYEAR;
            eir_for_today = monthly_EIR.at( month_index ) / ACTUALDAYSPERMONTH;
        }
        else
        {
            int day_index = int( today ) % DAYSPERYEAR;
            eir_for_today = daily_EIR.at( day_index );
        }

        today += dt;

        eir_for_today *= scaling_factor;

        LOG_DEBUG_F("Day = %d, annualized EIR = %0.2f\n", today, DAYSPERYEAR*eir_for_today );

        ISporozoiteChallengeConsumer *iscc;
        if (s_OK != parent->QueryInterface(GET_IID(ISporozoiteChallengeConsumer), (void**)&iscc))
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "iscc", "ISporozoiteChallengeConsumer", "INodeEventContext" );
        }
        iscc->ChallengeWithInfectiousBites( 1, eir_for_today);
    }

    float InputEIR::GetCostPerUnit() const
    {
        // -------------------------------------------------------------------------------
        // --- Since this intervention is used to infect people in absence of mosquitos,
        // --- it doesn't have a cost associated with it.
        // -------------------------------------------------------------------------------
        return 0.0;
    }

}
