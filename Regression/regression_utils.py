import json
import collections
import os
import tempfile
import string
import sys
from hashlib import md5
from io import open
import argparse
import subprocess
import shutil
from pathlib import Path # hereby making this script dependent on Python3.6 :(

# below is list of 'global' variables that are shared across >1 modules in the regression suite of code. 
# probably all of them could ultimately be made at least a static member of MyRegressionRunner or Monitor.

cache_cwd = os.getcwd()
final_warnings = ""
reg_threads = []
version_string = None

def recursive_json_overrider( ref_json, flat_input_json ):
    for val in ref_json:
        is_special =  val == "Malaria_Drug_Params" \
                   or val == "TB_Drug_Params" \
                   or val == "HIV_Drug_Params" \
                   or val == "STI_Network_Params_By_Property" \
                   or val == "Parasite_Genetics"
        #if not leaf, call recursive_json_leaf_reader
        if json.dumps( ref_json[val] ).startswith( "{" ) and not is_special:
            recursive_json_overrider( ref_json[val], flat_input_json )
        elif (val == "Vector_Species_Params") or (val == "Malaria_Drug_Params"):
            # Vector_Speceis_Params/Malaria_Drug_Params is an ARRAY of Vector_Species_Param/Malaria_Drug_Params objects so we have
            # to do overriding differently.  The code below implements the following rules:
            # 1) Array exists in param_overrides but is empty = no vectors wanted
            # 2) A species is defined in the param_overrides that is not defined
            #    in the defaults = use only species in param_overrides
            # 3) No parameter exists in param_overrides = use default
            # 4) param_overrides has an element for one or more of the species in default
            #    = keep all species in default but override the parameters in the element
            
            no_vectors_wanted = (val in flat_input_json) and (len(flat_input_json[val]) == 0)

            if val not in flat_input_json:
                flat_input_json[val] = []

            species_in_override_not_ref = True
            for flat_vsp_obj in flat_input_json[val]:
                for ref_vsp_obj in ref_json[val]:
                    if ref_vsp_obj["Name"] == flat_vsp_obj["Name"]:
                        species_in_override_not_ref = False
            
            if no_vectors_wanted or (species_in_override_not_ref and (len(flat_input_json[val]) > 0)):
                #keep what is in override
                continue
            else:
                vsp_list = []
                for ref_vsp_obj in ref_json[val]:
                    for flat_vsp_obj in flat_input_json[val]:
                        if ref_vsp_obj["Name"] == flat_vsp_obj["Name"]:
                            for key in flat_vsp_obj:
                                if key != "Name":
                                    ref_vsp_obj[key] = flat_vsp_obj[key]
                    vsp_list.append( ref_vsp_obj )
                flat_input_json[val] = vsp_list

        # do MDP as special case. Sigh sigh sigh.
        elif is_special:
            # could "genericize" this if we need to... happens to work for now, since both VSP and MDP are 3-levels deep...
            if val not in flat_input_json:
                flat_input_json[val] = { }
            elif val == "STI_Network_Params_By_Property" and not("NONE" in flat_input_json[val].keys()):
                continue

            #if val == "STI_Network_Params_By_Property" and "NONE" in ref_json[val].keys() and len( flat_input_json[val].keys() ) > 0:
            #    ref_json[val].pop( "NONE" )

            for species in ref_json[val]:
                # Check the value in the dictionary to see if it is not another dictionary.
                # If not, then copy the value if not there
                if( not isinstance( ref_json[val][species], dict ) ):
                    if species not in flat_input_json[val]:
                        flat_input_json[val][species] = ref_json[val][species]
                    continue
                if species not in flat_input_json[val]:
                    if( isinstance( ref_json[val][species], dict ) ):
                        flat_input_json[val][species] = { }
                for param in ref_json[val][species]:
                    if( isinstance( ref_json[val][species], dict ) ):
                        if param not in flat_input_json[val][species]:
                            flat_input_json[val][species][param] = ref_json[val][species][param]
                    else:
                        flat_input_json[val][species] = ref_json[val][species]
        else:
            if val not in flat_input_json:
                flat_input_json[val] = ref_json[val]


