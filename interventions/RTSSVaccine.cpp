
#include "stdafx.h"
#include "RTSSVaccine.h"

#include "InterventionEnums.h"
#include "MalariaContexts.h"     // for BoostAntibody method
#include "IIndividualHumanContext.h"

SETUP_LOGGING( "RTSSVaccine" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(RTSSVaccine)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
    END_QUERY_INTERFACE_BODY(RTSSVaccine)

    IMPLEMENT_FACTORY_REGISTERED(RTSSVaccine)

    RTSSVaccine::RTSSVaccine()
        : BaseIntervention()
        , antibody_type( MalariaAntibodyType:: CSP )
        , antibody_variant( 0 )
        , boosted_antibody_concentration( 1.0f )
    {
        initSimTypes( 1, "MALARIA_SIM" );
    }

    bool
    RTSSVaccine::Configure(
        const Configuration * inputJson
    )
    {
        initConfigTypeMap( "Cost_To_Consumer", &cost_per_unit, IV_Cost_To_Consumer_DESC_TEXT, 0, 999999, 3.75 );
        initConfigTypeMap( "Boosted_Antibody_Concentration", &boosted_antibody_concentration, Boosted_Antibody_Concentration_DESC_TEXT, 0.0, FLT_MAX, 1.0 );

        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! Commented these out because only CSP is supported.
        // !!! By not allowing the user to change this value, 
        // !!! it can be clear that it only supports CSP.
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        //initConfig( "Antibody_Type", antibody_type, inputJson, MetadataDescriptor::Enum("Antibody_Type", RV_Antibody_Type_DESC_TEXT, MDD_ENUM_ARGS(MalariaAntibodyType)) );
        //initConfigTypeMap( "Antibody_Variant", &antibody_variant, RV_Antibody_Variant_DESC_TEXT, 0, 1e5, 0 );

        return BaseIntervention::Configure( inputJson );
    }

    void RTSSVaccine::Update( float dt )
    {
        if( !BaseIntervention::UpdateIndividualsInterventionStatus() ) return;

        IMalariaHumanContext * imhc = nullptr;
        if( s_OK != parent->QueryInterface( GET_IID( IMalariaHumanContext ), (void**)&imhc ) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "imhc", "IMalariaHumanContext", "IIndividualHumanContext" );
        }
        imhc->GetMalariaSusceptibilityContext()->BoostAntibody( antibody_type, antibody_variant, boosted_antibody_concentration );

        SetExpired( true );
    }

    REGISTER_SERIALIZABLE(RTSSVaccine);

    void RTSSVaccine::serialize(IArchive& ar, RTSSVaccine* obj)
    {
        BaseIntervention::serialize( ar, obj );
        RTSSVaccine& vaccine = *obj;
        ar.labelElement("antibody_type") & (uint32_t&)vaccine.antibody_type;
        ar.labelElement("antibody_variant") & vaccine.antibody_variant;
        ar.labelElement("boosted_antibody_concentration") & vaccine.boosted_antibody_concentration;
    }
}
