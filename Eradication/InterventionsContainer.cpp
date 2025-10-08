
#include "stdafx.h"

#include "Debug.h"
#include "Exceptions.h"
#include "Sugar.h"
#include "Environment.h"
#include "InterventionsContainer.h"
#include "IIndividualHuman.h"
#include "IIndividualHumanContext.h"
#include <typeinfo>
#ifndef WIN32
#include <cxxabi.h>
#endif
#include "NodeEventContext.h"
#include "INodeContext.h"
#include "Properties.h"
#include "EventTrigger.h"

SETUP_LOGGING( "InterventionsContainer" )

namespace Kernel
{
    InterventionsContainer::InterventionsContainer()
        : drugVaccineReducedAcquire(1.0f)
        , drugVaccineReducedTransmit(1.0f)
        , drugVaccineReducedMortality(1.0f)
        , birth_rate_mod(1.0f)
        , non_disease_death_rate_mod(1.0f)
        , interventions()
        , intervention_names()
        , numAdded(0)
        , parent(nullptr)
        , m_LastIPChange()
    {
    }

    InterventionsContainer::~InterventionsContainer()
    {
        for (auto intervention : interventions)
        {
            delete intervention;
        }
    }

    QueryResult
    InterventionsContainer::QueryInterface( iid_t iid, void** ppinstance )
    {
        assert(ppinstance); // todo: add a real message: "QueryInterface requires a non-NULL destination!");

        if ( !ppinstance )
            return e_NULL_POINTER;

        ISupports* foundInterface;

        GET_IID(IIndividualHumanInterventionsContext);
        iid_t retVal = GET_IID(IIndividualHumanInterventionsContext);
        if (iid == retVal)
            foundInterface = static_cast<IIndividualHumanInterventionsContext*>(this);
        else if (iid == GET_IID(IInterventionConsumer))
            foundInterface = static_cast<IInterventionConsumer*>(this);
        else if (iid == GET_IID(IVaccineConsumer))
            foundInterface = static_cast<IVaccineConsumer*>(this);
        else if (iid == GET_IID(IDrugVaccineInterventionEffects))
            foundInterface = static_cast<IDrugVaccineInterventionEffects*>(this);
        else if (iid == GET_IID(IBirthRateModifier))
            foundInterface = static_cast<IBirthRateModifier*>(this);
        else if (iid == GET_IID(INonDiseaseDeathRateModifier))
            foundInterface = static_cast<INonDiseaseDeathRateModifier*>(this);
        else
            foundInterface = nullptr;

        QueryResult status;
        if ( !foundInterface )
        {
            status = e_NOINTERFACE;
            // if we had a base class we would QI down into it, but we don't.
        }
        else
        {
            //foundInterface->AddRef();           // not implementing this yet!
            status = s_OK;
        }

        *ppinstance = foundInterface;
        return status;
    }

    void InterventionsContainer::PropagateContextToDependents()
    {
        IIndividualHumanContext *context = GetParent();
        for (auto intervention : interventions)
        {
            intervention->SetContextTo(context);
        }
    }

    std::list<void*> InterventionsContainer::GetInterventionsByInterface( iid_t iid )
    {
        std::list<void*> interface_list;
        for (auto intervention : interventions)
        {
            void* p_interface = nullptr;
            if ( s_OK == intervention->QueryInterface( iid, (void**)&p_interface) )
            {
                interface_list.push_back( p_interface );
            }
        }

        return interface_list;
    }

    IDistributableIntervention* InterventionsContainer::GetIntervention( const std::string& type_name )
    {
        for( auto p_intervention : interventions )
        {
            std::string cur_iv_type_name = typeid( *p_intervention ).name();
            if( cur_iv_type_name == type_name)
            {
                return p_intervention ;
            }
        }
        return nullptr ;
    }

    void InterventionsContainer::Remove( int index )
    {
        IDistributableIntervention* p_intervention = interventions[ index ];

        interventions[ index ] = interventions.back();
        interventions.pop_back();

        intervention_names[ index ] = intervention_names.back();
        intervention_names.pop_back();

        delete p_intervention;
    }

    void InterventionsContainer::PurgeExistingByType( const std::string & type_name )
    {
        for( int i = 0; i < interventions.size(); ++i )
        {
            IDistributableIntervention* p_intervention = interventions[ i ];
            std::string cur_iv_type_name = typeid( *p_intervention ).name();
            if( cur_iv_type_name == type_name)
            {
                LOG_DEBUG_F("Found an existing intervention '%s'. Purging.\n", type_name.c_str());
                Remove( i );
                break;
            }
        }
    }

    void InterventionsContainer::PurgeExistingByName( const std::string& type_name, const InterventionName& iv_name)
    {
        for(int i = 0; i < interventions.size(); ++i)
        {
            IDistributableIntervention* p_intervention = interventions[i];
            std::string cur_iv_type_name = typeid(*p_intervention).name();
            if(cur_iv_type_name == type_name)
            {
                if (p_intervention->GetName() == iv_name)
                {
                    LOG_DEBUG_F("Found an existing intervention of type '%s' with name '%s'. Purging.\n", type_name.c_str(), iv_name.c_str());
                    Remove(i);
                    break;
                }
            }
        }
    }

