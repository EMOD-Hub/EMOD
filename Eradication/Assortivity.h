
#pragma once

#include <vector>
#include "IAssortivity.h"
#include "IRelationship.h"
#include "EnumSupport.h"
#include "Properties.h"

namespace Kernel 
{
    class RANDOMBASE;

    // The AssortivityGroup is used to specify the group of people that a person
    // might have an affinity for or against.
    ENUM_DEFINE(AssortivityGroup,
        ENUM_VALUE_SPEC(NO_GROUP                   , 0)  // pair with the first person in the list
        ENUM_VALUE_SPEC(STI_INFECTION_STATUS       , 1)  // pair with someone who is/isn't STI positive
        ENUM_VALUE_SPEC(INDIVIDUAL_PROPERTY        , 2)  // pair with someone who has an individual property that you like
        ENUM_VALUE_SPEC(STI_COINFECTION_STATUS     , 3)  // pair with someone who is/isn't STI Co-infected (only applies to HIV model)
        ENUM_VALUE_SPEC(HIV_INFECTION_STATUS       , 4)  // pair with someone who is/isn't HIV positive (based on infectivity) (only applies to HIV model)
        ENUM_VALUE_SPEC(HIV_TESTED_POSITIVE_STATUS , 5)  // pair with someone who is/isn't tested positive for HIV (only applies to HIV model)
        ENUM_VALUE_SPEC(HIV_RECEIVED_RESULTS_STATUS, 6)) // pair with someone who has received similar HIV test results (only applies to HIV model)


    class IDMAPI Assortivity : public IAssortivity
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING();
        DECLARE_QUERY_INTERFACE();
    public:
        Assortivity( RelationshipType::Enum relType=RelationshipType::TRANSITORY, RANDOMBASE *prng=nullptr );
        virtual ~Assortivity();

        const IPKey& GetPropertyKey() const { return m_PropertyKey ; }

        // -------------------------
        // --- IAssortivity Methods
        // -------------------------
        virtual bool Configure(const Configuration *config) override;

        // Update assortivity parameters/controls.
        virtual void Update( const IdmDateTime& rCurrentTime, float dt ) override;

        // Using the attributes of pPartnerA and the attributes of the potential partners, 
        // select a partner from the potentialPartnerList.
        // Return nullptr if a suitable partner was not found.
        // nullptr can be returned even if the list is not empty.
        virtual IIndividualHumanSTI* SelectPartner( IIndividualHumanSTI* pPartnerA,
                                                    const list<IIndividualHumanSTI*>& potentialPartnerList ) override;

        virtual void SetParameters( RANDOMBASE* prng ) override;
    protected:
        typedef std::function<int( const Assortivity*, const IIndividualHumanSTI* )> tGetIndexFunc;
        typedef std::function<std::string( const Assortivity*, const IIndividualHumanSTI* )> tGetStringValueFunc;

        IIndividualHumanSTI* Assortivity::SelectPartnerFromScore( float total_score,
                                                                  const std::vector<float>& rScores,
                                                                  const std::vector<IIndividualHumanSTI*>& rPartners );

        IIndividualHumanSTI* FindPartner( IIndividualHumanSTI* pPartnerA,
                                          const list<IIndividualHumanSTI*>& potentialPartnerList,
                                          tGetIndexFunc func);

        IIndividualHumanSTI* FindPartnerIP( IIndividualHumanSTI* pPartnerA,
                                            const list<IIndividualHumanSTI*>& potentialPartnerList,
                                            tGetStringValueFunc func);

        // Derived classes will add more options to the switch statement
        virtual IIndividualHumanSTI* SelectPartnerForExtendedGroups( AssortivityGroup::Enum group,
                                                                     IIndividualHumanSTI* pPartnerA,
                                                                     const list<IIndividualHumanSTI*>& potentialPartnerList );

        // This routine is called inside Configure() but before the data is completely read.
        virtual void AddConfigurationParameters( AssortivityGroup::Enum group, const Configuration *config ) {};

        // This routine is called inside Configure() but after the data has been read.
        // It allows the derived class to make sure values are correct for the given
        // Group type.
        virtual void CheckDerivedValues() {} ;

        // Return the value of the group read in
        AssortivityGroup::Enum GetGroup() const { return m_Group; };

        // Return the value to use based on calls to Update()
        virtual AssortivityGroup::Enum GetGroupToUse() const ;

        RelationshipType::Enum GetRelationshipType() const { return m_RelType ; }
        std::vector<std::string>& GetAxes() { return m_Axes; }

        int GetIndex( const std::string& rStringValue );
        void CheckAxesForTrueFalse();
        void CheckAxesForProperty();
        void CheckMatrix();
        void SortMatrixFalseTrue();
        bool UsesStartYear() const ;

        static std::string Assortivity::ValuesToString( const std::vector<std::string>& rList ) ;

        RelationshipType::Enum          m_RelType ;
        RANDOMBASE*                     m_pRNG ;
        AssortivityGroup::Enum          m_Group ;
        IPKey                           m_PropertyKey ;
        std::vector<std::string>        m_Axes ;
        std::vector<std::vector<float>> m_WeightingMatrix ;
        float                           m_StartYear;  // if current year is < start year, default to NO_GROUP
        bool                            m_StartUsing; // value is based on start year versus current year

        DECLARE_SERIALIZABLE(Assortivity);
    };
}