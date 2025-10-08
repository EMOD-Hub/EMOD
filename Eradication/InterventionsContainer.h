
#pragma once

#include <list>

#include "Interventions.h"

namespace Kernel
{
    // this container becomes a help implementation member of the relevant IndividualHuman class 
    // it needs to implement consumer interfaces for all the relevant intervention types
    // InterventionsContainer is a generic 

    struct IBirthRateModifier : public ISupports
    {
        virtual void UpdateBirthRateMod( float mod ) = 0;
    };

    struct INonDiseaseDeathRateModifier : public ISupports
    {
        virtual void UpdateNonDiseaseDeathRateMod( float mod ) = 0;
    };

    struct IVaccineConsumer : public ISupports
    {
        virtual void UpdateVaccineAcquireRate(   float acq,  bool isMultiplicative = true ) = 0;
        virtual void UpdateVaccineTransmitRate(  float xmit, bool isMultiplicative = true ) = 0;
        virtual void UpdateVaccineMortalityRate( float mort, bool isMultiplicative = true ) = 0;
    };

    struct IDrugVaccineInterventionEffects : public ISupports
    {
        virtual float GetInterventionReducedAcquire()   const = 0;
        virtual float GetInterventionReducedTransmit()  const = 0;
        virtual float GetInterventionReducedMortality() const = 0;
        virtual ~IDrugVaccineInterventionEffects() { }
    };

    class InterventionsContainer : public IIndividualHumanInterventionsContext
                                 , public IVaccineConsumer
                                 , public IInterventionConsumer
                                 , public IDrugVaccineInterventionEffects
                                 , public IBirthRateModifier
                                 , public INonDiseaseDeathRateModifier
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()

    public:
        InterventionsContainer();
        virtual ~InterventionsContainer();

        // IIndividualHumanInterventionsContext
        virtual void SetContextTo(IIndividualHumanContext* context) override;
        virtual IIndividualHumanContext* GetParent() override;
        virtual std::list<void*> GetInterventionsByInterface( iid_t iid ) override;
        virtual void PurgeExistingByType( const std::string& type_name ) override;
        virtual void PurgeExistingByName( const std::string& type_name, const InterventionName& name ) override;
        virtual bool ContainsExistingByName( const InterventionName& name ) override;
        virtual void ChangeProperty( const char *property, const char* new_value ) override;
        virtual const std::vector<IDistributableIntervention*>& GetInterventions() const override;
        virtual uint32_t GetNumInterventions() const override;
        virtual uint32_t GetNumInterventionsAdded() override;
        virtual const IPKeyValue& GetLastIPChange() const override;

        // IUnknown
        virtual QueryResult QueryInterface(iid_t iid, void** pinstance) override;

        // IBirthRateModifier
        virtual void UpdateBirthRateMod( float mod ) override;

        // INonDiseaseDeathRateModifier
        virtual void UpdateNonDiseaseDeathRateMod( float mod ) override;

        // IVaccineConsumer
        virtual void UpdateVaccineAcquireRate(   float acq,  bool isMultiplicative = true ) override;
        virtual void UpdateVaccineTransmitRate(  float xmit, bool isMultiplicative = true ) override;
        virtual void UpdateVaccineMortalityRate( float mort, bool isMultiplicative = true ) override;

        // IDrugVaccineInterventionEffects
        virtual float GetInterventionReducedAcquire()   const override;
        virtual float GetInterventionReducedTransmit()  const override;
        virtual float GetInterventionReducedMortality() const override;


        virtual bool GiveIntervention( IDistributableIntervention * pIV ) override;

        virtual void InfectiousLoopUpdate( float dt ); // update only interventions that need updating in Infectious Update loop
        virtual void Update( float dt ); // update non-infectious loop update interventions once per time step

        virtual float GetBirthRateMod() const;
        virtual float GetNonDiseaseDeathRateMod() const;

    protected:
        void Remove( int index );

        float drugVaccineReducedAcquire;
        float drugVaccineReducedTransmit;
        float drugVaccineReducedMortality;
        float birth_rate_mod;
        float non_disease_death_rate_mod;

        // ----------------------------------------------------------------------------------------
        // --- intervention_names is to be a parallel aray of names of the interventions.
        // --- The InterventionName object is really a shell around a pointer to the unique
        // --- instance of a string with the name.  This means the array of InterventionName
        // --- objects is really just an array of pointers that can be easily loaded into
        // --- cache - one time for all interventions.  This means you can search the array quickly.
        // --- When looping over the interventions themselves, we have to load each intervention
        // --- object into cache before we can get the name.  This also attempts to reduce time as
        // --- well as by doing a pointer comparision of the strings instead of an actual string compare.
        // ----------------------------------------------------------------------------------------
        std::vector<IDistributableIntervention*> interventions;
        std::vector<InterventionName> intervention_names;

        uint32_t numAdded; // number of interventions added this timestep

        virtual void PropagateContextToDependents(); // pass context to interventions if they need it

        IIndividualHumanContext *parent;    // context for this interventions container
        IPKeyValue m_LastIPChange;

    private:
        IDistributableIntervention* GetIntervention( const std::string& iv_name );

        DECLARE_SERIALIZABLE(InterventionsContainer);
    };
}
