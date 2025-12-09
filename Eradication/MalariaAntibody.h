
#pragma once

#include <stdint.h>       // for int64_t
#include "MalariaEnums.h" // for MalariaAntibodyType enum
#include "Configuration.h"

namespace Kernel
{
    struct IArchive;
    class SusceptibilityMalariaConfig;

    class MalariaAntibody
    {
    public:

        DECLARE_QUERY_INTERFACE()
        IMPLEMENT_ONLY_REFERENCE_COUNTING()

    public:
        static MalariaAntibody CreateAntibody( MalariaAntibodyType::Enum type, int variant, float capacity=0.0f );

        MalariaAntibody();
        ~MalariaAntibody() {}

        void  Decay( float dt );
        float StimulateCytokines( float dt, float inv_uL_blood );
        void  UpdateAntibodyCapacity( float dt, float inv_uL_blood );
        void  UpdateAntibodyCapacityByRate( float dt, float growth_rate );
        void  UpdateAntibodyConcentration( float dt );
        void  ResetCounters();

        void  IncreaseAntigenCount( int64_t antigenCount, float currentTime, float dt );
        void  SetAntigenicPresence( bool antigenPresent );

        int64_t GetAntigenCount() const;
        bool    GetAntigenicPresence() const;
        float   GetAntibodyCapacity() const;
        float   GetAntibodyConcentration() const;

        void    SetAntibodyCapacity( float antibody_capacity );
        void    SetAntibodyConcentration(float antibody_concentration);

        MalariaAntibodyType::Enum GetAntibodyType() const;
        int GetAntibodyVariant() const;

        void  SetTimeLastActive( float time );
        void  PrepareForSerialization( float time, float dt );

        void SetActiveIndex( int32_t index );
        int32_t GetActiveIndex() const;

    protected:
        float   m_antibody_capacity;
        float   m_antibody_concentration;
        int64_t m_antigen_count;
        int32_t m_active_index;
        float   m_time_last_active;         // sim time since the antibody was last active not just decaying
        float   m_duration_not_active_serialization;

        MalariaAntibodyType::Enum m_antibody_type;
        int m_antibody_variant;

        void Initialize( MalariaAntibodyType::Enum type, int variant, float capacity = 0, float concentration = 0 );
    public:
        static void serialize( IArchive& ar, MalariaAntibody& obj );
    };

    struct pfemp1_antibody_t
    {
        MalariaAntibody* minor;
        MalariaAntibody* major;
    };

    void serialize(IArchive&, pfemp1_antibody_t&);
}
