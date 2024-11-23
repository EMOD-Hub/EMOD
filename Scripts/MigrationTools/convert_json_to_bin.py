# convert_json_to_bin.py
# -----------------------------------------------------------------------------
# This script converts a JSON formated txt file to an EMOD binary-formatted migration file.
# It also creates the required metadata file.
#
# The JSON file allows the user to specify different rates for different ages
# and genders.
# 
# The output binary file has one or two Gender Data sections depending on whether
# the JSON file has different data for each gender.  Each Gender Data section has
# one Age Data section for each age specified in the JSON file.  Each Age Data
# section has one Node Data section for each node that individuals can migrate
# from.  Each Node Data section has one chunk of data
#    [1-unint32_t (4-bytes) plus 1-double (8-bytes)]
# for each destination where each Node Data section has DestinationsPerNode chuncks.
# In other words, each Node Data section is 12-bytes times DestinationsPerNode
# -----------------------------------------------------------------------------

import sys, os, json, collections, struct, datetime

# -----------------------------------------------------------------------------
# Age Limits
# -----------------------------------------------------------------------------
AGE_Min = 0.0
AGE_Max = 125.0

# -----------------------------------------------------------------------------
# CheckAge
# -----------------------------------------------------------------------------
def CheckAge( age ):
    if( age < AGE_Min ):
        print(f"Invalid age={age} < {AGE_Min}")
        exit(-1)

    if( age > AGE_Max ):
        print(f"Invalid age={age} > {AGE_Max}")
        exit(-1)

# -----------------------------------------------------------------------------
# CheckAgeArray
# -----------------------------------------------------------------------------
def CheckAgeArray( ages_years ):
    errmsg = JSON_AgesYears + " must be an array of ages in years and in increasing order."
    if( len( ages_years ) == 0 ):
        print(errmsg)
        exit(-1)

    prev = 0.0
    for age in ages_years:
        CheckAge( age )
        if( age < prev ):
            print(errmsg)
            exit(-1)
        prev = age

# -----------------------------------------------------------------------------
# GenderDataTypes
# -----------------------------------------------------------------------------
GDT_SAME_FOR_BOTH_GENDERS = "SAME_FOR_BOTH_GENDERS"
GDT_ONE_FOR_EACH_GENDER   = "ONE_FOR_EACH_GENDER"

GenderDataTypes = []
GenderDataTypes.append( GDT_SAME_FOR_BOTH_GENDERS )
GenderDataTypes.append( GDT_ONE_FOR_EACH_GENDER   )

# -----------------------------------------------------------------------------
# CheckGenderDataType
# -----------------------------------------------------------------------------
def CheckGenderDataType( gdt ):
    found = False
    for type in GenderDataTypes:
        found |= (type == gdt)

    if( not found ):
        print(f"Invalid GenderDataType = {gdt}")
        exit(-1)

# -----------------------------------------------------------------------------
# InterpolationTypes
# -----------------------------------------------------------------------------
InterpolationTypes = []
InterpolationTypes.append( "LINEAR_INTERPOLATION" )
InterpolationTypes.append( "PIECEWISE_CONSTANT"   )

# -----------------------------------------------------------------------------
# CheckInterpolationType
# -----------------------------------------------------------------------------
def CheckInterpolationType( interp_type ):
    found = False
    for type in InterpolationTypes:
        found |= (type == interp_type)

    if( not found ):
        print(f"Invalid InterpolationType = {interp_type}")
        exit(-1)

# -----------------------------------------------------------------------------
# MigrationTypes
# -----------------------------------------------------------------------------
MigrationTypes = []
MigrationTypes.append( "LOCAL_MIGRATION"    )
MigrationTypes.append( "AIR_MIGRATION"      )
MigrationTypes.append( "REGIONAL_MIGRATION" )
MigrationTypes.append( "SEA_MIGRATION"      )

# -----------------------------------------------------------------------------
# CheckMigrationType
# -----------------------------------------------------------------------------
def CheckMigrationType( mig_type ):
    found = False
    for type in MigrationTypes:
        found |= (type == mig_type)

    if( not found ):
        print(f"Invalid MigrationType = {mig_type}")
        exit(-1)

# -----------------------------------------------------------------------------
# JSON Element Names
# -----------------------------------------------------------------------------
# NOTE: The indention below indicates where the tag is used in the JSON

