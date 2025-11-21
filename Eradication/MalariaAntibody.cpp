
#include "stdafx.h"
#include "MalariaAntibody.h"
#include "Sigmoid.h"
#include "SusceptibilityMalaria.h"

#define NON_TRIVIAL_ANTIBODY_THRESHOLD  (0.0000001)
#define TWENTY_DAY_DECAY_CONSTANT       (0.05f)
#define B_CELL_PROLIFERATION_THRESHOLD  (0.4)
#define B_CELL_PROLIFERATION_CONSTANT   (0.33f)
#define ANTIBODY_RELEASE_THRESHOLD      (0.3)
#define ANTIBODY_RELEASE_FACTOR         (4)

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(MalariaAntibody)
    END_QUERY_INTERFACE_BODY(MalariaAntibody)

    MalariaAntibody MalariaAntibody::CreateAntibody( MalariaAntibodyType::Enum type, int variant, float capacity )
    {
        MalariaAntibody antibody;
        antibody.Initialize( type, variant, capacity );

        return antibody;
    }

    MalariaAntibody::MalariaAntibody()
        : m_antibody_capacity(0.0)
        , m_antibody_concentration(0.0)
        , m_antigen_count(0)
        , m_active_index(-1)
        , m_time_last_active(-1.0)
        , m_duration_not_active_serialization(0.0)
        , m_antibody_type( MalariaAntibodyType::CSP )
        , m_antibody_variant(0)
    {
    }

    void MalariaAntibody::Initialize( MalariaAntibodyType::Enum type, int variant, float capacity, float concentration )
    {
        m_antibody_type          = type;
        m_antibody_variant       = variant;
        m_antibody_capacity      = capacity;
        m_antibody_concentration = concentration;
    }

    void MalariaAntibody::Decay( float decay_time )
    {
        // decay_time - time (in days) the antibody has spent being inactive, see IncreaseAntigenCount for details
        // this is only called when antibodies are activated, we calculate how much they would have decayed and then grow them

        // allow the decay of anti-CSP concentrations greater than unity (e.g. after boosting by vaccine)
        // TODO: this might become the default when boosting extends to other antibody types?
        if( (m_antibody_type == MalariaAntibodyType::CSP) && (m_antibody_concentration > m_antibody_capacity) )
        {
            m_antibody_concentration -= m_antibody_concentration * decay_time / SusceptibilityMalariaConfig::antibody_csp_decay_days;
        }
        else
        {
            // otherwise do the normal behavior of decaying antibody concentration based on capacity

            // don't do multiplication and subtraction unless antibody levels non-trivial
            if(m_antibody_concentration > NON_TRIVIAL_ANTIBODY_THRESHOLD)
            {
                m_antibody_concentration = m_antibody_concentration * exp( -decay_time * TWENTY_DAY_DECAY_CONSTANT );
            }

            if(m_antibody_capacity > SusceptibilityMalariaConfig::memory_level)
            {
                // antibody capacity decays to a medium value (.3) dropping below .4 in ~120 days from 1.0
                m_antibody_capacity = ( m_antibody_capacity - SusceptibilityMalariaConfig::memory_level )
                    * exp( -decay_time * SusceptibilityMalariaConfig::hyperimmune_decay_rate )
                    + SusceptibilityMalariaConfig::memory_level;
            } // stays around memory level until antibody_days_to_long_term_decay kicks in

            // --------------------------------------------------------------------------------
            // --- If the antibody has been dormant for a long time, start a gradual decay.
            // --- This is to help reduce the issue where older people have too much immunity.
            // --------------------------------------------------------------------------------
            if(( decay_time >= SusceptibilityMalariaConfig::antibody_days_to_long_term_decay ) &&
                ( m_antibody_capacity > FLT_EPSILON ))
            {
                float delta_time = decay_time - SusceptibilityMalariaConfig::antibody_days_to_long_term_decay;
                m_antibody_capacity = m_antibody_capacity * exp( -delta_time / SusceptibilityMalariaConfig::antibody_long_term_decay_days );
            }

        }
    }

    float MalariaAntibody::StimulateCytokines( float dt, float inv_uL_blood )
    {
        // Cytokines released at low antibody concentration (if capacity hasn't switched into high proliferation rate yet)
        return ( 1 - m_antibody_concentration ) * float(m_antigen_count) * inv_uL_blood;
    }

    // Let's use the MSP version of antibody growth in the base class ...
    void MalariaAntibody::UpdateAntibodyCapacity( float dt, float inv_uL_blood )
    {
        if( m_antibody_type == MalariaAntibodyType::PfEMP1_minor )
        {
            // The minor PfEMP1 version is similar but not exactly the same...
            float min_stimulation = SusceptibilityMalariaConfig::antibody_stimulation_c50 * SusceptibilityMalariaConfig::minimum_adapted_response;
            float growth_rate     = SusceptibilityMalariaConfig::antibody_capacity_growthrate * SusceptibilityMalariaConfig::non_specific_growth;
            float threshold       = SusceptibilityMalariaConfig::antibody_stimulation_c50;

            if (m_antibody_capacity <= B_CELL_PROLIFERATION_THRESHOLD)
            {
                m_antibody_capacity += growth_rate * dt * (1.0f - m_antibody_capacity) * float(Sigmoid::basic_sigmoid(threshold, float(m_antigen_count) * inv_uL_blood + min_stimulation));
            }
            else
            {
                //rapid B cell proliferation above a threshold given stimulation
                m_antibody_capacity += (1.0f - m_antibody_capacity) * B_CELL_PROLIFERATION_CONSTANT * dt;
            }
        }
        else if( m_antibody_type == MalariaAntibodyType::PfEMP1_major )
        {
            // The major PfEMP1 version is slightly different again...
            float min_stimulation = SusceptibilityMalariaConfig::antibody_stimulation_c50 * SusceptibilityMalariaConfig::minimum_adapted_response;
            float growth_rate     = SusceptibilityMalariaConfig::antibody_capacity_growthrate;
            float threshold       = SusceptibilityMalariaConfig::antibody_stimulation_c50;

            if (m_antibody_capacity <= B_CELL_PROLIFERATION_THRESHOLD)
            {
                //ability and number of B-cells to produce antibodies, with saturation
                m_antibody_capacity += growth_rate * dt * (1.0f - m_antibody_capacity) * float(Sigmoid::basic_sigmoid(threshold, float(m_antigen_count) * inv_uL_blood + min_stimulation));
            }
            else
            {
                //rapid B cell proliferation above a threshold given stimulation
                m_antibody_capacity += (1.0f - m_antibody_capacity) * B_CELL_PROLIFERATION_CONSTANT * dt;
            }
        }
        else //MSP1
        {
            float growth_rate = SusceptibilityMalariaConfig::MSP1_antibody_growthrate;
            float threshold   = SusceptibilityMalariaConfig::antibody_stimulation_c50;

            m_antibody_capacity += growth_rate  * (1.0f - m_antibody_capacity) * float(Sigmoid::basic_sigmoid( threshold, float(m_antigen_count) * inv_uL_blood));

            // rapid B cell proliferation above a threshold given stimulation
            if (m_antibody_capacity > B_CELL_PROLIFERATION_THRESHOLD)
            {
                m_antibody_capacity += ( 1.0f - m_antibody_capacity ) * B_CELL_PROLIFERATION_CONSTANT * dt;
            }
        }
        if(m_antibody_capacity > 1.0)
        {
            m_antibody_capacity = 1.0;
        }
    }

    // Different arguments used by CSP update called directly from IndividualHumanMalaria::ExposeToInfectivity
    // and also in SusceptibilityMalaria::updateImmunityCSP
    void MalariaAntibody::UpdateAntibodyCapacityByRate( float dt, float growth_rate )
    {
        m_antibody_capacity += growth_rate * dt * (1 - m_antibody_capacity);

        if (m_antibody_capacity > 1.0)
        {
            m_antibody_capacity = 1.0;
        }
    }

    void MalariaAntibody::UpdateAntibodyConcentration( float dt )
    {
        // allow the decay of anti-CSP concentrations greater than unity (e.g. after boosting by vaccine)
        // TODO: this might become the default when boosting extends to other antibody types?
        if ( (m_antibody_type == MalariaAntibodyType::CSP) && (m_antibody_concentration > m_antibody_capacity) )
        {
            m_antibody_concentration -= m_antibody_concentration * dt / SusceptibilityMalariaConfig::antibody_csp_decay_days;
            return;
        }
        // otherwise do the normal behavior of incrementing antibody concentration based on capacity

        // release of antibodies and effect of B cell proliferation on capacity
        // antibodies released after capacity passes 0.3
        // detection and proliferation in lymph nodes, etc...
        // and circulating memory cells
        if ( m_antibody_capacity > ANTIBODY_RELEASE_THRESHOLD )
        {
            m_antibody_concentration += ( m_antibody_capacity - m_antibody_concentration ) * ANTIBODY_RELEASE_FACTOR * dt;
        }

        if ( m_antibody_concentration > m_antibody_capacity )
        {
            m_antibody_concentration = m_antibody_capacity;
        }
    }

    void MalariaAntibody::ResetCounters()
    {
        m_antigen_count = 0;
        m_active_index = -1;
    }

    void MalariaAntibody::IncreaseAntigenCount( int64_t antigenCount, float currentTime, float dt )
    {
        m_antigen_count += antigenCount;

        // --------------------------------------------------------------------------------------------
        // --- currentTime here is not sim currentTime but currentTime + X*infectious_timestep(i.e. dt)
        // --- so if this was updated the previous infectious_timestep then it was just active
        // --- and should not decay.   The difference between the currentTime and the last time active
        // --- needs to be greater than the infectious_timestep so we know that it spent sometime
        // --- unactive and needs to be decayed.
        // --- We subtract the dt because we don't decay for the current time step.
        // --- For example, assume that it was last active at time T, was inactive at times
        // --- T+1 and T+2, and was reactivated at time T+3.  (Assume dt=1.)  This means
        // --- currentTime = T+3 and m_time_last_active = T.  currentTime - m_time_last_active
        // --- would give us the duration of three time steps but the antibody was only inactive
        // --- for two of them.  It is active this time step so we don't want to count it.
        // --------------------------------------------------------------------------------------------
        float decay_time = currentTime - m_time_last_active - dt + m_duration_not_active_serialization;
        if( (m_time_last_active != -1.0) && (decay_time > 0.0) )
        {
            Decay( decay_time );
        }
        m_time_last_active = currentTime;

        // ---------------------------------------------------------------------------------
        // --- Need to clear this every time it is active due to the case where the
        // --- antibody was active, but we serialized and called PrepareForSerialization()
        // ---------------------------------------------------------------------------------
        m_duration_not_active_serialization = 0.0;
    }

    void MalariaAntibody::SetTimeLastActive( float time )
    {
        m_time_last_active = time;
    }

    void MalariaAntibody::PrepareForSerialization( float time, float dt )
    {
        if( (time - m_time_last_active) >= dt )
        {
            // this is "+=" incase someone saves multiple serialized files.
            m_duration_not_active_serialization += (time - m_time_last_active);
        }
        m_time_last_active = time;
    }

    void MalariaAntibody::SetAntigenicPresence( bool antigenPresent )
    {
        m_antigen_count = (antigenPresent ? 1 : 0);
    }

    int64_t MalariaAntibody::GetAntigenCount() const
    {
        return m_antigen_count;
    }

    bool MalariaAntibody::GetAntigenicPresence() const
    {
        return (m_antigen_count> 0);
    }

    float MalariaAntibody::GetAntibodyCapacity() const
    {
        return m_antibody_capacity;
    }

    float MalariaAntibody::GetAntibodyConcentration() const
    {
        return m_antibody_concentration;
    }

    void MalariaAntibody::SetAntibodyCapacity( float antibody_capacity )
    {
        m_antibody_capacity = antibody_capacity;
    }

    void MalariaAntibody::SetAntibodyConcentration( float antibody_concentration )
    {
        m_antibody_concentration = antibody_concentration;
    }

    MalariaAntibodyType::Enum MalariaAntibody::GetAntibodyType() const
    {
        return m_antibody_type;
    }

    int MalariaAntibody::GetAntibodyVariant() const
    {
        return m_antibody_variant;
    }

    void MalariaAntibody::SetActiveIndex( int32_t index )
    {
        m_active_index = index;
    }

    int32_t MalariaAntibody::GetActiveIndex() const
    {
        return m_active_index;
    }

    void MalariaAntibody::serialize(IArchive& ar, MalariaAntibody& antibody)
    {
        ar.startObject();
            ar.labelElement("m_antibody_capacity"     ) & antibody.m_antibody_capacity;
            ar.labelElement("m_antibody_concentration") & antibody.m_antibody_concentration;
            ar.labelElement("m_antigen_count"         ) & antibody.m_antigen_count;
            //m_active_index is at most cleared and set each timestep so don't need to serialize
            //ar.labelElement("m_active_index"         ) & antibody.m_active_index;
            ar.labelElement("m_time_last_active"      ) & antibody.m_time_last_active;
            ar.labelElement("m_duration_not_active_serialization") & antibody.m_duration_not_active_serialization;
            ar.labelElement("m_antibody_type"         ) & (uint32_t&)antibody.m_antibody_type;
            ar.labelElement("m_antibody_variant"      ) & antibody.m_antibody_variant;
        ar.endObject();
    }

    void serialize(IArchive& ar, pfemp1_antibody_t& antibodies)
    {
        ar.startObject();
            ar.labelElement("minor") & antibodies.minor;
            ar.labelElement("major") & antibodies.major;
        ar.endObject();
    }
}