    bool InterventionsContainer::ContainsExistingByName( const InterventionName& rInputName )
    {
        for( auto& name : intervention_names )
        {
            if( name == rInputName )
            {
                return true;
            }
        }
        return false;
    }

    const std::vector<IDistributableIntervention*>& InterventionsContainer::GetInterventions() const
    {
        return interventions;
    }

    void InterventionsContainer::InfectiousLoopUpdate( float dt )
    {
        // --- ------------------------------------------------------------------------------------
        // --- The purpose of this method is to update the interventions that need to be updated
        // --- in the infectious update loop.  These interventions can be updated multiple times
        // --- per time step and affect the person's infections.  Drugs are typically interventions
        // --- that need to be in this loop.
        // --- ------------------------------------------------------------------------------------
        drugVaccineReducedAcquire   = 1.0;
        drugVaccineReducedTransmit  = 1.0;
        drugVaccineReducedMortality = 1.0;
        birth_rate_mod              = 1.0;
        non_disease_death_rate_mod  = 1.0;

        for( auto intervention : interventions )
        {
            if( intervention->NeedsInfectiousLoopUpdate() )
            {
                intervention->Update( dt );
            }
        }
    }

    void InterventionsContainer::Update( float dt )
    {
        if( !interventions.empty() )
        {
            // -----------------------------------------------------------------------
            // --- The "orig_num" check below is done below so that we update
            // --- interventions that were distributed by the existing interventions.
            // --- We need to update both types of new interventions.
            // -----------------------------------------------------------------------
            int orig_num = interventions.size();
            for( int i = 0; i < interventions.size(); ++i )
            {
                if( !interventions[i]->NeedsInfectiousLoopUpdate() || (i >= orig_num) )
                {
                    interventions[i]->Update( dt );
                }
            }

            for( int i = 0; i < interventions.size(); )
            {
                // ------------------------------------------------------------------------------
                // --- BEWARE: Some interventions distribute other interventions or fire events
                // --- (which can cause interventions to be added) during the call to Expired().
                // ------------------------------------------------------------------------------
                if( interventions[i]->Expired() )
                {
                    LOG_DEBUG("Found an expired intervention\n");
                    Remove( i );
                    // don't increment the index because another intervention could be at this index
                }
                else
                {
                    ++i;
                }
            }
        }
    }

    bool InterventionsContainer::GiveIntervention(
        IDistributableIntervention* iv
    )
    {
        ++numAdded;
        interventions.push_back( iv );
        intervention_names.push_back( iv->GetName() );
        // We need to increase the reference counter here to represent fact that interventions container
        // is keeping a pointer to the intervention. (Otherwise when event coordinator calls Release,
        // and ref counter is decremented, the intervention object will delete itself.)
        iv->AddRef();
        iv->SetContextTo( parent );
        // TODO: For vaccine, now vaccine intervention needs to call ApplyVaccineTake on itself (???)
        LOG_DEBUG_F("InterventionsContainer (individual %d) has %d interventions now\n", GetParent()->GetSuid().data, interventions.size());
        return true;
    }

    void InterventionsContainer::UpdateVaccineAcquireRate(
        float acquire,
        bool isMultiplicative
    )
    {
        if( isMultiplicative )
        {
            drugVaccineReducedAcquire *= (1.0f - acquire);
        }
        else
        {
            drugVaccineReducedAcquire -= acquire;
        }

        if( drugVaccineReducedAcquire > 1.0 )
        {
            drugVaccineReducedAcquire = 1.0;
        }
        else if( drugVaccineReducedAcquire < 0.0 )
        {
            LOG_WARN_F("drugVaccineReducedAcquire(=%f) < 0, setting to zero\n",drugVaccineReducedAcquire);
            drugVaccineReducedAcquire = 0.0;
        }
    }

    void InterventionsContainer::UpdateVaccineTransmitRate(
        float xmit,
        bool isMultiplicative
    )
    {
        if( isMultiplicative )
        {
            drugVaccineReducedTransmit *= (1.0f - xmit);
        }
        else
        {
            drugVaccineReducedTransmit -= xmit;
        }

        if( drugVaccineReducedTransmit > 1.0 )
        {
            drugVaccineReducedTransmit = 1.0;
        }
        else if( drugVaccineReducedTransmit < 0.0 )
        {
            LOG_WARN_F("drugVaccineReducedTransmit(=%f) < 0, setting to zero\n",drugVaccineReducedTransmit);
            drugVaccineReducedTransmit = 0.0;
        }
    }

