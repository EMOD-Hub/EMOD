
#include "stdafx.h"
#include "IMigrationInfo.h"
#include "Migration.h"
#include "SimulationConfig.h"
#ifndef DISABLE_VECTOR
#include "MigrationInfoVector.h"
#include "VectorParameters.h"
#endif

SETUP_LOGGING( "IMigrationInfo" )

namespace Kernel
{
    namespace MigrationFactory
    {
        IMigrationInfoFactory* ConstructMigrationInfoFactory( const ::Configuration *config, 
                                                              const std::string& idreference )
        {
            // -------------------------------------------------------------------------
            // --- Getting these via the global to be consisten with ConstructMigrationInfoFactoryVector
            // -------------------------------------------------------------------------
            bool use_default_demog = false;
            int default_torus_size = 0;

            if( GET_CONFIGURABLE( SimulationConfig ) != nullptr )
            {
                use_default_demog  = !GET_CONFIGURABLE( SimulationConfig )->demographics_initial;
                default_torus_size = GET_CONFIGURABLE( SimulationConfig )->default_torus_size;
            }

            IMigrationInfoFactory* p_mif = nullptr;
            if( use_default_demog )
            {
                release_assert( default_torus_size >= 3 ); // 3 is minimum allowabled Default_Geography_Torus_Size
                p_mif = new MigrationInfoFactoryDefault( default_torus_size );
            }
            else
            {
                p_mif = new MigrationInfoFactoryFile();
            }
            p_mif->Initialize( config, idreference );
            return p_mif;
        }

#ifndef DISABLE_VECTOR
        IMigrationInfoFactoryVector* ConstructMigrationInfoFactoryVector( JsonConfigurable* pParent, 
                                                                          const ::Configuration* config )
        {
            // -------------------------------------------------------------------------
            // --- We have to get these via the global because we are now creating this
            // --- when we are creating VectorSpeciesParameters
            // -------------------------------------------------------------------------
            bool use_default_demog = false;
            int default_torus_size = 0;

            if( GET_CONFIGURABLE( SimulationConfig ) != nullptr )
            {
                use_default_demog        = !GET_CONFIGURABLE( SimulationConfig )->demographics_initial;
                default_torus_size       = GET_CONFIGURABLE( SimulationConfig )->default_torus_size;
            }
            IMigrationInfoFactoryVector* p_mifv = nullptr ;

            if( use_default_demog )
            {
                release_assert( default_torus_size >= 3 ); // 3 is minimum allowabled Default_Geography_Torus_Size
                bool enable_vector_migration = true; // true so we create the factor with the schema parameters
                p_mifv = new MigrationInfoFactoryVectorDefault( enable_vector_migration, default_torus_size );
            }
            else
            {
                p_mifv = new MigrationInfoFactoryVector();
            }
            p_mifv->ReadConfiguration( pParent, config );
            return p_mifv;
        }
#endif
    }

}