JSON_IdRef          = "IdReference"
JSON_InterpType     = "Interpolation_Type"
JSON_GenderDataType = "Gender_Data_Type"
JSON_AgesYears      = "Ages_Years"
JSON_NodeData       = "Node_Data"
JSON_ND_FromNodeId      = "From_Node_ID"
JSON_ND_RateData        = "Rate_Data"
JSON_RD_ToNodeId            = "To_Node_ID"
JSON_RD_RatesBoth           = "Avg_Num_Trips_Per_Day_Both"
JSON_RD_RatesMale           = "Avg_Num_Trips_Per_Day_Male"
JSON_RD_RatesFemale         = "Avg_Num_Trips_Per_Day_Female"

# -----------------------------------------------------------------------------
# CheckInJson
# -----------------------------------------------------------------------------
def CheckInJson( fn, data, key ):
    if( not key in data ):
        print(f"Could not find {key} in file {fn}.")
        exit(-1)

# -----------------------------------------------------------------------------
# CheckRatesSize
# -----------------------------------------------------------------------------
def CheckRatesSize( num_ages, rd_data, key ):
    if( len( rd_data[ key ] ) != num_ages ):
        print (f"{JSON_AgesYears} has {num_ages} values and one of the {key} has {len( rd_data[ key ] )} values.  They must have the same number.")
        exit(-1)

# -----------------------------------------------------------------------------
# ReadJson
# -----------------------------------------------------------------------------
def ReadJson( json_fn ):
    json_file = open( json_fn,'r')
    json_data = json.load( json_file )
    json_file.close()

    CheckInJson( json_fn, json_data, JSON_IdRef           )
    CheckInJson( json_fn, json_data, JSON_InterpType      )
    CheckInJson( json_fn, json_data, JSON_GenderDataType  )
    CheckInJson( json_fn, json_data, JSON_AgesYears       )
    CheckInJson( json_fn, json_data, JSON_NodeData        )

    CheckInterpolationType( json_data[ JSON_InterpType     ] )
    CheckGenderDataType(    json_data[ JSON_GenderDataType ] )
    CheckAgeArray( json_data[ JSON_AgesYears ] )

    if( len( json_data[ JSON_NodeData ] ) == 0 ):
        print(f"{JSON_NodeData} has no elements so there would be no migration data.")
        exit(-1)

    num_ages = len( json_data[ JSON_AgesYears ] )

    for nd_data in json_data[ JSON_NodeData ]:
        CheckInJson( json_fn, nd_data, JSON_ND_FromNodeId )
        CheckInJson( json_fn, nd_data, JSON_ND_RateData    )

        if( len( nd_data[ JSON_ND_RateData ] ) == 0 ):
            print(f"{JSON_ND_RateData} has no elements so there would be no migration data.")
            exit(-1)

        for rd_data in nd_data[ JSON_ND_RateData ]:
            CheckInJson( json_fn, rd_data, JSON_RD_ToNodeId )

            if( json_data[ JSON_GenderDataType ] == GDT_ONE_FOR_EACH_GENDER ):
                CheckInJson( json_fn, rd_data, JSON_RD_RatesMale )
                CheckInJson( json_fn, rd_data, JSON_RD_RatesFemale )

                CheckRatesSize( num_ages, rd_data, JSON_RD_RatesMale   )
                CheckRatesSize( num_ages, rd_data, JSON_RD_RatesFemale )
            else:
                CheckInJson( json_fn, rd_data, JSON_RD_RatesBoth )

                CheckRatesSize( num_ages, rd_data, JSON_RD_RatesBoth )

    return json_data

# -----------------------------------------------------------------------------
# SummaryData
# -----------------------------------------------------------------------------
class SummaryData:
    def __init__( self, nodeCount, offsetStr, maxDestinations ):
        self.num_nodes = nodeCount
        self.offset_str = offsetStr
        self.max_destinations_per_node = maxDestinations

