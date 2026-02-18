import json, sys, os, shutil

def CreateHeader( start_time, num_timesteps, timestep, num_channels ):

    inset_json = {}

    inset_json["DateTime"            ] = "Unknown"
    inset_json["DTK_Version"         ] = "Unknown"
    inset_json["Report_Type"         ] = "InsetChart"
    inset_json["Report_Version"      ] = "1.0"
    inset_json["Start_Time"          ] = start_time
    inset_json["Simulation_Timestep" ] = timestep
    inset_json["Timesteps"           ] = num_timesteps
    inset_json["Channels"            ] = num_channels
    
    return inset_json

        
def application( output_path ):
    files_to_convert = []
    for fn in os.listdir( output_path ):
        if fn.startswith("ReportVectorGenetics") and fn.endswith( ".csv" ):
            files_to_convert.append(os.path.join( output_path, fn ) )

    for csv_fn in files_to_convert:
        json_fn = csv_fn.replace( "csv", "json" )

        # data for header
        start_time = 0
        num_timesteps = 0
        timestep = 1 # assume vectors so assume 1
        num_channels = 0

        json_data = {}
        json_data[ "Channels" ] = {}

        with open( csv_fn, "r" ) as rs:
            # column names should be Time, NodeID, Genome, VectorPopulation
            header = rs.readline()
            header = header.rstrip()
            column_names = header.split(",")
            if( len(column_names) != 4 ):
                print("Expected 4 columns got "+len(column_names))
                exit(-1)

            channel_names = []
            found_channels = False
            first_values = []
            while( not found_channels ):
                line = rs.readline()
                line = line.rstrip()
                values = line.split(",")
                if( values[0] != '0' ):
                    found_channels = True
                else:
                    channel_names.append( values[2] )
                first_values.append( values )

            num_channels = len(channel_names)
            for name in channel_names:
                json_data[ "Channels" ][ name ] = {}
                json_data[ "Channels" ][ name ][ "Units" ] = ""
                json_data[ "Channels" ][ name ][ "Data"  ] = []


            for values in first_values:
                name = values[2]
                json_data[ "Channels" ][ name ][ "Data" ].append( float(values[ 3 ]) )


            for line in rs:
                line = line.rstrip()
                values = line.split(",")
                name = values[2]
                json_data[ "Channels" ][ name ][ "Data" ].append( float(values[ 3 ]) )


        num_timesteps = len( json_data[ "Channels" ][ channel_names[0] ][ "Data" ] )

        inset_json = CreateHeader( start_time, num_timesteps, timestep, num_channels )

        json_data[ "Header" ] = inset_json

        with open( json_fn, "w" ) as handle:
            handle.write( json.dumps( json_data, indent=4, sort_keys=False ) )

        os.remove(csv_fn)
        print(f"Done converting {csv_fn} to {json_fn}")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("need output path")
        exit(0)

    output_path = sys.argv[1]

    application( output_path )