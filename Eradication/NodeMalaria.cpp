
#include "stdafx.h"

#include "NodeMalaria.h"
#include "IndividualMalaria.h"
#include "Susceptibility.h" // for SusceptibilityConfig::immunity_initalization...
#include "Common.h"
#include "Malaria.h"
#include "NodeMalariaEventContext.h"
#include "SimulationConfig.h"
#include "MathFunctions.h"
#include "DistributionFactory.h"

SETUP_LOGGING( "NodeMalaria" )

namespace Kernel
{
    // QI stuff in case we want to use it more extensively
    BEGIN_QUERY_INTERFACE_DERIVED(NodeMalaria, NodeVector)
        HANDLE_INTERFACE(INodeMalaria)
    END_QUERY_INTERFACE_DERIVED(NodeMalaria, NodeVector)

    NodeMalaria::NodeMalaria()
        : NodeVector()
        , m_New_Clinical_Cases(0)
        , m_New_Severe_Cases(0)
        , m_Maternal_Antibody_Fraction(0)
    {
    }

    NodeMalaria::NodeMalaria(ISimulationContext *simulation, ExternalNodeId_t externalNodeId, suids::suid suid)
        : NodeVector(simulation, externalNodeId, suid)
        , m_New_Clinical_Cases(0)
        , m_New_Severe_Cases(0)
        , m_Maternal_Antibody_Fraction(0)
    {
    }

    bool NodeMalaria::Configure( const Configuration * config )
    { 
        return NodeVector::Configure( config );
    }

    void NodeMalaria::Initialize()
    {
        NodeVector::Initialize();
    }

    NodeMalaria *NodeMalaria::CreateNode(ISimulationContext *simulation, ExternalNodeId_t externalNodeId, suids::suid suid)
    {
        NodeMalaria *newnode = _new_ NodeMalaria(simulation, externalNodeId, suid);
        newnode->Initialize();

        return newnode;
    }

    NodeMalaria::~NodeMalaria()
    {
        // individual deletion done by ~Node
        // vectorpopulation deletion handled at _Vector level
    }

    IIndividualHuman* NodeMalaria::createHuman( suids::suid suid, float monte_carlo_weight, float initial_age, int gender)
    {
        return IndividualHumanMalaria::CreateHuman(getContextPointer(), suid, monte_carlo_weight, initial_age, gender);
    }

    IIndividualHuman* NodeMalaria::addNewIndividual( float monte_carlo_weight, float initial_age, int gender, int initial_infection_count, float immparam, float riskparam, float mighet)
    {
        // just the base class for now
        return NodeVector::addNewIndividual(monte_carlo_weight, initial_age, gender, initial_infection_count, immparam, riskparam, mighet);
    }

    void NodeMalaria::accumulateIndividualPopulationStatistics( float dt, IIndividualHuman* basic_individual)
    {
        // Do base-class behavior, e.g. UpdateInfectiousness, statPop, Possible_Mothers
        NodeVector::accumulateIndividualPopulationStatistics(dt, basic_individual);

        // Cast from IndividualHuman to IndividualHumanMalaria
        IMalariaHumanContext* individual = nullptr;
        if( basic_individual->QueryInterface( GET_IID( IMalariaHumanContext ), (void**)&individual ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "individual", "IndividualHumanMalaria", "IndividualHuman" );
        }
        float mc_weight = float(basic_individual->GetMonteCarloWeight());

        if ( basic_individual->IsPossibleMother() )
        {
            m_Maternal_Antibody_Fraction += mc_weight * individual->GetMalariaSusceptibilityContext()->get_fraction_of_variants_with_antibodies(MalariaAntibodyType::PfEMP1_major);
        }
    }

    void NodeMalaria::updatePopulationStatistics(float dt)
    {
        // Update population statistics and vector lifecycle probabilities as in base class
        NodeVector::updatePopulationStatistics(dt);

        // normalize and calculate a few more derived quantities
        if ( Possible_Mothers > 0 )
        {
            m_Maternal_Antibody_Fraction /= Possible_Mothers;
        }
    }

    void NodeMalaria::updateNodeStateCounters( IIndividualHuman* ih )
    {
        float weight = ih->GetMonteCarloWeight();
        IMalariaHumanContext* malaria_human = nullptr;
        if( ih->QueryInterface( GET_IID( IMalariaHumanContext ), (void**) &malaria_human ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "ih", "IndividualHuman", "IndividualHumanMalaria" );
        }

        m_New_Clinical_Cases += weight * malaria_human->HasClinicalSymptomNew(ClinicalSymptomsEnum::CLINICAL_DISEASE);
        m_New_Severe_Cases   += weight * malaria_human->HasClinicalSymptomNew(ClinicalSymptomsEnum::SEVERE_DISEASE);

        malaria_human->GetMalariaSusceptibilityContext()->ResetMaximumSymptoms();

        Node::updateNodeStateCounters(ih); // this must be at the end, since it clears the NewInfectionState
        //malaria_human->Release();
    }

    void NodeMalaria::resetNodeStateCounters()
    {
        NodeVector::resetNodeStateCounters();

        m_New_Clinical_Cases      = 0;
        m_New_Severe_Cases        = 0;

        m_Maternal_Antibody_Fraction = 0;
    }

    const SimulationConfig* NodeMalaria::params()
    {
        return GET_CONFIGURABLE(SimulationConfig);
    }

    void NodeMalaria::SetupEventContextHost()
    {
        event_context_host = _new_ NodeMalariaEventContextHost(this);
    }

    REGISTER_SERIALIZABLE(NodeMalaria);

    void NodeMalaria::serialize(IArchive& ar, NodeMalaria* obj)
    {
        NodeVector::serialize(ar, obj);
        NodeMalaria& node = *obj;
        ar.labelElement( "m_New_Clinical_Cases"         ) & node.m_New_Clinical_Cases;
        ar.labelElement( "m_New_Severe_Cases"           ) & node.m_New_Severe_Cases;
        ar.labelElement( "m_Maternal_Antibody_Fraction" ) & node.m_Maternal_Antibody_Fraction;
    }
}