def flattenConfig( configjson_path, new_config_name="config" ):
    if os.path.exists( configjson_path ) == False:
        print( "Path " + configjson_path + " supposedly doesn't exist!!!" )
        return None

    configjson_flat = {}
    configjson = load_json(configjson_path)
    recursive_json_overrider( configjson, configjson_flat )

    # get defaults from config.json and synthesize output from default and overrides
    if "Default_Config_Path" in configjson_flat:
        default_config_path = configjson_flat["Default_Config_Path"]
        stripped_path = default_config_path.strip()
        if stripped_path != default_config_path:
            print("Warning: config parameter 'Default_Config_Path' has leading or trailing whitespace in value \"{0}\"."
                  " Trimming whitespace and continuing.".format(default_config_path))
            default_config_path = stripped_path

        try:
            # This code has always worked by treating the default_configpath as relative the Regression directory.
            # No longer doing that, but preserving that capability for back-compat. Going forward, relative to the 
            # configjson_path.
            simdir = Path( configjson_path ).parent
            default_config_json = None
            if Path( os.path.join( str( simdir ), default_config_path) ).exists():
                default_config_path = os.path.join( str(simdir), default_config_path)
            else:
                default_config_path = os.path.join( '.', default_config_path)

            if os.path.exists( default_config_path ) == False:
                print( "Didn't find file at 'Default_Config_Path' = '{0}'".format( default_config_path ) )
                raise Exception( "Bad Default_Config_Path!!!" )

            default_config_json = load_json( default_config_path )
            recursive_json_overrider( default_config_json, configjson_flat ) 

        except Exception as ex:
            print( "Exception opening default config: " + str( ex ) )
            raise ex

    else:
        print( "Didn't find 'Default_Config_Path' in '{0}'".format( configjson_path ) )
        raise Exception( "Bad Default_Config_Path!!!" )

    # still need that parameter top level node
    configjson = {}
    configjson["parameters"] = configjson_flat

    # don't need backslashes in the default config path 
    # messing with anything else downstream now that it is flattened
    if "Default_Config_Path" in configjson["parameters"]:
        configjson["parameters"].pop("Default_Config_Path")

    return configjson


def md5_hash(handle):

    handle.seek(0)
    md5calc = md5()

    data = handle.read( 1048576 )
    while data:
        md5calc.update(data)
        data = handle.read( 1048576 )
    hash = md5calc.hexdigest()

    return hash


def md5_hash_of_file( filename ):

    with open(filename, 'rb') as handle:
        hash = md5_hash(handle)

    return hash


def areTheseJsonFilesTheSame( file1, file2, key = None ):
    #print( "Comparing " + file1 + " and " + file2 )
    
    def get_json_data( filename, key ):
        json_data = load_json(filename)
        if key is not None:
            #print( json_data.keys() )
            if key not in json_data.keys():
                print( "ERROR: Requested key \"" + key + "\" not in json data. Found: " + str( json_data.keys() ) )
                return None
            json_data = json_data[key]
                
        return json_data
        
    def get_data_hash( data ):
        with tempfile.TemporaryFile() as handle:
            json.dump( data, handle )
            hash = md5_hash( handle )
        
        return hash

    if os.path.exists( file1 ) == False:
        return False

    ref_data = get_json_data( file1, key )
    if ref_data == None:
        return False
    ref_hash = get_data_hash( ref_data )

    #print( "md5 of reference insetchart.json = " + str(ref_hash) )

    test_data = get_json_data( file2, key )
    test_hash = get_data_hash( test_data )
    
    #print( "md5 of test insetchart.json = " + str(test_hash) )
    
    return False if test_hash != ref_hash else True

