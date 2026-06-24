
#include "stdafx.h"

#include "ReportMicrosporidia.h"

#include "report_params.rc"
#include "NodeEventContext.h"
#include "Individual.h"
#include "VectorContexts.h"
#include "ReportUtilities.h"
#include "INodeContext.h"
#include "IVectorCohort.h"
#include "IdmDateTime.h"

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!! CREATING NEW REPORTS
// !!! If you are creating a new report by copying this one, you will need to modify 
// !!! the values below indicated by "<<<"

// Name for logging, CustomReport.json, and DLL GetType()
SETUP_LOGGING( "ReportMicrosporidia" ) // <<< Name of this file

namespace Kernel
{
    // ----------------------------------------
    // --- ReportMicrosporidia Methods
    // ----------------------------------------

    BEGIN_QUERY_INTERFACE_DERIVED( ReportMicrosporidia, BaseTextReport )
        HANDLE_INTERFACE( IReport )
    END_QUERY_INTERFACE_DERIVED( ReportMicrosporidia, BaseTextReport )

    IMPLEMENT_FACTORY_REGISTERED( ReportMicrosporidia )

    ReportMicrosporidia::ReportMicrosporidia()
        : ReportMicrosporidia( "ReportMicrosporidia.csv" )
    {
    }

    ReportMicrosporidia::ReportMicrosporidia( const std::string& rReportName )
        : BaseTextReport( rReportName )
        , m_SpeciesList()
        , m_MsStrainNamesBySpecies()
    {
        initSimTypes( 2, "VECTOR_SIM", "MALARIA_SIM" );

        // ------------------------------------------------------------------------------------------------
        // --- Since this report will be listening for events, it needs to increment its reference count
        // --- so that it is 1.  Other objects will be AddRef'ing and Release'ing this report/observer
        // --- so it needs to start with a refcount of 1.
        // ------------------------------------------------------------------------------------------------
        AddRef();
    }

    ReportMicrosporidia::~ReportMicrosporidia()
    {
    }

    bool ReportMicrosporidia::Configure( const Configuration * inputJson )
    {
        bool ret = JsonConfigurable::Configure( inputJson );
        if( ret && !JsonConfigurable::_dryrun )
        {
        }
        return ret;
    }

    std::string ReportMicrosporidia::GetHeader() const
    {
        std::stringstream header ;

        header         << "Time"
               << "," << "NodeID"
               << "," << "Species"
               << "," << "MicrosporidiaStrain";

        header << "," << "VectorPopulation" ;
        for( int i = 0; i < VectorStateEnum::pairs::count(); ++i )
        {
            header << "," << VectorStateEnum::pairs::get_keys()[ i ];
        }

        return header.str();
    }

    void ReportMicrosporidia::LogNodeData( Kernel::INodeContext* pNC )
    {
        auto time      = pNC->GetTime().time ;
        auto nodeId    = pNC->GetExternalID();
        auto node_suid = pNC->GetSuid();
        
        INodeVector * pNodeVector = NULL;
        if (s_OK != pNC->QueryInterface(GET_IID(INodeVector), (void**)&pNodeVector) )
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "pNC", "INodeVector", "INodeContext");
        }
        const VectorPopulationReportingList_t& vector_pop_list = pNodeVector->GetVectorPopulationReporting();

        // ---------------------------------------------------------------------
        // --- if the list is empty, then we want to include all of the species
        // ---------------------------------------------------------------------
        if( m_SpeciesList.empty() )
        {
            for( auto vp : vector_pop_list )
            {
                m_SpeciesList.push_back( vp->get_SpeciesID() );
                m_MsStrainNamesBySpecies.push_back( vp->GetMicrosporidiaStrainNames() );
            }
        }

        int species_index = 0;
        for( auto vp : vector_pop_list )
        {
            const std::string& species = vp->get_SpeciesID();

            int strain_index = 0;
            for( auto& ms_strain_name : m_MsStrainNamesBySpecies[ species_index ] )
            {
                GetOutputStream()
                           << time
                    << "," << nodeId
                    << "," << species
                    << "," << ms_strain_name;

                uint32_t infectious = vp->getMicrosporidiaCount( strain_index, VectorStateEnum::STATE_INFECTIOUS );
                uint32_t infected   = vp->getMicrosporidiaCount( strain_index, VectorStateEnum::STATE_INFECTED   );
                uint32_t adult      = vp->getMicrosporidiaCount( strain_index, VectorStateEnum::STATE_ADULT      );
                uint32_t male       = vp->getMicrosporidiaCount( strain_index, VectorStateEnum::STATE_MALE       );
                uint32_t immature   = vp->getMicrosporidiaCount( strain_index, VectorStateEnum::STATE_IMMATURE   );
                uint32_t larva      = vp->getMicrosporidiaCount( strain_index, VectorStateEnum::STATE_LARVA      );
                uint32_t eggs       = vp->getMicrosporidiaCount( strain_index, VectorStateEnum::STATE_EGG        );

                uint32_t vec_pop = infectious + infected + adult;
                GetOutputStream()
                    << "," << vec_pop
                    << "," << infectious
                    << "," << infected  
                    << "," << adult     
                    << "," << male      
                    << "," << immature  
                    << "," << larva     
                    << "," << eggs
                    << "\n";
                ++strain_index;
            }
            ++species_index;
        }
    }
}