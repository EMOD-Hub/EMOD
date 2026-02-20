#!/usr/bin/python

import os
import json
import threading
import subprocess
import glob
import regression_local_monitor
import regression_hpc_monitor
import regression_hpc_linux_monitor
import regression_utils as ru
import sys
import shutil
import pdb
from ctypes import *


class MyRegressionRunner(object):

    # static variables
    emodules_map = {}

    # class variables
    debug = False
    campaign_filename = "campaign.json"

    def __init__(self, params):
        self.params = params
        try:
            self.dtk_hash = ru.md5_hash_of_file(self.params.executable_path)
        except Exception as ex:
            self.dtk_hash = None
            print( "Exception getting md5 of Eradication binary/exe; OK if doing pymod run." )
        self.sim_dir_sem = threading.Semaphore()
        self.emodules_map["interventions"] = []
        self.emodules_map["disease_plugins"] = []
        self.emodules_map["reporter_plugins"] = []
        self.src_dest_set = set()
        self.dll_name_to_path = {}
        if params.dll_root is not None and params.use_dlls is True:
            # print( "dll_root (remote) = " + params.dll_root )
            self.copyEModulesOver(params)
        else:
            print("Not using DLLs")

        return

    def log(self, message):
        if self.debug:
            print(message)
        return

    def update_file(self, source, destination):

        succeeded = False
        self.log("Updating '{0}' from'{1}'".format(destination, source))
        if os.path.exists(source):
            src_dest_pair = (source, destination)
            if src_dest_pair not in self.src_dest_set:
                ru.copy(source, destination)
                self.src_dest_set.add(src_dest_pair)
            succeeded = True
        else:
            self.log("Could not find source file '{0}' to copy to '{1}'".format(source, destination))
            print("Could not find source file '{0}' to copy to '{1}'".format(source, destination))

        return succeeded

    def copy_serialized_population_files(self, config_json, simulation_directory, scenario_directory):
        if config_json['parameters']["Serialized_Population_Reading_Type"] != "READ":
            return

        input_files = config_json['parameters'].get('Serialized_Population_Filenames', [])
        input_files_dir = config_json['parameters']['Serialized_Population_Path']
        input_files_dir = os.path.join(scenario_directory, input_files_dir)

        serialized_pop_filenames = []
        #config_json["parameters"]["Serialized_Population_Filenames"] = []

        for filename in input_files:
            if not filename or len(filename.strip(' ')) == 0:
                continue

            scenario_file = os.path.join(input_files_dir, filename)
            
            # Copy directly to remote simulation working directory
            if os.path.isfile(scenario_file):
                #print('Copying %s to remote working directory'%filename)
                simulation_path = os.path.join(self.params.sim_root, simulation_directory)
                simulation_file = os.path.join(simulation_path, os.path.basename(filename))
                self.update_file(scenario_file, simulation_file)
                serialized_pop_filenames.append(os.path.basename(filename))

        if( len(serialized_pop_filenames) > 0 ):
            config_json["parameters"]["Serialized_Population_Filenames"] = serialized_pop_filenames
            config_json['parameters']['Serialized_Population_Path'] = "."

        return

    def copy_demographics_files_to_user_input(self, simulation_directory, config_json, working_input_directory,
                                              scenario_directory, source_input_directory):

        input_files = config_json['parameters'].get('Demographics_Filenames', [])
        if not input_files:
            input_files = config_json['parameters'].get('Demographics_Filename', '').split(';')

        demographics_filenames = []
        config_json["parameters"]["Demographics_Filenames"] = []
        missing_files = []

        for filename in input_files:
            if not filename or len(filename.strip(' ')) == 0:
                continue

            scenario_file = None
            source_path = "."
            if working_input_directory != None:
                scenario_file = os.path.join(scenario_directory, filename)
                source_path = os.path.join(source_input_directory, filename)
                dest_path = os.path.join(working_input_directory, os.path.basename(filename))

            # For any demographics overlays WITHIN regression folder:
            # Copy directly to remote simulation working directory
            if scenario_file and os.path.isfile(scenario_file):
                # print('Copying %s to remote working directory'%filename)
                simulation_path = os.path.join(self.params.sim_root, simulation_directory)
                simulation_file = os.path.join(simulation_path, os.path.basename(filename))
                if simulation_path != simulation_file:
                    self.update_file(scenario_file, simulation_file)
            elif source_path != dest_path:
                if not self.update_file(source_path, dest_path):
                    print("Could not find source file '{0}' locally ({1}) or in inputs ({2}) [{3}]!".format(filename, scenario_file, source_path, scenario_directory))
                    # config_json["parameters"]["Demographics_Filename"] = "input file ({0}) not found".format(filename)
                    # return
                    missing_files.append(os.path.basename(filename))

            demographics_filenames.append(os.path.basename(filename))

        config_json["parameters"]["Demographics_Filenames"] = demographics_filenames

        if "Demographics_Filename" in config_json["parameters"]:
            del(config_json["parameters"]["Demographics_Filename"])

        if len(missing_files) > 0:
            config_json["parameters"][".Missing_Demographics_Files"] = missing_files

        return

    def copy_one_climate_and_migration_file(self, key, filename,
                                            simulation_directory, source_input_directory,
                                            working_input_directory, scenario):

        source = os.path.join(source_input_directory, filename)
        dest = os.path.join(working_input_directory, os.path.basename(filename))
        scenario_file = os.path.join(scenario, filename)

        # if the file is in the scenario folder
        # Copy directly to remote simulation working directory
        if os.path.isfile(scenario_file):
            simulation_path = os.path.join(self.params.sim_root, simulation_directory)
            simulation_file = os.path.join(simulation_path, os.path.basename(filename))
            source = scenario_file
            dest = simulation_file

        if (source != dest) and (not self.update_file(source, dest)):
            print("Could not find input file '{0}' to copy to '{1}' for scenario '{2}'".format(source, dest, scenario))

        if key != "Load_Balance_Filename":
            source = source + ".json"
            dest = dest + ".json"
            if (source != dest) and (not self.update_file(source, dest)):
                print("Could not find input file '{0}' to copy to '{1}' for scenario '{2}'".format(source, dest, scenario))

        
    def copy_climate_and_migration_files_to_user_input(self, simulation_directory, config_json, source_input_directory,
                                                       working_input_directory, scenario):

        filter_list = ['Demographics_Filename', 'Demographics_Filenames',
                       'Campaign_Filename',
                       'Custom_Reports_Filename',
                       'Serialized_Population_Filenames',
                       '.Serialized_Population_Filenames']

        # Copy climate and human migration files also
        for key in config_json["parameters"]:
            if ("_Filename" in key) and (key not in filter_list):
                filename = config_json["parameters"][key]
                if len(filename) == 0:
                    continue
                self.copy_one_climate_and_migration_file( key, filename, simulation_directory, source_input_directory,
                                                          working_input_directory, scenario)
                
        if "Vector_Species_Params" in config_json["parameters"].keys():
            for vsp in config_json["parameters"]["Vector_Species_Params"]:
                if "Vector_Migration_Filename" in vsp.keys():
                    filename = vsp["Vector_Migration_Filename"]
                    if len(filename) == 0:
                        continue
                self.copy_one_climate_and_migration_file( "Vector_Migration_Filename", filename,
                                                          simulation_directory, source_input_directory,
                                                          working_input_directory, scenario )

        return

    def copy_pymod_files( self, config_json, simulation_directory, scenario_path ):
        if "emodularization" not in scenario_path:
            return

        # Copy *_template.json and *_test.py from scenario_path to simulation_directory.
        # And copy ../*.pyd files
        sim_dir = os.path.join(self.params.sim_root, simulation_directory)

        # just search for all pyd files by walking the tree and copy them to each sim folder for now
        pyds = []
        suffix = ".so" if os.name == "posix" else ".pyd"
        #pyds = glob.glob( ( "../emodularization/**/*." + suffix ), recursive=True)
        for root, dirs, files in os.walk("../emodularization/"):
            for file in files:
                if file.endswith(suffix):
                    pyds.append((os.path.join(root, file))) 
        print( "Found python shared objects to copy: " + str( pyds ) )
        for pyd in pyds:
            print( "Copying " + pyd + " to" + os.path.join( sim_dir, os.path.basename( pyd ) ) )
            ru.copy( pyd, os.path.join( sim_dir, os.path.basename( pyd ) ) )
       
        # Yes, I can combine the below 3 blocks by having list pairs of root-dir and regex but I 
        # want the last two to go away.
        regexes = [ "*_template.json", "demographics_*.json", "*.json", "*.py" ]
        # copy certain files (nice if we can be more specific)
        for pattern in regexes:
            foundfiles = glob.glob(os.path.join( scenario_path, pattern ))
            for myfile in foundfiles:
                ru.copy( myfile, os.path.join( sim_dir, os.path.basename( myfile ) ) )
       

        # WANT TO GET RID OF THIS: Some multi-test situations have common python scripts in the parent folder
        # but regular tests (non-sub-foldered) could have who-knows-what in their parent dir!
        for py in glob.glob( os.path.join( os.path.join( scenario_path, ".." ), "*.py" )):
            ru.copy( py, os.path.join( sim_dir, os.path.basename( py ) ) )

        # Refresh all the shared_embedded_py_scripts: THIS IS GOING TO GO AWAY SOON WITH PIP INSTALL OF dtk_test_support
        """
        for py_file in glob.glob(os.path.join("shared_embedded_py_scripts", "dtk_*.py")):
            py_input = self.params.py_input
            self.copy_sim_file("shared_embedded_py_scripts", py_input, os.path.basename(py_file))
        """
        return

    def copy_input_files_to_user_input(self, simulation_directory, scenario_path, config_json):
        # Copy local demographics/input file(s) and remote base input file into user-local input directory 
        # E.g., //diamonds-hn/EMOD/home/jbloedow/input/Bihar, where "//diamonds-hn/EMOD/home/jbloedow/input/"
        # is gotten from config["home"] and "Bihar" is from config_json["Geography"]
        # Then use that directory as the input.
        source_input_directory = "."
        working_input_directory = self.params.user_input
        if "parameters" in config_json and "Geography" in config_json["parameters"]:
            source_input_directory = os.path.join(self.params.shared_input, config_json["parameters"]["Geography"])
            working_input_directory = os.path.join(self.params.user_input, config_json["parameters"]["Geography"])

            if (config_json["parameters"]["Geography"] != "") and not os.path.exists(working_input_directory):
                print("Creating " + working_input_directory)
                os.makedirs(working_input_directory)

        config_json["input_directory"] = working_input_directory
        # Harmonizing these to do the same thing
        self.copy_demographics_files_to_user_input(simulation_directory, config_json, working_input_directory, scenario_path, source_input_directory) 
        self.copy_climate_and_migration_files_to_user_input(simulation_directory, config_json, source_input_directory, working_input_directory, scenario_path) 
        self.copy_serialized_population_files(config_json,simulation_directory, scenario_path)
        self.copy_pymod_files(config_json, simulation_directory, scenario_path)
        self.params.use_user_input_root = True

        return

    def transform_path( self, win_path ):
        return win_path.replace( "\\", "/" ).replace( "bayesianfil01", "mnt" ).replace( "IDM", "idm" ).replace( "//", "/" )

    # Copy just build dlls to deployed places based on commandline argument
    # - The default is to use all of the DLLs found in the location the DLL projects
    #   place the DLLs (<trunk>\x64\Release).
    # - --dll-path allows the user to override this default path
    def copyEModulesOver(self, params):

        print( "src_root = " + params.src_root )

        if params.dll_path is not None:
            emodule_dir = params.dll_path
        else:
            if params.scons:
                emodule_dir = os.path.join(params.src_root, "build")
                emodule_dir = os.path.join(emodule_dir, "x64")
            else:
                emodule_dir = os.path.join(params.src_root, "x64")
            if params.debug:
                emodule_dir = os.path.join(emodule_dir, "Debug")
            elif params.quick_start:
                emodule_dir = os.path.join(emodule_dir, "QuickStart")
            else:
                emodule_dir = os.path.join(emodule_dir, "Release")

        print('Assuming emodules (dlls) are in local directory: ' + emodule_dir)

        if not os.path.exists(emodule_dir):
            print("Except that directory does not exist!  Not copying emodules.")
            return

        # print "dll_root = " + params.dll_root

        dll_dirs = ["disease_plugins",  "reporter_plugins", "interventions"]

        for dll_subdir in dll_dirs:
            suffix = "*.dll" if os.name == "nt" and self.params.linux == False else "*.so"
            #print( "Using suffix: " + str(suffix) )
            dlls = glob.glob(os.path.join( os.path.join(emodule_dir, dll_subdir), suffix )) 
            for dll in dlls:
                dll_hash = ru.md5_hash_of_file(dll)
                # print( dll_hash )
                # 1) calc md5 of dll
                # 2) check for existence of rivendell (or whatever) for <root>/emodules/<subdir>/<md5>
                # 3) if no exist, create and copy
                # 4) put full path in emodules_json
                # 5) write out emodules_json when done to target sim dir
                try:
                    target_dir = os.path.join(params.dll_root, dll_subdir)
                    target_dir = os.path.join(target_dir, dll_hash)

                    if params.sec:
                        print(dll + " will be used without checking 'new-ness'.")
                    elif not os.path.isdir(target_dir):
                        print(dll + ": copying to cluster")
                    else:
                        print(dll + ": Already on cluster")

                    if not os.path.isdir(target_dir) and not params.sec:   # sec = command-line option to skip this
                        os.makedirs(target_dir)
                        ru.copy(dll, os.path.join(target_dir, os.path.basename(dll)))

                    dll_path = os.path.join(target_dir, os.path.basename(dll))
                    #print( "dll_path = " + dll_path )
                    # dll_path has to be converted to /mnt for --linux
                    if self.params.linux:
                        dll_path = self.transform_path( dll_path )
                    self.emodules_map[dll_subdir].append(dll_path)

                    try:
                        if os.name == "nt" and self.params.linux == False:
                            dll_handle = cdll.LoadLibrary(dll)
                        else:
                            # this does lazy loading so not all dependencies have to be found
                            dll_handle = CDLL(dll,mode=1)
                        gettype = dll_handle.GetType
                        gettype.restype = c_char_p
                        name = dll_handle.GetType().decode("utf-8")
                        self.dll_name_to_path[name] = dll_path
                    except Exception as ex:
                        print("ERROR: " + str(ex) )
                        sys.exit() 

                except IOError as ioex:
                    print( "Failed to copy dll " + dll + " to " + os.path.join(os.path.join(params.dll_root, dll_dirs[1]), os.path.basename(dll)) )
                    print( "Exception " + str( ioex ) )
                    ru.final_warnings += "Failed to copy dll " + dll + " to " + os.path.join(os.path.join(params.dll_root, dll_dirs[1]), os.path.basename(dll)) + "\n"

        return

    def copy_sim_file(self, config_id, sim_dir, filename):
        if len(filename) != 0:
            filename = os.path.join(config_id, filename)
            if os.path.isfile(filename):
                # print( "Copying " + filename )
                ru.copy(filename, sim_dir, True)
            else:
                print("ERROR: Failed to find file to copy: " + filename)
        return

    def is_local_simulation(self):
        return True if os.name == "posix" else self.params.local_execution

    def filter_emodules(self, custom_reports):
        final_reporters = []

        # Extract the name of all reporters used in the custom_reports file
        reports_json = json.loads(custom_reports)

        # Return empty reporters if the file isn't formatted right or no reporters are provided
        if reports_json.get("Reports") is None:
            return final_reporters

        reporters = []
        for reporter in reports_json["Reports"]:
            name = reporter["class"]
            reporters.append(name)
        reporters_set = set(reporters)

        # Re-build the list of reporter DLL paths in the emodules map based on the reporters appearing above
        for reporter in reporters_set:
            try:
                final_reporters.append(self.dll_name_to_path[reporter])
            except KeyError:
                continue
                #print("Warning: when building reporter_plugins emodule list, no path found for '{0}'. "
                #      "Continuing.".format(reporter))
        return final_reporters

    def commissionFromConfigJson(self, sim_id, reply_json, scenario_path, report, scenario_type='tests',serialization_test_type=None):
        # scenario_type == 'tests' will compare results to reference
        # scenario_type != 'tests', e.g. 'science' or 'sweep' will skip comparison
        # now we have the config_json, find out if we're commissioning locally or on HPC

        sim_dir = os.path.join(self.params.sim_root, sim_id)
        bin_dir = os.path.join(self.params.bin_root, self.dtk_hash) if self.dtk_hash else None

        if self.is_local_simulation():
            #print("Commissioning locally (not on cluster)!")
            sim_dir = os.path.join(self.params.local_sim_root, sim_id)
            bin_dir = os.path.join(self.params.local_bin_root, self.dtk_hash) if self.dtk_hash else None
        # else:
            # print( "HPC!" )

        # create unique simulation directory
        self.sim_dir_sem.acquire()
        os.makedirs(sim_dir)
        self.sim_dir_sem.release()

        # only copy binary if new to us; copy to bin/<md5>/Eradication.exe and run from there
        # Would like to create a symlink and run from the sim dir, but can't do that on cluster; no permissions!

        # JPS - can't we just check for existence of that file?  This seems overly complicated...

        # check in bin_dir to see if our binary exists there...
        foundit = False
        bin_path = None 
        if scenario_type == "pymod":
            bin_path = "python " # py script needs to come from folder not hardcoded
            if os.name == "posix":
                # This is not the correct solution but I don't know yet how to make sure we use python3 where it's present or just python
                # Test on windows bamboo
                bin_path = "python3 "
            script_name = os.path.basename( scenario_path.strip('/') ) + "_test.py"
            bin_path += script_name
            foundit = True
        else:
            bin_path = os.path.join(bin_dir, "Eradication" if os.name == "posix" else "Eradication.exe")
        if bin_dir and os.path.exists(bin_dir):
            if os.path.exists(bin_path):
                foundit = True
        elif bin_dir:
            os.makedirs(bin_dir)
        
        if not foundit:
            print("We didn't have it, copy it up...")
            ru.copy(self.params.executable_path, bin_path)
            print("Copied!")

        reply_json["bin_path"] = bin_path
        reply_json["executable_hash"] = self.dtk_hash

        # JPS - not sure what some of the rest of this stuff does
        # campaign_json is non, and config.json contains double the stuff it needs in the sim dir... :-(
        
        # tease out campaign json, save separately
        if "campaign_json" in reply_json:
            #campaign_json = json.dumps(reply_json["campaign_json"]).replace("u'", "'").replace("'", '"').strip('"')
            with open( sim_dir + "/" + self.campaign_filename, 'w' ) as camp_file:
                    if ("campaign_json" in reply_json) and (reply_json["campaign_json"] is not None):
                        try:
                            camp_json = reply_json["campaign_json"]
                            if isinstance( camp_json, dict ):
                                json.dump(camp_json, camp_file)
                            else:
                                camp_json_grrr = json.loads( camp_json )
                                json.dump(camp_json_grrr , camp_file)
                        except Exception as ex:
                            print( "Exception writing campaign.json for " + scenario_path )
                            print( str( ex ) )
        elif "Campaign_Filename" in reply_json["parameters"]:
            camp_filename = reply_json["parameters"]["Campaign_Filename"]
            if len(camp_filename) > 0:
                try:
                    shutil.copy( os.path.join( scenario_path, camp_filename ), os.path.join( sim_dir, camp_filename ) )
                except Exception as ex:
                    print( "Exception copying campaign file to sim dir: " + str(ex) )


        # tease out custom_reports json, save separately
        if "custom_reports_json" in reply_json and reply_json["custom_reports_json"] is not None:
            reports_json = json.dumps(reply_json["custom_reports_json"]).replace("u'", "'").replace("'", '"').strip('"')
            reply_json["custom_reports_json"] = None
            # save custom_reports.json
            with open(sim_dir + "/custom_reports.json", 'w') as f:
                # f.write( json.dumps( reports_json, sort_keys=True, indent=4 ) )
                f.write(str(reports_json))

        # Use a local variable here because we don't want the PSP in the config.json that gets written out to disk
        # but we need it passed through to the monitor thread execution in the reply_json/config_json.
        py_input = None
        if "parameters" in reply_json and "Python_Script_Path" in reply_json["parameters"]:
            psp_param = reply_json["parameters"]["Python_Script_Path"]
            if psp_param == "LOCAL" or psp_param == ".":    # or . is for new usecase when using existing config.json (SFT)
                py_input = "."
                for py_file in glob.glob(os.path.join(scenario_path, "dtk_*.py")):
                    self.copy_sim_file(scenario_path, sim_dir, os.path.basename(py_file))
            elif psp_param == "SHARED":
                # We are going to copy scripts from s_e_p_s to simdir. But we need to separate dtk_test files from dtk_ep4 (TBD)
                #py_input = self.params.py_input
                py_input = "."
                if not os.path.exists(py_input):
                    os.makedirs(py_input)
                for py_file in glob.glob(os.path.join(scenario_path, "dtk_*.py")):
                    self.copy_sim_file(scenario_path, sim_dir, os.path.basename(py_file))

                # SFTS ONLY PLEASE 
                dtk_test_path = None
                #if scenario_type == "science" and os.path.exists( "shared_embedded_py_scripts/dtk_test" ):
                if os.path.exists( "shared_embedded_py_scripts/dtk_test" ):
                    dtk_test_path = os.path.join( sim_dir, "dtk_test" )
                    os.mkdir( dtk_test_path )
                    with open( os.path.join( dtk_test_path, "__init__.py" ), "w" ) as dunder:
                        dunder.write( "name='dtk_test alpha'" )

                for py_file in glob.glob(os.path.join("shared_embedded_py_scripts", "dtk_*.py")):
                    if os.path.basename( py_file ).startswith( "dtk_pre" ) or os.path.basename( py_file ).startswith( "dtk_post" ):
                        self.copy_sim_file("shared_embedded_py_scripts", sim_dir, os.path.basename(py_file))
                    elif dtk_test_path is not None:
                        self.copy_sim_file("shared_embedded_py_scripts", dtk_test_path, os.path.basename(py_file))

                # this is for dtk_test stuff
                for py_file in glob.glob(os.path.join("shared_embedded_py_scripts/dtk_test", "dtk_*.py")):
                    if dtk_test_path is not None:
                        self.copy_sim_file("shared_embedded_py_scripts/dtk_test", dtk_test_path, os.path.basename(py_file))

                # Copy any files in shared_embedded to the SCENARION SIMULATION FOLDER (THIS IS A CHANGE).
                for py_file in glob.glob(os.path.join("shared_embedded_py_scripts", "dtk_*.py")):
                    self.copy_sim_file("shared_embedded_py_scripts", sim_dir, os.path.basename(py_file))

            elif psp_param != "NO":
                print(psp_param + " is not a valid value for Python_Script_Path. Valid values are NO, LOCAL, SHARED. Exiting.")
                sys.exit() 
            #del(reply_json["parameters"]["Python_Script_Path"])

        self.copy_input_files_to_user_input(sim_id, scenario_path, reply_json)

        # print "Writing out config and campaign.json."
        # save config.json
        with open(sim_dir + "/config.json", 'w') as f:
            f.write(json.dumps(reply_json, sort_keys=True, indent=4))

        # now that config.json is written out, add Py Script Path back (if non-empty)
        if py_input is not None:
            #reply_json["PSP"] = py_input
            reply_json["PSP"] = reply_json["parameters"]["Python_Script_Path"]
            del(reply_json["parameters"]["Python_Script_Path"])

        # save campaign.json
        #with open(sim_dir + "/" + self.campaign_filename, 'w') as f:
            # f.write( json.dumps( campaign_json, sort_keys=True, indent=4 ) )
            #f.write(str(campaign_json))

        
        try:
            self.emodules_map["reporter_plugins"] = self.filter_emodules(reports_json)
        except UnboundLocalError:
            # reports_json is undefined, so use no reporters
            self.emodules_map["reporter_plugins"] = []
            
        with open(sim_dir + "/emodules_map.json", 'w') as f:
            f.write(json.dumps(self.emodules_map, sort_keys=True, indent=4))

        # ------------------------------------------------------------------
        # If you uncomment the following line, it will copy the program database
        # file to the directory where a simulation will run (i.e. with the config.json file).
        # This will help you get a stack trace with files and line numbers.
        # ------------------------------------------------------------------
        # print( "Copying PDB file...." )
        # ru.copy( "../Eradication/x64/Release/Eradication.pdb", sim_dir )
        # ------------------------------------------------------------------

        if os.path.isfile(os.path.join(scenario_path, "dtk_pre_process.py")):
            self.copy_sim_file(scenario_path, sim_dir, "dtk_pre_process.py")
        if os.path.isfile(os.path.join(scenario_path, "dtk_post_process.py")):
            self.copy_sim_file(scenario_path, sim_dir, "dtk_post_process.py")

        monitorThread = None    # need scoped here

        # print "Creating run & monitor thread."
        if self.is_local_simulation():
            monitorThread = regression_local_monitor.Monitor(sim_id, scenario_path, report, self.params, reply_json, scenario_type,serialization_test_type)
        else:
            if self.params.linux:
                monitorThread = regression_hpc_linux_monitor.LinuxHpcMonitor(sim_id, scenario_path, report, self.params, self.params.label, reply_json, scenario_type, serialization_test_type)
            else:
                monitorThread = regression_hpc_monitor.HpcMonitor(sim_id, scenario_path, report, self.params, self.params.label, reply_json, scenario_type, serialization_test_type)

        # monitorThread.daemon = True
        monitorThread.daemon = False
        # print "Starting run & monitor thread."
        monitorThread.start()

        # print "Monitor thread started, notify data service, and return."
        return monitorThread

    def attempt_test(self):
        if self.is_local_simulation():
            pass  # No test submissions for local simulations
        else:
            monitor_thread = regression_hpc_monitor.HpcMonitor("TestJob", None, None, self.params, self.params.label,
                                                               None, None, None, priority="Highest")

            # Test whether we can submit jobs to the cluster as specified
            try:
                monitor_thread.test_submission()
            except (subprocess.TimeoutExpired, AssertionError) as ex:
                print("FAILED to submit test job to the cluster. Make sure that you have valid credentials "
                      "cached with the cluster and that the cluster at the specified address is available.")
                raise ex

    def doSchemaTest(self):
        # print( "Testing schema generation..." )
        test_schema_path = "test-schema.json"
        subprocess.call([self.params.executable_path, "--get-schema", "--schema-path", test_schema_path], stdout=open(os.devnull))
        try:
            ru.load_json(test_schema_path)
            print("schema works.")
            os.remove(test_schema_path)
            return "passed"
        except Exception as ex:
            print(str(ex))

        return "failed"     # Well, it doesn't seem to have passed...
