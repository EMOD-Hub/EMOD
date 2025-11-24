
#pragma once

#include "Configure.h"
#include "ISerializable.h"
#include "IAdditionalRestrictions.h"
#include "AdditionalRestrictionsFactory.h"

namespace Kernel
{
    class OrAndCollection : public IComplexJsonConfigurable
    {
    public:
        OrAndCollection();
        ~OrAndCollection();

        //IComplexJsonConfigurable
        virtual void ConfigureFromJsonAndKey( const Configuration* inputJson, const std::string& key );
        virtual json::QuickBuilder GetSchema();
        virtual bool  HasValidDefault() const;

        void CheckConfiguration( const char* parameterName );

    protected:
        friend class TargetingLogic;

        std::vector<std::vector<IAdditionalRestrictions*>> m_Restrictions;
        json::Object m_JsonSchemaBase;
    };

    class TargetingLogic : public JsonConfigurable, public IAdditionalRestrictions
    {
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED(AdditionalRestrictionsFactory,
                                   TargetingLogic,
                                   IAdditionalRestrictions)
        DECLARE_SERIALIZABLE(TargetingLogic)
        DECLARE_QUERY_INTERFACE()

    public:
        TargetingLogic();
        virtual ~TargetingLogic();

        virtual bool Configure(const Configuration* config) override;
        virtual bool IsQualified(IIndividualHumanEventContext* pContext) const override;

    private:
        bool m_CompareTo;
        OrAndCollection m_OrAndLogic;
    };
}