    void InterventionsContainer::UpdateVaccineMortalityRate(
        float mort,
        bool isMultiplicative
    )
    {
        if( isMultiplicative )
        {
            drugVaccineReducedMortality *= (1.0f - mort);
        }
        else
        {
            drugVaccineReducedMortality -= mort;
        }

        if( drugVaccineReducedMortality > 1.0 )
        {
            drugVaccineReducedMortality = 1.0;
        }
        else if( drugVaccineReducedMortality < 0.0 )
        {
            LOG_WARN_F("drugVaccineReducedMortality(=%f) < 0, setting to zero\n",drugVaccineReducedMortality);
            drugVaccineReducedMortality = 0.0;
        }
    }

    void InterventionsContainer::UpdateBirthRateMod( float mod )
    {
        birth_rate_mod *= mod;
    }

    void InterventionsContainer::UpdateNonDiseaseDeathRateMod( float mod )
    {
        non_disease_death_rate_mod *= mod;
    }

    void InterventionsContainer::ChangeProperty(
        const char * property,
        const char * new_value
    )
    {
/*
        LOG_DEBUG_F( "ChangeProperty: property = %s, new_value = %s\n", property, new_value );
        if( strlen( property ) == 0 )
        {
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "ChangeProperty called with empty property string." );
        }
        if( strlen( new_value ) == 0 )
        {
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "ChangeProperty called with empty value string." );
        }
*/
        // Get parent property (remove need for casts)
        IPKeyValueContainer* pProps = parent->GetEventContext()->GetProperties();

        // Check that property exists, except Age_Bins which are special case. We bootstrap individuals into age_bins at t=1,
        // with no prior existing age bin property.
        IPKey key( property );
        if( ( std::string( property ) != "Age_Bin" ) && !pProps->Contains( key ) )
        {
            throw BadMapKeyException( __FILE__, __LINE__, __FUNCTION__, "properties", property );
        }

        IPKeyValue new_kv( property, new_value );

        if( !pProps->Contains( new_kv ) )
        {
            IPKeyValue old_kv = pProps->Get( key );
            LOG_DEBUG_F( "Moving individual (%lu) property %s from %s to %s\n", parent->GetSuid().data, property, old_kv.GetValueAsString().c_str(), new_value );
            pProps->Set( new_kv );
            parent->UpdateGroupMembership();
            m_LastIPChange = new_kv;

            //broadcast that the individual changed properties
            IIndividualEventBroadcaster* broadcaster = parent->GetEventContext()->GetNodeEventContext()->GetIndividualEventBroadcaster();
            LOG_DEBUG_F( "Individual %d changed property, broadcasting PropertyChange \n", parent->GetSuid().data );
            broadcaster->TriggerObservers( parent->GetEventContext(), EventTrigger::PropertyChange );
        }
    }

    uint32_t InterventionsContainer::GetNumInterventions() const
    {
        return interventions.size();
    }

    uint32_t InterventionsContainer::GetNumInterventionsAdded()
    {
        // ------------------------------------------------------------------------------------------
        // --- By clearing the value after getting it, we are attempting to ensure we include
        // --- the interventions that are added outside Update() like from StandardEventCoordinator.
        // ------------------------------------------------------------------------------------------
        int ret = numAdded;
        numAdded = 0;
        return ret;
    }

    const IPKeyValue& InterventionsContainer::GetLastIPChange() const
    {
        return m_LastIPChange;
    }

    void
    InterventionsContainer::SetContextTo(
        IIndividualHumanContext* context
    )
    {
        parent = context;
        if (parent)
        {
            PropagateContextToDependents();
        }
    }

    IIndividualHumanContext*
    InterventionsContainer::GetParent()
    {
        return parent;
    }

    float InterventionsContainer::GetInterventionReducedAcquire()   const { return drugVaccineReducedAcquire;   }
    float InterventionsContainer::GetInterventionReducedTransmit()  const { return drugVaccineReducedTransmit;  }
    float InterventionsContainer::GetInterventionReducedMortality() const { return drugVaccineReducedMortality; }
    float InterventionsContainer::GetBirthRateMod()                 const { return birth_rate_mod;              }
    float InterventionsContainer::GetNonDiseaseDeathRateMod()       const { return non_disease_death_rate_mod;  }

    REGISTER_SERIALIZABLE(InterventionsContainer);

    void InterventionsContainer::serialize(IArchive& ar, InterventionsContainer* obj)
    {
        InterventionsContainer& container = *obj;
        ar.labelElement( "drugVaccineReducedAcquire"   ) & container.drugVaccineReducedAcquire;
        ar.labelElement( "drugVaccineReducedTransmit"  ) & container.drugVaccineReducedTransmit;
        ar.labelElement( "drugVaccineReducedMortality" ) & container.drugVaccineReducedMortality;
        ar.labelElement( "birth_rate_mod"              ) & container.birth_rate_mod;
        ar.labelElement( "non_disease_death_rate_mod"  ) & container.non_disease_death_rate_mod;
        ar.labelElement( "interventions"               ) & container.interventions;
        ar.labelElement( "m_LastIPChange"              ) & container.m_LastIPChange;

        if( ar.IsReader() )
        {
            container.intervention_names.clear();
            for( auto intervention : container.interventions )
            {
                container.intervention_names.push_back( intervention->GetName() );
            }
        }
    }

}