# this implementation handles files created on either Windows or Linux
# os.path.getsize() does not work
def file_len(fname):
    i = -1 # needed incase file is empty
    with open(fname) as f:
        for i, l in enumerate(f):
            pass
    return i + 1

def make_event_map(json_in):
    event_map = {}
    for event in json_in["Events"]:
        try:
            event_map[event['Event_Name']] = (hash( json.dumps(event, sort_keys=True) ), event)
        except KeyError as e:
            print( event )
            raise Exception( 'The event printed above is missing the required %s key' % str(e) )
    return event_map

def strToEventList( str_in ):
    events_idx = str_in.find( 'Events')  # -1 is not found
    start_idx = str_in.find( '[', events_idx)  # -1 is not found

    psn = start_idx+1
    square_count = 1
    curly_count = 0
    events_str_array = []
    while square_count > 0 and psn <= len(str_in):
        cur = str_in[psn]

        #if args.verbose:
        #    print (psn, cur, square_count, curly_count, len(events_str_array))

        if cur == '[':
            square_count += 1
        elif cur == ']':
            square_count -= 1
        elif cur == '{':
            if curly_count == 0:
                begin_psn = psn
            curly_count += 1
        elif cur == '}':
            if curly_count == 1:
                events_str_array.append( str_in[begin_psn:psn+1] )
            curly_count -= 1
        psn += 1

    return events_str_array


def flattenCampaign(override_fn, verbose):
    #print "Looking for " + override_fn + " relative to " + os.getcwd()
    if os.path.exists( override_fn ) == False:
        #print( "Path \'" + override_fn + "\' supposedly doesn't exist!!! Looking relative to " + os.getcwd() )
        return None

    with open(override_fn, "r", encoding="utf-8") as override_fn_file:
        override_str = override_fn_file.read()
    override_json = json.loads( override_str )
    override_event_map = make_event_map(override_json)
    override_event_str_list = strToEventList( override_str )

    try:
        base_fn = override_json["Default_Campaign_Path"]
    except KeyError as e:
        raise Exception( 'The override campaign file %s is missing a required key: %s' % (sys.argv[1], str(e)) )

    with open(base_fn, "r", encoding="utf-8") as base_fn_file:
        base_str = base_fn_file.read()
    base_json = json.loads( base_str )
    base_event_map = make_event_map(base_json)
    base_event_str_list = strToEventList( base_str )

    merged_json = {}

    for key, value in base_json.items():
        if key == "Events":
            # Skip events
            if verbose:
                print( "Skipping key: %s" % key )
            continue
        elif key in override_json:
            if verbose:
                print( "Key from override: %s" % key )
            # Use modified value, if any
            merged_json[key] = override_json[key]
        else:
            if verbose:
                print( "Key from base: %s" % key )
            # Use base
            merged_json[key] = value

    merged_json["Default_Campaign_Path"] = override_json["Default_Campaign_Path"]
    merged_str = json.dumps(merged_json, indent=4, sort_keys=True )
    merged_events_str = ""

    override_event_name_to_string_map = {}
    for override_event_str in override_event_str_list:
        override_event_json = json.loads(override_event_str)
        event_name = override_event_json["Event_Name"]
        override_event_name_to_string_map[event_name] = override_event_str

    for override_event_str in override_event_str_list:
        override_event_json = json.loads(override_event_str)
        event_name = override_event_json["Event_Name"]

        if event_name not in base_event_map:
            if verbose:
                print( "USE OVERLAY (EXCLUSIVE): %s" % event_name )
            merged_events_str += "        %s,\n"%override_event_str


    for base_event_str in base_event_str_list:
        base_event_json = json.loads(base_event_str)
        event_name = base_event_json["Event_Name"]

        base_hash = base_event_map[event_name][0]

        if event_name in override_event_map:
            if verbose :
                print( "USE OVERLAY: %s" % event_name )
            override_event_str = override_event_name_to_string_map[event_name]
            merged_events_str += "        %s,\n"%override_event_str
        else:
            if verbose :
                print( "USE BASE: %s" % event_name )
            merged_events_str += "        %s,\n"%base_event_str

    merged_events_str = merged_events_str[:-2]  # Chop final ",\n"


    insert_psn = merged_str.rfind('}')
    insert_psn = merged_str.rfind('\n', insert_psn)
    merged_str = merged_str[0:insert_psn-1] \
               + ',\n' \
               + '    "Events":\n' \
               + '    [\n' \
               + merged_events_str + "\n" \
               + '    ]\n' \
               + '}'


    saveto_fn = override_fn.replace( "campaign_overrides", "campaign" )
    with open( saveto_fn, 'w' ) as handle:
        if verbose:
            print( "Writing flattened campaign to %s" % saveto_fn )
        handle.write( merged_str )


    return json.loads(merged_str)


