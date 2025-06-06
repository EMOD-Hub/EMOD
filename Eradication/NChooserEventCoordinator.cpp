
#include "stdafx.h"

#include "NChooserEventCoordinator.h"
#include "InterventionFactory.h"
#include "Environment.h"
#include "RANDOM.h"
#include "IIndividualHumanSTI.h"
#include "IIndividualHumanHIV.h"
#include "IHIVInterventionsContainer.h"
#include "IdmDateTime.h"
#include "SimulationEventContext.h"

SETUP_LOGGING( "NChooserEventCoordinator" )

namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- AgeRange
    // ------------------------------------------------------------------------
    BEGIN_QUERY_INTERFACE_BODY(AgeRange)
    END_QUERY_INTERFACE_BODY(AgeRange)


    AgeRange::AgeRange( float minYears, float maxYears )
    : JsonConfigurable()
    , m_MinYears(minYears)
    , m_MaxYears(maxYears)
    {
    }

    AgeRange::~AgeRange()
    {
    }

    bool AgeRange::operator<( const AgeRange& rThat ) const
    {
        return (this->m_MinYears < rThat.m_MinYears);
    }

    bool AgeRange::Configure( const Configuration * inputJson )
    {
        initConfigTypeMap("Min", &m_MinYears, NC_AR_Min_DESC_TEXT, 0.0, MAX_HUMAN_AGE, 0.0 );
        initConfigTypeMap("Max", &m_MaxYears, NC_AR_Max_DESC_TEXT, 0.0, MAX_HUMAN_AGE, MAX_HUMAN_AGE );

        bool ret = JsonConfigurable::Configure( inputJson );
        if( ret && !JsonConfigurable::_dryrun )
        {
            if( m_MinYears >= m_MaxYears )
            {
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Min", m_MinYears, "Max", m_MaxYears, "Min must be < Max" );
            }
        }
        return ret;
    }

    float AgeRange::GetMinYear() const
    {
        return m_MinYears;
    }

    float AgeRange::GetMaxYear() const
    {
        return m_MaxYears;
    }

    bool AgeRange::IsInRange( float ageYears ) const
    {
        return ((m_MinYears <= ageYears) && (ageYears < m_MaxYears));
    }

    // ------------------------------------------------------------------------
    // --- AgeRangeList
    // ------------------------------------------------------------------------

    AgeRangeList::AgeRangeList()
        : JsonConfigurableCollection("AgeRangeList")
    {
    }

    AgeRangeList::~AgeRangeList()
    {
    }

    bool LessThan( AgeRange* pLeft, AgeRange* pRight )
    {
        return (pLeft->GetMinYear() < pRight->GetMinYear());
    }

    void AgeRangeList::CheckConfiguration()
    {
        if( Size() == 0 )
        {
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, "'Age_Ranges_Years' cannot have zero elements." );
        }
        std::sort( m_Collection.begin(), m_Collection.end(), LessThan );

        float prev_min = m_Collection[0]->GetMinYear();
        float prev_max = m_Collection[0]->GetMaxYear();
        for( int i = 1 ; i < m_Collection.size() ; ++i )
        {
            float this_min = m_Collection[i]->GetMinYear();
            float this_max = m_Collection[i]->GetMaxYear();

            if( prev_max > this_min )
            {
                std::stringstream ss;
                ss << "'Age_Ranges_Years' cannot have age ranges that overlap.  ";
                ss << "(" << prev_min << ", " << prev_max << ") vs (" << this_min << ", " << this_max << ")";
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
        }
    }

    AgeRange* AgeRangeList::CreateObject()
    {
        AgeRange* p_ar = new AgeRange();
        return p_ar;
    }

    // ------------------------------------------------------------------------
    // --- TargetedByAgeAndGender
    // ------------------------------------------------------------------------

    TargetedByAgeAndGender::TargetedByAgeAndGender( const AgeRange& rar, 
                                                    Gender::Enum gender, 
                                                    int numTargeted, 
                                                    int numTimeSteps, 
                                                    int initialTimeStep )
    : m_AgeRange( rar )
    , m_Gender( gender )
    , m_NumTargeted( numTargeted )
    , m_TimeStep( initialTimeStep )
    , m_NumTargetedPerTimeStep()
    , m_QualifyingIndividuals()
    {
        release_assert( initialTimeStep < numTimeSteps );

        int avg_tgt = m_NumTargeted / numTimeSteps;
        int remainder = m_NumTargeted - (avg_tgt * numTimeSteps);

        for( int i = 0 ; i < numTimeSteps; ++i )
        {
            m_NumTargetedPerTimeStep.push_back( avg_tgt );
        }
        for( int i = 0 ; i < remainder; ++i )
        {
            m_NumTargetedPerTimeStep[i] += 1;
        }
        m_QualifyingIndividuals.reserve( m_NumTargeted );
    }

    TargetedByAgeAndGender::~TargetedByAgeAndGender()
    {
    }

    void TargetedByAgeAndGender::IncrementNextNumTargets()
    {
        ++m_TimeStep;
    }

    int TargetedByAgeAndGender::GetNumTargeted() const
    {
        release_assert( m_TimeStep < m_NumTargetedPerTimeStep.size() );
        return m_NumTargetedPerTimeStep[ m_TimeStep ];
    }

    void TargetedByAgeAndGender::ClearQualifyingIndividuals()
    {
        m_QualifyingIndividuals.clear();
    }

    void TargetedByAgeAndGender::FindQualifyingIndividuals( INodeEventContext* pNEC,
                                                            const DiseaseQualifications& rDisease,
                                                            PropertyRestrictions<IPKey, IPKeyValue, IPKeyValueContainer>& rPropertyRestrictions )
    {
        INodeEventContext::individual_visit_function_t fn =
            [ this, &rDisease, &rPropertyRestrictions ](IIndividualHumanEventContext *ihec)
        {
            if( !m_AgeRange.IsInRange( ihec->GetAge()/DAYSPERYEAR ) ) return;

            if( (m_Gender != Gender::COUNT) && (m_Gender != ihec->GetGender()) ) return;

            if( !rPropertyRestrictions.Qualifies( *(ihec->GetProperties()) ) ) return;

            if( !rDisease.Qualifies( ihec ) ) return;

            m_QualifyingIndividuals.push_back( ihec );
        };

        pNEC->VisitIndividuals( fn );
    }

    std::vector<IIndividualHumanEventContext*> TargetedByAgeAndGender::SelectIndividuals( RANDOMBASE* pRNG )
    {
        if( GetNumTargeted() >= m_QualifyingIndividuals.size() )
        {
            return m_QualifyingIndividuals;
        }

        // ----------------------------------------------------------------------------------
        // --- Robert Floyd's Algorithm for Sampling without Replacement
        // --- http://www.nowherenearithaca.com/2013/05/robert-floyds-tiny-and-beautiful.html
        // ----------------------------------------------------------------------------------

        uint32_t N = m_QualifyingIndividuals.size();
        uint32_t M = GetNumTargeted();
        std::set<uint32_t> selected_indexes = pRNG->chooseMofN( M, N );

        std::vector<IIndividualHumanEventContext*> selected_individuals;
        for( auto index : selected_indexes )
        {
            selected_individuals.push_back( m_QualifyingIndividuals[ index ] );
        }

        return selected_individuals;
    }

    bool TargetedByAgeAndGender::IsFinished() const
    {
        if( (m_TimeStep != 0) && (m_TimeStep >= (m_NumTargetedPerTimeStep.size()-1)) )
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    // ------------------------------------------------------------------------
    // --- TargetedDistribution
    // ------------------------------------------------------------------------

    BEGIN_QUERY_INTERFACE_BODY(TargetedDistribution)
    END_QUERY_INTERFACE_BODY(TargetedDistribution)

    bool TargetedDistribution::LeftLessThanRight( const TargetedDistribution* pLeft, const TargetedDistribution* pRight )
    {
        return *pLeft < *pRight;
    }

    TargetedDistribution::TargetedDistribution( NChooserObjectFactory* pObjectFactory )
    : JsonConfigurable()
    , m_pObjectFactory( pObjectFactory )
    , m_pDiseaseQualifications( nullptr )
    , m_StartDay(0.0)
    , m_EndDay(FLT_MAX)
    , m_PropertyRestrictions()
    , m_AgeRangeList()
    , m_NumTargeted()
    , m_NumTargetedMales()
    , m_NumTargetedFemales()
    {
    }

    TargetedDistribution::~TargetedDistribution()
    {
    }

    bool TargetedDistribution::operator<( const TargetedDistribution& rThat ) const
    {
        return (this->m_StartDay < rThat.m_StartDay);
    }

    void TargetedDistribution::AddTimeConfiguration()
    {
        initConfigTypeMap("Start_Day", &m_StartDay, NC_TD_Start_Day_DESC_TEXT, 0.0,  FLT_MAX, 0.0 );
        initConfigTypeMap("End_Day",   &m_EndDay,   NC_TD_End_Day_DESC_TEXT,   0.0,  FLT_MAX, FLT_MAX );
    }

    void TargetedDistribution::CheckTimePeriod() const
    {
        if( m_StartDay >= m_EndDay )
        {
            throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, 
                "Start_Day", m_StartDay,
                "End_Day", m_EndDay,
                "Start_Day must be < End_Day" );
        }
    }

    bool TargetedDistribution::Configure( const Configuration * inputJson )
    {
        AddTimeConfiguration();
        AddDiseaseConfiguration();

        initConfigTypeMap("Num_Targeted",         &m_NumTargeted,        NC_TD_Num_Targeted_DESC_TEXT,              0, INT_MAX,      0 );
        initConfigTypeMap("Num_Targeted_Males",   &m_NumTargetedMales,   NC_TD_Num_Targeted_Males_DESC_TEXT,        0, INT_MAX,      0 );
        initConfigTypeMap("Num_Targeted_Females", &m_NumTargetedFemales, NC_TD_Num_Targeted_Females_DESC_TEXT,      0, INT_MAX,      0 );

        initConfigComplexCollectionType("Age_Ranges_Years", &m_AgeRangeList, NC_TD_Age_Ranges_Years_DESC_TEXT );
        initConfigComplexType("Property_Restrictions_Within_Node", &m_PropertyRestrictions, Property_Restrictions_Within_Node_DESC_TEXT );

        bool ret = JsonConfigurable::Configure( inputJson );
        if( ret && !JsonConfigurable::_dryrun )
        {
            CheckTimePeriod();

            if( (m_NumTargeted.size() > 0) && ((m_NumTargetedMales.size() > 0) || (m_NumTargetedFemales.size() > 0)) )
            {
                std::stringstream ss;
                ss << "The number of elements in 'Num_Targeted' is " << m_NumTargeted.size() << ".\n";
                ss << "The number of elements in 'Num_Targeted_Males' is " << m_NumTargetedMales.size() << ".\n";
                ss << "The number of elements in 'Num_Targeted_Females' is " << m_NumTargetedFemales.size() << ".\n";
                ss << "If using Num_Targeted, then Num_Targeted_Males and Num_Targeted_Females must be empty.\nIf using Num_Targeted_Males and Num_Targeted_Females, then Num_Targeted must be empty.";
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }

            if( (m_NumTargeted.size() > 0) && (m_AgeRangeList.Size() != m_NumTargeted.size()) )
            {
                std::stringstream ss;
                ss << "The number of elements in 'Num_Targeted'(=" << m_NumTargeted.size() << ") is not the same as 'Age_Ranges_Years'(=" << m_AgeRangeList.Size() << ").\n";
                ss << "'Num_Targeted' and 'Age_Range_Years' must have the same number of elements, but not zero.  There must be one age range for each number targeted.";
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }

            if( ((m_NumTargetedMales.size() > 0) || (m_NumTargetedFemales.size() > 0)) && ((m_AgeRangeList.Size() != m_NumTargetedMales.size()) || (m_AgeRangeList.Size() != m_NumTargetedFemales.size())) )
            {
                std::stringstream ss;
                ss << "The number of elements in 'Num_Targeted_Males' is " << m_NumTargetedMales.size() << ".\n";
                ss << "The number of elements in 'Num_Targeted_Females' is " << m_NumTargetedFemales.size() << ".\n";
                ss << "The number of elements in 'Age_Range_Years' is " << m_AgeRangeList.Size() << ".\n";
                ss << "'Num_Targeted_Males', 'Num_Targeted_Females', and 'Age_Range_Years' must have the same number of elements, but not zero.  There must be one age range for each number targeted.";
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }

            if( (m_AgeRangeList.Size() == 0) && (m_NumTargeted.size() == 0) && (m_NumTargetedMales.size() == 0) && (m_NumTargetedFemales.size() == 0) )
            {
                std::stringstream ss;
                ss << "The arrays 'Age_Range_Years', 'Num_Targeted_Males', and 'Num_Targeted_Females' have zero elements.\n";
                ss << "Num_Targeted_Males, Num_Targeted_Females, and Age_Range_Years must have the same number of elements, but not zero.  There must be one age range for each number targeted.";
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }

            CheckForZeroTargeted();

            CheckDiseaseConfiguration();

            m_AgeRangeList.CheckConfiguration();
        }
        return ret;
    }


    void TargetedDistribution::ScaleTargets( float popScaleFactor )
    {
        if( popScaleFactor == 1.0 )
        {
            return;
        }
        if( m_NumTargeted.size() > 0 )
        {
            int num_total = 0;
            for( int i = 0 ; i < m_NumTargeted.size() ; ++i )
            {
                m_NumTargeted[i] *= popScaleFactor;
                num_total += m_NumTargeted[i];
            }
            if( num_total == 0 )
            {
                std::stringstream msg;
                msg << "The x_Base_Population (" << popScaleFactor << ") has scaled the values of Num_Targets all to zero so won't target anyone.\n";
                LOG_WARN( msg.str().c_str() ); //GH-605 - Change to warning so users can more easily test configuration.
            }
        }
        else
        {
            int num_total = 0;
            for( int i = 0 ; i < m_NumTargetedMales.size() ; ++i )
            {
                m_NumTargetedMales[i] *= popScaleFactor;
                m_NumTargetedFemales[i] *= popScaleFactor;
                num_total += m_NumTargetedMales[i];
                num_total += m_NumTargetedFemales[i];
            }
            if( num_total == 0 )
            {
                std::stringstream msg;
                msg << "The x_Base_Population (" << popScaleFactor << ") has scaled the values of Num_Targets_Males and Num_Target_Females all to zero so won't target anyone.\n";
                LOG_WARN( msg.str().c_str() ); //GH-605 - Change to warning so users can more easily test configuration.
            }
        }
    }

    void TargetedDistribution::CheckForZeroTargeted()
    {
        if( m_NumTargeted.size() > 0 )
        {
            int total = 0;
            for( auto num : m_NumTargeted )
            {
                total += num;
            }
            if( total == 0 )
            {
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, "'Num_Targeted' has zero values and won't target anyone." );
            }
        }
        else
        {
            release_assert( m_NumTargetedMales.size() == m_NumTargetedFemales.size() );

            int total_males = 0;
            int total_females = 0;
            for( int i = 0 ; i < m_NumTargetedMales.size() ; ++i )
            {
                total_males   += m_NumTargetedMales[i];
                total_females += m_NumTargetedFemales[i];
            }
            if( (total_males + total_females) == 0 )
            {
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, "'Num_Targeted_Males' and 'Num_Targeted_Females' have zero values and won't target anyone." );
            }
        }
    }

    float TargetedDistribution::GetStartInDays() const
    {
        return m_StartDay;
    }

    float TargetedDistribution::GetEndInDays() const
    {
        return m_EndDay;
    }

    float TargetedDistribution::GetCurrentInDays( const IdmDateTime& rDateTime ) const
    {
        return rDateTime.time;
    }

    void TargetedDistribution::CreateAgeAndGenderList( const IdmDateTime& rDateTime, float dt )
    {
        float start_days = GetStartInDays();
        float end_days   = GetEndInDays();
        int num_time_steps = int( (end_days - start_days) / dt );

        float current_days = GetCurrentInDays( rDateTime );
        int current_time_step = int( (current_days - start_days) / dt );

        for( int i = 0 ; i < m_AgeRangeList.Size(); ++i )
        {
            if( m_NumTargeted.size() > 0 )
            {
                if( m_NumTargeted[i] > 0 )
                {
                    TargetedByAgeAndGender* p_tbag = m_pObjectFactory->CreateTargetedByAgeAndGender( *m_AgeRangeList[i],
                                                                                                     Gender::COUNT, 
                                                                                                     m_NumTargeted[i],
                                                                                                     num_time_steps,
                                                                                                     current_time_step );
                    m_AgeAndGenderList.push_back( p_tbag );
                }
            }
            else
            {
                if( m_NumTargetedMales[i] > 0 )
                {
                    TargetedByAgeAndGender* p_tbag = m_pObjectFactory->CreateTargetedByAgeAndGender( *m_AgeRangeList[i],
                                                                                                     Gender::MALE, 
                                                                                                     m_NumTargetedMales[i],
                                                                                                     num_time_steps,
                                                                                                     current_time_step );
                    m_AgeAndGenderList.push_back( p_tbag );
                }
                if( m_NumTargetedFemales[i] > 0 )
                {
                    TargetedByAgeAndGender* p_tbag = m_pObjectFactory->CreateTargetedByAgeAndGender( *m_AgeRangeList[i],
                                                                                                     Gender::FEMALE, 
                                                                                                     m_NumTargetedFemales[i],
                                                                                                     num_time_steps,
                                                                                                     current_time_step );
                    m_AgeAndGenderList.push_back( p_tbag );
                }
            }
        }
        // ----------------------------------------------------------------------------------
        // --- DMB 9/26/2016 - I want this to be an error but users need the ability to test
        // --- set x_Base_Population very small in order to test other things. 
        // --- Hence, we need warning. (GH-605)
        // ----------------------------------------------------------------------------------
        if( m_AgeAndGenderList.size() == 0 )
        {
            LOG_WARN("Have zero individuals targeted.\n");
        }
    }

    void TargetedDistribution::UpdateTargeting( const IdmDateTime& rDateTime, float dt )
    {
        if( m_AgeAndGenderList.size() == 0 )
        {
            CreateAgeAndGenderList( rDateTime, dt );
        }
        else
        {
            for( auto p_ag : m_AgeAndGenderList )
            {
                p_ag->IncrementNextNumTargets();
            }
        }
    }

    std::vector< IIndividualHumanEventContext* >
    TargetedDistribution::DetermineWhoGetsIntervention( RANDOMBASE* pRNG, const std::vector<INodeEventContext*> nodeList )
    {
        // ---------------------------------------------------------
        // --- Find the individuals for each age and gender
        // --- that meet the demographic restrictions
        // ---------------------------------------------------------
        if( m_pDiseaseQualifications == nullptr )
        {
            m_pDiseaseQualifications = m_pObjectFactory->CreateDiseaseQualifications( this );
            release_assert( m_pDiseaseQualifications );
        }

        for( auto p_ag : m_AgeAndGenderList )
        {
            p_ag->ClearQualifyingIndividuals();

            for( auto p_nec : nodeList )
            {
                p_ag->FindQualifyingIndividuals( p_nec, *m_pDiseaseQualifications, m_PropertyRestrictions );
            }
        }

        // ----------------------------------------------------------------------
        // --- Of those that qualify, pick those that should get the intervention
        // ----------------------------------------------------------------------

        // Determine the total number of individuals that can receive the intervention
        int num_total = 0;
        for( auto p_ag : m_AgeAndGenderList )
        {
            num_total += p_ag->GetNumTargeted();
        }

        // Create the vector to return and allocate space for the individuals
        std::vector< IIndividualHumanEventContext* > distribute_to;
        distribute_to.reserve( num_total );

        for( auto p_ag : m_AgeAndGenderList )
        {
            std::vector< IIndividualHumanEventContext* > selected = p_ag->SelectIndividuals( pRNG );
            distribute_to.insert( distribute_to.end(), selected.begin(), selected.end() );
        }
        return distribute_to;
    }

    bool TargetedDistribution::IsFinished() const
    {
        if( m_AgeAndGenderList.size() == 0 )
        {
            // haven't started yet
            return false;
        }
        bool is_finished = true;

        for( int i = 0 ; is_finished && (i < m_AgeAndGenderList.size()) ; ++i )
        {
            is_finished = m_AgeAndGenderList[i]->IsFinished();
        }
        return is_finished;
    }

    void TargetedDistribution::CheckOverlaped( const TargetedDistribution& rPrev ) const
    {
        if( rPrev.m_EndDay > this->m_StartDay )
        {
            std::stringstream ss;
            ss << "'Distributions' cannot have time periods that overlap.  ";
            ss << "(" << rPrev.m_StartDay << ", " << rPrev.m_EndDay << ") vs (" << this->m_StartDay << ", " << this->m_EndDay << ")";
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
    }

    bool TargetedDistribution::IsPastStart( const IdmDateTime& rDateTime ) const
    {
        return m_StartDay <= rDateTime.time;
    }

    bool TargetedDistribution::IsPastEnd( const IdmDateTime& rDateTime ) const
    {
        return m_EndDay <= rDateTime.time;
    }

    void TargetedDistribution::CheckStartDay( float campaignStartDay ) const
    {
        if( GetStartInDays() < campaignStartDay )
        {
            LOG_WARN_F("Campaign starts on day %f. A NChooserEventCoordiator distribution starts before on day %f.  It will not distribute the targeted amount.\n",
                        campaignStartDay, GetStartInDays() );
        }
    }

    // ------------------------------------------------------------------------
    // --- TargetedDistributionList
    // ------------------------------------------------------------------------

    TargetedDistributionList::TargetedDistributionList( NChooserObjectFactory* pObjectFactory )
    : JsonConfigurableCollection( "TargetedDistributionList" )
    , m_pObjectFactory( pObjectFactory )
    , m_CurrentIndex(0)
    , m_pCurrentTargets(nullptr)
    {
        release_assert( m_pObjectFactory );
    }

    TargetedDistributionList::~TargetedDistributionList()
    {
    }

    TargetedDistribution* TargetedDistributionList::CreateObject()
    {
        TargetedDistribution* p_td = m_pObjectFactory->CreateTargetedDistribution();
        return p_td;
    }

    void TargetedDistributionList::CheckConfiguration()
    {
        if( Size() == 0 )
        {
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, "'Distributions' cannot have zero elements." );
        }
        std::sort( m_Collection.begin(), m_Collection.end(), TargetedDistribution::LeftLessThanRight );

        for( int i = 1 ; i < m_Collection.size() ; ++i )
        {
            m_Collection[ i ]->CheckOverlaped( *m_Collection[i-1] );
        }
    }

    void TargetedDistributionList::UpdateTargeting( const IdmDateTime& rDateTime, float dt )
    {
        // ----------------------------------------------------
        // --- Update where we are in the list of distributions
        // ----------------------------------------------------
        while( (m_CurrentIndex < m_Collection.size()) &&
               m_Collection[ m_CurrentIndex ]->IsPastEnd( rDateTime ) )
        {
            ++m_CurrentIndex;
        }

        // -----------------------------------------
        // --- Find the current distributino period
        // -----------------------------------------
        m_pCurrentTargets = nullptr;
        if( (m_CurrentIndex < m_Collection.size()) &&
            (m_Collection[ m_CurrentIndex ]->IsPastStart( rDateTime )) )
        {
            m_pCurrentTargets = m_Collection[ m_CurrentIndex ];

            m_pCurrentTargets->UpdateTargeting( rDateTime, dt );
        }
    }

    TargetedDistribution* TargetedDistributionList::GetCurrentTargets()
    {
        return m_pCurrentTargets;
    }

    bool TargetedDistributionList::IsFinished( const IdmDateTime& rDateTime, float dt )
    {
        bool is_finished = false;
        while( (m_CurrentIndex < m_Collection.size()) &&
               ( (m_Collection[ m_CurrentIndex ]->IsPastEnd( rDateTime )) ||
                 m_Collection[ m_CurrentIndex ]->IsFinished() ) )
        {
            ++m_CurrentIndex;
        }

        if( m_CurrentIndex >= m_Collection.size() )
        {
            is_finished = true;
        }
        return is_finished;
    }

    void TargetedDistributionList::ScaleTargets( float popScaleFactor )
    {
        for( auto p_td : m_Collection )
        {
            p_td->ScaleTargets( popScaleFactor );
        }
    }

    void TargetedDistributionList::CheckStartDay( float campaignStartDay ) const
    {
        for( auto p_td : m_Collection )
        {
            p_td->CheckStartDay( campaignStartDay );
        }
    }


    // ------------------------------------------------------------------------
    // --- NChooserObjectFactory
    // ------------------------------------------------------------------------

    NChooserObjectFactory::NChooserObjectFactory()
    {
    }

    NChooserObjectFactory::~NChooserObjectFactory()
    {
    }

    TargetedDistribution* NChooserObjectFactory::CreateTargetedDistribution()
    {
        return new TargetedDistribution( this );
    }

    TargetedByAgeAndGender* NChooserObjectFactory::CreateTargetedByAgeAndGender( const AgeRange& rar, 
                                                                                 Gender::Enum gender, 
                                                                                 int numTargeted, 
                                                                                 int numTimeSteps, 
                                                                                 int initialTimeStep )
    {
        return new TargetedByAgeAndGender( rar, gender, numTargeted, numTimeSteps, initialTimeStep );
    }

    DiseaseQualifications*  NChooserObjectFactory::CreateDiseaseQualifications( TargetedDistribution* ptd )
    {
        return new DiseaseQualifications();
    }

    // ------------------------------------------------------------------------
    // --- NChooserEventCoordinator
    // ------------------------------------------------------------------------

    IMPLEMENT_FACTORY_REGISTERED(NChooserEventCoordinator)
    IMPL_QUERY_INTERFACE2(NChooserEventCoordinator, IEventCoordinator, IConfigurable)

    NChooserEventCoordinator::NChooserEventCoordinator()
    : m_Parent( nullptr )
    , m_pObjectFactory( new NChooserObjectFactory() )
    , m_CachedNodes()
    , m_InterventionName()
    , m_pIntervention( nullptr )
    , m_TargetedDistributionList( m_pObjectFactory )
    , m_DistributionIndex(0)
    , m_IsFinished(false)
    , m_HasBeenScaled(false)
    {
        release_assert( m_pObjectFactory );
    }

    NChooserEventCoordinator::NChooserEventCoordinator( NChooserObjectFactory* pObjectFactory )
    : m_Parent( nullptr )
    , m_pObjectFactory( pObjectFactory )
    , m_CachedNodes()
    , m_InterventionName()
    , m_pIntervention( nullptr )
    , m_TargetedDistributionList( m_pObjectFactory )
    , m_DistributionIndex(0)
    , m_IsFinished(false)
    , m_HasBeenScaled(false)
    {
        release_assert( m_pObjectFactory );
    }

    NChooserEventCoordinator::~NChooserEventCoordinator()
    {
        delete m_pObjectFactory;
        delete m_pIntervention;
    }


    bool NChooserEventCoordinator::Configure( const Configuration * inputJson )
    {
        InterventionConfig intervention_config;
        initConfigComplexCollectionType( "Distributions", &m_TargetedDistributionList, NC_Distributions_DESC_TEXT );
        initConfigComplexType( "Intervention_Config", &intervention_config, Intervention_Config_DESC_TEXT );

        bool retValue = JsonConfigurable::Configure( inputJson );

        if( retValue && !JsonConfigurable::_dryrun)
        {
            m_TargetedDistributionList.CheckConfiguration();

            m_pIntervention = InterventionFactory::CreateIntervention( intervention_config._json, inputJson->GetDataLocation(), "Intervention_Config", true );

            m_InterventionName = std::string( json::QuickInterpreter(intervention_config._json)["class"].As<json::String>() );
        }

        return retValue;
    }

    void NChooserEventCoordinator::SetContextTo( ISimulationEventContext *isec )
    {
        m_Parent = isec;
    }

    void NChooserEventCoordinator::CheckStartDay( float campaignStartDay ) const
    {
        m_TargetedDistributionList.CheckStartDay( campaignStartDay );
    }

    void NChooserEventCoordinator::AddNode( const suids::suid& node_suid )
    {
        INodeEventContext* pNEC = m_Parent->GetNodeEventContext( node_suid );
        if( !m_HasBeenScaled )
        {
            m_TargetedDistributionList.ScaleTargets( pNEC->GetNodeContext()->GetBasePopulationScaleFactor() );
            m_HasBeenScaled = true;
        }
        m_CachedNodes.push_back( pNEC );
    }

    void NChooserEventCoordinator::Update( float dt )
    {
        // --------------------------------------------------------------------------------------
        // --- Update who is to be targeted
        // --- NOTE: We can't determine who gets the intervention because things could change
        // --- before UpdateNodes() is called.  However, we can determine what we are targeting.
        // --------------------------------------------------------------------------------------
        m_TargetedDistributionList.UpdateTargeting( m_Parent->GetSimulationTime(), dt );
    }

    void NChooserEventCoordinator::UpdateNodes( float dt )
    {
        release_assert( m_CachedNodes.size() > 0 );

        // ---------------------------------------
        // --- Determine who gets the intervention
        // ---------------------------------------
        TargetedDistribution* p_current_targets = m_TargetedDistributionList.GetCurrentTargets();

        if( p_current_targets != nullptr ) // can be nullptr if in between periods
        {
            std::vector< IIndividualHumanEventContext *> individual_list = p_current_targets->DetermineWhoGetsIntervention( m_CachedNodes[ 0 ]->GetRng(), m_CachedNodes );

            // ------------------------------------------------------------------------------------------------------
            // --- Distribute the intervention
            // --- WARNING: This does not use the IVisitIndividual interface method of distributing the intervention
            // --- like StandardInterventionDistributionEventCoordinator.  This was done for performance reasons.
            // ------------------------------------------------------------------------------------------------------
            for( auto pIHEC : individual_list )
            {
                IDistributableIntervention *di = m_pIntervention->Clone();
                release_assert(di);

                ICampaignCostObserver* p_icco = nullptr;
                if (s_OK != pIHEC->GetNodeEventContext()->QueryInterface(GET_IID(ICampaignCostObserver), (void**)&p_icco))
                {
                    throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "pIHEC->GetNodeEventContext()", "ICampaignCostObserver", "INodeEventContext" );
                }

                di->AddRef();
                di->Distribute( pIHEC->GetInterventionsContext(), p_icco );
                di->Release();
            }

            std::stringstream ss;
            ss << "UpdateNodes() gave out " << individual_list.size() << " '" << m_InterventionName.c_str() << "' interventions\n";
            LOG_INFO( ss.str().c_str() );
        }

        // ----------------------------------------------------
        // --- Determine if finished distributing interventions
        // ----------------------------------------------------
        m_IsFinished = m_TargetedDistributionList.IsFinished(  m_Parent->GetSimulationTime(), dt );
    }

    bool NChooserEventCoordinator::IsFinished()
    {
        return m_IsFinished;
    }
}

