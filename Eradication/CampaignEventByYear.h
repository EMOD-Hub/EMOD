
#include "CampaignEvent.h"

namespace Kernel
{
    class CampaignEventByYear : public CampaignEvent
    {
        DECLARE_FACTORY_REGISTERED(CampaignEventFactory, CampaignEventByYear, IConfigurable)

    public:
        friend class CampaignEventFactory;
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()  
        DECLARE_QUERY_INTERFACE()

        CampaignEventByYear();
        virtual ~CampaignEventByYear();

        virtual bool Configure(const Configuration* config) override;
    };
}