# -----------------------------------------------------------------------------
# GetSummaryData
# -----------------------------------------------------------------------------
def GetSummaryData( json_data ):
    from_node_id_list = []

    # -------------------------------------------------------------------------
    # Find the list node that individuals can migrate from
    # Also find the maximum number of nodes that one can go to from a give node.
    # This max is used in determine the layout of the binary data.
    # -------------------------------------------------------------------------
    max_destinations = 0
    for node_data in json_data[ JSON_NodeData ]:
        from_node_id_list.append( int( node_data[ JSON_ND_FromNodeId ] ) )
        destinations = len( node_data[ JSON_ND_RateData ] )
        if( destinations > max_destinations ):
            max_destinations = destinations

    print(f"max_destinations = {max_destinations}")

    # -------------------------------------------------------------------
    # Create NodeOffsets string
    # This contains the location of each From Node's data in the bin file
    # -------------------------------------------------------------------
    offset_str = ""
    nodecount = 0

    for from_node_id in from_node_id_list:
        offset_str += '%0.8X' % from_node_id
        offset_str += '%0.8X' % (nodecount * max_destinations * 12) # 12 -> sizeof(uint32_t) + sizeof(double) 
        nodecount += 1

    summary = SummaryData( nodecount, offset_str, max_destinations )

    return summary

# -----------------------------------------------------------------------------
# WriteBinFile
# -----------------------------------------------------------------------------
def WriteBinFile( bin_fn, json_data, summary ):
    bin_file = open( bin_fn, 'wb' )
    
    if( json_data[ JSON_GenderDataType ] == GDT_ONE_FOR_EACH_GENDER ):
        WriteBinFileGender( bin_file, json_data, summary, JSON_RD_RatesMale )
        WriteBinFileGender( bin_file, json_data, summary, JSON_RD_RatesFemale )
    else:
        WriteBinFileGender( bin_file, json_data, summary, JSON_RD_RatesBoth )

    bin_file.close()

# -----------------------------------------------------------------------------
# WriteBinFileGender
# -----------------------------------------------------------------------------
def WriteBinFileGender( bin_file, json_data, summary, rates_key ):
    for age_index in range( len( json_data[ JSON_AgesYears ] ) ):
        for node_data in json_data[ JSON_NodeData ]:
            array_id = []
            array_rt = []
                
            # Initialize with zeros
            for i in range( summary.max_destinations_per_node ):
                array_id.append(0)
                array_rt.append(0)

            # Populate arrays with data
            index = 0
            for rate_data in node_data[ JSON_ND_RateData ] :
                array_id[ index ] = int( rate_data[ JSON_RD_ToNodeId ] )
                array_rt[ index ] = rate_data[ rates_key ][ age_index ]
                index += 1

            # Format data into binary
            bin_data_id = struct.pack( 'I'*len(array_id), *array_id )
            bin_data_rt = struct.pack( 'd'*len(array_rt), *array_rt )

            bin_file.write( bin_data_id )
            bin_file.write( bin_data_rt )

# -----------------------------------------------------------------------------
# WriteMetadataFile
# -----------------------------------------------------------------------------
def WriteMetadataFile( metadata_fn, mig_type, json_data, rate_data ):
    output_json = collections.OrderedDict([])

    output_json["Metadata"] = {}
    output_json["Metadata"]["IdReference"        ] = json_data[ JSON_IdRef ]
    output_json["Metadata"]["DateCreated"        ] = datetime.datetime.now().ctime()
    output_json["Metadata"]["Tool"               ] = os.path.basename(sys.argv[0])
    output_json["Metadata"]["DatavalueCount"     ] = rate_data.max_destinations_per_node
    output_json["Metadata"]["MigrationType"      ] = mig_type
    output_json["Metadata"]["GenderDataType"     ] = json_data[ JSON_GenderDataType ]
    output_json["Metadata"]["InterpolationType"  ] = json_data[ JSON_InterpType ]
    output_json["Metadata"]["AgesYears"          ] = json_data[ JSON_AgesYears ]
    output_json["Metadata"]["NodeCount"          ] = rate_data.num_nodes
    output_json["NodeOffsets"] = rate_data.offset_str

    with open( metadata_fn, 'w') as file:
        json.dump( output_json, file, indent=4 )

# -----------------------------------------------------------------------------
# Main
# -----------------------------------------------------------------------------
if __name__ == "__main__":
    if len(sys.argv) != 4:
        print ("\nUsage: %s [input-json] [output-bin] [migration-type]" % os.path.basename(sys.argv[0]))
        exit(0)

    json_fn  = sys.argv[1]
    bin_fn   = sys.argv[2]
    mig_type = sys.argv[3]

    metadata_fn = bin_fn + ".json"

    CheckMigrationType( mig_type )

    json_data = ReadJson( json_fn )

    summary = GetSummaryData( json_data )

    WriteBinFile( bin_fn, json_data, summary )
    WriteMetadataFile( metadata_fn, mig_type, json_data, summary )

    print(f"Finished converting {json_fn} to {bin_fn} and {metadata_fn}")