def load_json(filepath, post_process=None, ignore_notfound=True):
    """Load json from a file, with optional post processing of contents prior to parsing"""
    if os.path.exists(filepath):
        try:
            with open(filepath, 'r', encoding='utf-8') as json_file:
                if post_process:
                    return json.loads(post_process(json_file.read()))
                else:
                    return json.loads(json_file.read())
        except ValueError:
            print("JSON decode error from file {} ".format(filepath))
            raise
        except IOError:
            print("Error accessing json file {} ".format(filepath))
            raise
    else:
        if not ignore_notfound:
            # should this raise an error?
            print("JSON file not found: {}".format(filepath))
    return None


def touch_file(filename):
    """Update a file's last modification date by opening/closing it"""
    with open(filename, "a"):
        os.utime(filename, None)


def files_are_identical(source, target):
    """
    Determine whether a speculative file copy is necessary by comparing the md5 hash of file contents.

    :param source: source file path
    :param target: destination directory or file path
    :return: true if source and destination files exist and have identical md5 hashes
    """
    # if the target doesn't exist, it can't be identical
    if os.path.exists(target):
        # if the source doesn't exist then we have bigger problems, if it isn't a file, skip this check
        if os.path.isfile(source):
            # if the target is a directory, find out what the target file will be and check that
            target_file = target
            if os.path.isdir(target):
                target_file = os.path.join(target, os.path.basename(source))
                if not os.path.exists(target_file):
                    return False
            # only check hash equality if file sizes are identical
            if os.path.getsize(source) == os.path.getsize(target_file):
                source_hash = md5_hash_of_file(source)
                target_hash = md5_hash_of_file(target_file)
                return source_hash == target_hash
    return False


def copy(source, destination, skip_unchanged=False):
    fnull = open(os.devnull, 'w')
    source = os.path.abspath(source)
    destination = os.path.abspath(destination)
    result = 1

    if skip_unchanged and files_are_identical(source, destination):
        return 0

    # If 'copy' exists, use that
    try:
        result = subprocess.call(['copy', source, destination], shell=True, stdout=fnull, stderr=subprocess.STDOUT)
    except OSError:
        pass
    if result == 0:
        return result

    # If 'cp' exists, use that
    try:
        return subprocess.call(['cp', source, destination], shell=False, stdout=fnull, stderr=subprocess.STDOUT)
    except OSError:
        pass
    if result == 0:
        return result

    # Otherwise, fall back to shutil.copy, but emit a warning
    print("Warning: 'copy' and 'cp' not found on the system, falling back to shutil.copy (may be "
              "substantially slower)")
    return shutil.copy(source, destination)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(help='Action to take', dest='action')
    flatten_parser = subparsers.add_parser('flatten-config', help="Flatten param_overrides with a config file to DTK expected config format")
    flatten_parser.add_argument('path', help="Path to the param_overrides.json file")
    args = parser.parse_args()

    if args.action == "flatten-config":
        flattenConfig(args.path, "config_flattened")
    else:
        raise Exception("Invalid utility selected")

