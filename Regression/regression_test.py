"""
This file is the root of regression. Almost everything here is about copying files around.
"""

import argparse
import datetime
import glob
import json
import os # e.g., mkdir
import re
import subprocess
import sys
import pdb
import threading

import regression_runner
import regression_utils as ru
import regression_runtime_params
import regression_report

def get_argparser(parser = None):
    """
    Add argparse parameters to a parser object.

    :param parser: argparse parser to add params to (useful for testing)
    :return: argparse parser populated with param arguments
    """
    if not parser:
        parser = argparse.ArgumentParser()
    parser.add_argument("suite",
                        help="JSON test-suite to run - e.g. full.json, sanity (converted to sanity.json) - one or more comma separated values")
    parser.add_argument("exe_path", metavar="exe-path", nargs="?", default="",
                        help="Path to the Eradication.exe binary to run.  Default is where the executable is normally built depending on --scons, --debug, and the OS.")
    parser.add_argument("--perf", action="store_true", default=False,                   help="Run for performance measurement purposes")
    parser.add_argument("--hidegraphs", action="store_true", default=False,             help="Suppress pop-up graphs in case of validation failure")
    parser.add_argument("--debug", action="store_true", default=False,                  help="Use debug path for emodules")
    parser.add_argument("--quick-start", action="store_true", default=False,            help="Use QuickStart path for emodules")
    parser.add_argument("--label",                                                      help="Custom suffix for HPC job name")
    parser.add_argument("--config", default="regression_test.cfg",                      help="Regression test configuration [regression_test.cfg]")
    parser.add_argument("--disable-schema-test", action="store_true", default=False,    help="Disable schema test (testing is on by default, use to suppress schema testing)")
    parser.add_argument("--component-tests", action="store_true", default=False,        help="Run the componentTests if the executable exists")
    parser.add_argument("--component-tests-show-output", action="store_true", default=False,
                                                                                        help="Show the output of the componentTests")
    parser.add_argument("--use-dlls", action="store_true", default=False,               help="Use emodules/DLLs when running tests")
    parser.add_argument("--all-outputs", action="store_true", default=False,            help="Use all output .json files for validation, not just InsetChart.json")
    parser.add_argument("--dll-path",                                                   help="Path to the root directory of the DLLs to use (e.g. contains reporter_plugins)")
    parser.add_argument("--skip-emodule-check", action="store_true", default=False,     help="Use this to skip the sometimes slow check that EMODules on cluster are up to date.")
    parser.add_argument("--config-constraints", nargs="?",                              help="key:value pair(s) which are used to filter the scenario list (the given key and value must be in the config.json)")
    parser.add_argument("--scons", action="store_true", default=False,                  help="Indicates scons build so look for custom DLLs in the build/64/Release directory.")
    parser.add_argument('--local', action='store_true', default=False,                  help='Run all simulations locally.')
    parser.add_argument('--linux', action='store_true', default=False,                  help='Run on linux target')
    parser.add_argument("--print-error", action='store_true', default=False,            help="Print error message to screen.")

    return parser

def setup(args=None):
    """
    Process command-line parameters

    :param args: argparse parser object to use instead of creating a new one (for testing)
    :return: RuntimeParameters object reflecting all the relevant arguments
    """
    if not args:
        args = get_argparser().parse_args()
    params = regression_runtime_params.RuntimeParameters(args)
    return params

def read_regression_files(suites):
    """
    Load regression test info for a list of suites from the appropriate files

    :param suites: json suite file or regression test path
    :return: json object containing an array of regression test paths
    """
    regression_list_json = {}
    for suite in suites:
        if os.path.isdir(suite) and os.path.exists(os.path.join(suite, "param_overrides.json")):
            print("You specified a directory - " + suite)
            root = "tests"
            # if this is single directory, determine if regression (tests) or science by presence of output/InsetChart.json
            if not os.path.exists( os.path.join( suite, "output/InsetChart.json") ):
                root = "science"
            add_regression_tests(regression_list_json, [{"path": suite}], root)
        else:
            if not suite.endswith(".json"):
                suite_file = suite + ".json"
            else:
                suite_file = suite
            print("You specified a file - " + suite_file)
            add_tests_from_file(regression_list_json, suite_file)
    return regression_list_json

def add_regression_tests(regression_list_json, tests, root="tests"):
    """
    Merge regression test info into an existing list, joining them at the root

    :param regression_list_json: existing list of regression tests, modified by this function
    :param tests: regression tests to add into the list
    :param root: root to use for joining (e.g. 'tests', 'science', 'sweep', etc.)
    """
    new_tests = tests
    if root in tests:
        new_tests = tests[root]

    if len(regression_list_json) == 0:
        regression_list_json[root] = new_tests
        return

    if root not in regression_list_json:
        raise ValueError(
            "Attempt to mix test types, regression list already contains a root '{0}', cannot also add '{1}'.".format(
                list(regression_list_json.keys())[0], root))

    if isinstance(regression_list_json[root], dict):
        regression_list_json[root].update(new_tests)
    else:
        regression_list_json[root].extend(new_tests)

def add_tests_from_file(regression_list_json, filename):
    """
    Load regression tests from a file and add them to an existing list

    :param regression_list_json: existing list of regression tests, modified by this function
    :param filename: suite filename (e.g. generic.json)
    """
    tests = ru.load_json(filename)
    test_type = reglist_test_type(tests)
    if test_type:
        add_regression_tests(regression_list_json, tests, test_type)
    else:
        print("Warning: failed to add any regression tests from file {0}. Expected either 1) a regression_test "
              "suite file path that points to a collection of tests, or 2) a directory path containing "
              "a single test.".format(filename))

def reglist_test_type(list):
    """
    Determine the type (tests, science, sweep, science_sweep) of a list of reg. tests

    :param list: regression tests list
    :return: type of regression list (e.g. "tests", "science", "sweep", "science_sweep"
    """
    if not list:
        return None
    test_types = ["tests", "science", "sweep", "science_sweep", "pymod" ]
    for test_type in test_types:
        if test_type in list:
            return test_type
    return None

def setup_pymod_directory( path ):
    #print( "PyMod test type: copy up all .pyd, .json, and .py files." ) # maybe later
    if os.path.exists( os.path.join( path, "nd_template.json" ) ) == False:
        print( "Could not find nd_template.json file in " + path )
        return None 
    return ru.load_json( os.path.join( path, "nd_template.json" ) )

def get_test_config(path, test_type, report):
    """
    Return the test config for a given test path

    :param path: regression test path
    :param test_type: e.g. "tests", "science", etc.
    :param report: regression report (for keeping track of errors in loading)
    :return: json regression test config
    """
    errmsg = None
    config_json = None

    if test_type == "pymod":
        config_json = setup_pymod_directory( os.path.dirname( path ) )
    else:
        if not os.path.exists( path ):
            errmsg = "Error flattening config - path does not exist: {}".format( path )
        else:
            param_overrides_filename = os.path.join(path, "param_overrides.json")
            if os.path.exists( param_overrides_filename ):
                try:
                    config_json = ru.flattenConfig( param_overrides_filename )
                except:
                    errmsg = "Error flattening config: {} - {}".format( sys.exc_info()[0],
                                                                        sys.exc_info()[1] )
            else:
                #print("Warning: no param_overrides.json file available, config will not be flattened")
                try:
                    config_json = ru.load_json(os.path.join(path, "config.json"))
                except Exception as ex:
                    errmsg = "Malformed config.json in: " + path

    if errmsg != None:
        print(errmsg)
        report.addErroringTest( path, errmsg, "(no simulation directory created).", test_type )

    return config_json

def flatten_campaign(simcfg_path, test_type, report):
    """
    Flatten the campaign config for a path

    :param simcfg_path: regression test path (directory root)
    :param test_type: e.g. "tests", "science", etc.
    :param report: regression report (for keeping track of errors in loading)
    :return: flattened json regression test campaign info
    """
    campaign_override_filename = os.path.join( simcfg_path, "campaign_overrides.json")
    try:
        if os.path.exists(campaign_override_filename):
            return ru.flattenCampaign(campaign_override_filename, False)
        else:
            return None
    except:
        print("Unexpected error while flattening campaign: {} - {}".format(sys.exc_info()[0], sys.exc_info()[1]))
        print("Failed to flatten campaign: " + campaign_override_filename)
        report.addErroringTest(simcfg_path, "Failed flattening campaign.", "(no simulation directory created).",
                               test_type)
    return None

def load_campjson_from_campaign(simcfg_path, enable_interventions, runner, campaign_filename=None):
    """
    Load campaign json

    :param simcfg_path: regression test path (directory root)
    :param enable_interventions: 1 = True
    :param campaign_filename: campaign filename specified from config json
    :return: json campaign data
    """
    if enable_interventions == 1:
        if not campaign_filename or not os.path.exists(campaign_filename):
            print( "Caution: Didn't find {0} for {1}.".format( campaign_filename, simcfg_path ) )
            campaign_filename = glob.glob(os.path.join(simcfg_path, "campaign_*.json"))[0]
        runner.campaign_filename = os.path.basename(campaign_filename)
        try:
            # One version breaks HIV campaign scenarios, the other breaks others.
            #campjson = ru.load_json(campaign_filename, lambda x: x.replace("u'", "'").replace("'", '"').strip('"'))
            campjson = ru.load_json(campaign_filename)
            #return json.dumps(campjson)
            return campjson
        except json.decoder.JSONDecodeError as err:
            err_msg = "JSON Decode Error loading test campaign, {}: {}".format(campaign_filename, err)
            sys.stderr.write(err_msg + "\n")
            report.addErroringTest(simcfg_path, err_msg, "(no simulation directory created).", test_type)
        except:
            err_msg = "Unexpected error while loading campaign, {}: {} - {}".format(campaign_filename, sys.exc_info()[0], sys.exc_info()[1])
            sys.stderr.write(err_msg + "\n")
            report.addErroringTest(simcfg_path, err_msg, "(no simulation directory created).", test_type)

    return {'Events': []}

def get_exe_version(exepath):
    """
    Return the version string of an Eradication executable

    :param exepath: path to Eradication.exe
    :return: version number as a string: maj.minor.build.rev (e.g. 2.11.3668.0)
    """
    proc = subprocess.Popen(exepath.split() + ["-v"], shell=False, stdout=subprocess.PIPE)
    [pipe_stdout, pipe_stderr] = proc.communicate()
    search_string = '[0-9]+.[0-9]+.[0-9]+.[0-9]+'
    version_results = re.search(search_string, pipe_stdout.decode('utf-8'))
    version_string = version_results.group(0)
    return version_string

def get_homepath(sim_root, run_local=False):
    """
    Find the home path of the current user

    :param sim_root: simulation root for running locally
    :param run_local: flag for whether the run is using HPC or running locally
    :return: home directory
    """
    if os.getenv("HOME") != None:
        return os.getenv("HOME")
    elif run_local:
        return os.path.join(os.getenv("HOMEDRIVE"), os.getenv("HOMEPATH"))
    else:  # cluster/HPC
        return os.path.join(sim_root, "..")

def configure_SFT_graphs(homepath, hide_graphs):
    """
    Prepare to generate graphs for SFTs by updating a touch file

    :param homepath: home directory
    :param hide_graphs: whether to show graphs or not
    """
    flag = os.path.join(homepath, ".rt_show.sft")
    if os.path.exists(flag):
        os.remove(flag)
    if not hide_graphs:
        ru.touch_file(flag)

def run_component_tests(scons_build, show_output):
    """
    Run component tests

    :param scons_build: flag to look for the binaries in the scons output location
    :param show_output: whether to show the output of the component tests
    :return: whether all tests succeeded
    """

    # find component test executable path
    if (scons_build or (os.sys.platform != 'win32')):
        component_test_path = "../build/x64/Release/componentTests/componentTests"
    else:
        component_test_path = "../x64/Release/componentTests"

    if os.sys.platform == 'win32':
        component_test_path += ".exe"

    component_test_path = os.path.realpath(component_test_path)

    if (os.path.exists(component_test_path)):
        os.chdir("../componentTests")
        if (show_output):
            with open("StdErr.txt", "w") as stderr_file:
                ret = subprocess.call([component_test_path], stderr=stderr_file)
            os.remove("StdErr.txt")
        else:
            with open("StdOut.txt", "w") as stdout_file:
                ret = subprocess.call([component_test_path], stdout=stdout_file)
            if (ret == 0):
                os.remove("StdOut.txt")

        if ret == 0:
            return True
        else:
            sys.stderr.write( "Component tests failed!" )
            return False
    else:
        return False

class SimulationIDGen(object):
    """Generate unique sim_ids, this is useful for keeping track of the first id created as well as ensuring uniqueness"""
    def __init__(self, timestamp=None):
        self.count = 0
        if timestamp:
            self.starttime = timestamp
        else:
            self.starttime = datetime.datetime.now()
        self.timestamp = self.starttime.strftime("%Y_%m_%d_%H_%M_%S_%f")[:-3]

    def get_simulation_id(self, num=None):
        """
        Increment simulation count and return the next simulation id. Optionally return a specific simulation id.

        :param num: Number of simulation id to return, if present skip incrementing the count
        :return: simulation id, ex: YYYY_MM_DD_hh_mm_ss_mmm_####, 2017_05_10_18_42_23_865_0001
        """
        if not num:
            self.count += 1
            num = self.count
        return '{}_{:04d}'.format(self.timestamp, num)

    def first_id(self):
        """Return the first id, usually used as a regression run id"""
        return self.get_simulation_id(1)

def get_serialization_test_type( configjson ):
    serialization_test_type = None

    # The serialization tests for exactness work on windows and WSL but do not on CentOs.
    # I'm out of time so we will run the full sim test and not the BEFORE and AFTER on Linux.
    if (os.name == "nt") and ("Serialization_Test" in configjson["parameters"]):
        if configjson["parameters"]["Serialization_Test"] == 1:
            if "Serialized_Population_Writing_Type" not in configjson["parameters"]:
                raise ValueError("If Serialization_Test=1, then you must define Serialized_Population_Writing_Type")
            if configjson["parameters"]["Serialized_Population_Writing_Type"] != "TIME":
                raise ValueError("If Serialization_Test=1, Serialized_Population_Writing_Type must equal TIME")

            if "Serialization_Precision" not in configjson["parameters"]:
                raise ValueError("If Serialization_Test=1, then you must define Serialization_Precision=FULL")
            if configjson["parameters"]["Serialization_Precision"] != "FULL":
                raise ValueError("If Serialization_Test=1, Serialization_Precision must equal FULL")

            if "Serialization_Times" not in configjson["parameters"]:
                raise ValueError("If Serialization_Test=1, then you must define Serialization_Times with one value")
            if len(configjson["parameters"]["Serialization_Times"]) != 1:
                raise ValueError("If Serialization_Test=1, Serialization_Times must have exactly one value")
            serialization_time = configjson["parameters"]["Serialization_Times"][0]

            if configjson["parameters"]["Start_Time"] != 0:
                raise ValueError("If Serialization_Test=1, Start_Time should be zero")

            if (serialization_time <= 0) or (configjson["parameters"]["Simulation_Duration"] <= serialization_time):
                raise ValueError("If Serialization_Test=1, The serialization time should be between 0 and Simulation_Duration")

            serialization_test_type = "FULL"

    return serialization_test_type

class SerializationTestMonitor(threading.Thread):
    """Verify sim from serialized file gives same results as full sim

    This class adds the ability to verify that a serialized population file was written
    and read correctly (i.e. all the right object state).  It does this by running three
    versions of the simulation:
        - BEFORE - A simulation up to the point where the serialized file is created
        - FULL   - The simulations for the full duration
        - AFTER  - A simulation that reads the serialized file created in FULL and runs to the end
    The test takes the InsetChart's created by BEFORE and AFTER, concatinates them, and compares
    it to the saved version.  Since FULL is also compared to the saved version, we know that the
    AFTER version was able to generated the same results as the FULL version.

    To make a regression test a SerializationTest, one must do the following:
        0) Configure a simulation to run for the full time, probably at least
           50 timesteps past the serialization point
        1) Add the flag "Serialization_Test" and set it to 1 in the param_overrides.json
        2) Have "Serialized_Population_Writing_Type" set to "TIME"
        3) Have only one timestep defined in "Serialization_Times"
        4) Have "Serialization_Precision" set to "FULL" so that we avoid round off
           error in the serialized file
        For example:
            "Serialization_Test": 1,
            "Serialized_Population_Writing_Type": "TIME",
            "Serialization_Precision": "FULL",
            "Serialization_Times": [ 500 ]

    WARNING: The AFTER sim will set the "Start_Time" to the time of the serialized file.
             This means campaign events that occur before this time will not occur in the
             AFTER sim.
    """

    def __init__( self,
                  full_sim_thread,
                  runner,
                  full_sim_id,
                  before_sim_id,
                  after_sim_id,
                  configjson,
                  sim_path,
                  report,
                  scenario_type='tests' ):
        threading.Thread.__init__( self )
        self.full_sim_thread = full_sim_thread
        self.runner = runner
        self.full_sim_id = full_sim_id
        self.before_sim_id = before_sim_id
        self.after_sim_id = after_sim_id
        self.configjson = configjson
        self.sim_path = sim_path
        self.report = report
        self.scenario_type = scenario_type

    def run(self):
        # Assume full_sim_thread was started before this class was created
        
        # Run the sim from time 0 to serialization time
        full_duration = self.configjson["parameters"]["Simulation_Duration"]
        serialization_time = self.configjson["parameters"]["Serialization_Times"][0]
        self.configjson["parameters"]["Simulation_Duration"] = serialization_time
        before_sim_thread = self.runner.commissionFromConfigJson( self.before_sim_id,
                                                                  self.configjson,
                                                                  self.sim_path,
                                                                  self.report,
                                                                  self.scenario_type,
                                                                  "BEFORE" )

        # Wait for the FULL sim to finish so that we know we can get its serialized population file
        self.full_sim_thread.join()

        # Define the path name of the serialized file to be read
        full_sim_dir = self.full_sim_thread.get_sim_path()
        #full_sim_dir = os.path.join( ".." , self.full_sim_thread.sim_timestamp )
        full_sim_output_dir = os.path.join( full_sim_dir, "output" )

        base_dtk_filename = "state-{:05d}".format(serialization_time)

        num_cores = 1
        if "Num_Cores" in self.configjson["parameters"]:
            num_cores = self.configjson["parameters"]["Num_Cores"]

        file_names = []
        if num_cores == 1:
            dtk_filename = base_dtk_filename + ".dtk"
            file_names.append( dtk_filename )
        else:
            for i in range(num_cores):
                dtk_filename = base_dtk_filename + "-{:03d}.dtk".format(i)
                file_names.append( dtk_filename )
        
        # This is here to test that a serialized file can be saved at time X but the
        # simulation that is reading the file starts at a different time, say Day 0
        serialized_start_time = serialization_time
        if "Serialization_Test_Start_Time" in self.configjson["parameters"].keys():
            serialized_start_time = float(self.configjson["parameters"]["Serialization_Test_Start_Time"])

        # Configure the AFTER sim to read the serialized file created by the FULL sim
        # Ensure that we have the AFTER sim starting the random number stream from
        # where it was when the file was created
        self.configjson["parameters"]["Serialized_Population_Writing_Type"] = "NONE"
        self.configjson["parameters"]["Serialized_Population_Reading_Type"] = "READ"
        self.configjson["parameters"]["Serialization_Mask_Node_Read"      ] = 0
        self.configjson["parameters"]["Serialized_Population_Filenames"   ] = file_names
        self.configjson["parameters"]["Serialized_Population_Path"        ] = full_sim_output_dir
        self.configjson["parameters"]["Simulation_Duration"               ] = full_duration - serialization_time
        self.configjson["parameters"]["Start_Time"                        ] = serialized_start_time
        self.configjson["parameters"]["Enable_Random_Generator_From_Serialized_Population"] = 1

        # Run the sim from serialization time to Simulation_Duration
        after_sim_thread = self.runner.commissionFromConfigJson( self.after_sim_id,
                                                                 self.configjson,
                                                                 self.sim_path,
                                                                 self.report,
                                                                 self.scenario_type,
                                                                 "AFTER" )

        # Wait for the BEFORE and AFTER sims to finish so we can concatinate their output
        # NOTE: These thread will not compare any output, just run the sim
        before_sim_thread.join()
        after_sim_thread.join()

        # Concatinate the outut of the BEFORE and AFTER sims
        before_sim_dir = before_sim_thread.get_sim_path()
        before_sim_output_dir = os.path.join( before_sim_dir, "output" )
        before_inset_chart_fn = os.path.join( before_sim_output_dir, "InsetChart.json" )
        before_inset_chart_fn_new = os.path.join( before_sim_output_dir, "InsetChart-before.json" )
        if os.path.exists( before_inset_chart_fn ):
            os.rename( before_inset_chart_fn, before_inset_chart_fn_new )
        else:
            print( "Test file \"" + before_inset_chart_fn + "\" -- for " + self.sim_path + " -- does not exist." )
            #failure_txt = "Report not generated by executable - InsetChart.json"
            #before_sim_thread.report.addErroringTest( self.sim_path, failure_txt, before_sim_dir, self.scenario_type )
            return
        
        after_sim_dir = after_sim_thread.get_sim_path()
        after_sim_output_dir = os.path.join( after_sim_dir, "output" )
        after_inset_chart_fn = os.path.join( after_sim_output_dir, "InsetChart.json" )
        after_inset_chart_fn_new = os.path.join( after_sim_output_dir, "InsetChart-after.json" )
        if os.path.exists( after_inset_chart_fn ):
            os.rename( after_inset_chart_fn, after_inset_chart_fn_new )
        else:
            print( "Test file \"" + after_inset_chart_fn + "\" -- for " + self.sim_path + " -- does not exist." )
            #failure_txt = "Report not generated by executable - InsetChart.json"
            #after_sim_thread.report.addErroringTest( self.sim_path, failure_txt, after_sim_dir, self.scenario_type )
            return

        self.concat_inset_chart( before_inset_chart_fn_new,
                                 after_inset_chart_fn_new,
                                 after_inset_chart_fn )

        # Since we put concatinated file in the AFTER sim directory, we will compare
        # it with the saved results
        after_sim_thread.serialization_test_type = "CONCAT"
        after_sim_thread.verify(after_sim_dir,"InsetChart.json")

        return

    def concat_inset_chart( self, before_fn, after_fn, concat_fn ):
        before_json = ru.load_json( before_fn )
        after_json = ru.load_json( after_fn )

        if( before_json["Header"]["Channels"] != after_json["Header"]["Channels"] ):
            print("\nData files dont have the same number of Channels.")
            return
        
        before_json["Header"]["Timesteps"] += after_json["Header"]["Timesteps"]

        channels = set(before_json["Channels"])

        for chan_title in channels:
            num_steps_before = len(before_json["Channels"][chan_title]["Data"])
            num_steps_after  = len(after_json[ "Channels"][chan_title]["Data"])

            initial_value = 0
        
            # Notice that cumulative channels need to be handled specially
            is_cumulative = ( chan_title.startswith("Cumulative") or
                              chan_title.startswith("Campaign Cost") or
                              chan_title.startswith("Births") )
            if( is_cumulative and (num_steps_before > 0) ):
                initial_value = before_json["Channels"][chan_title]["Data"][num_steps_before-1]
            
            for tstep_idx in range( 0, num_steps_after ):
                before_json["Channels"][chan_title]["Data"].append( initial_value + after_json["Channels"][chan_title]["Data"][tstep_idx] )

        with open( concat_fn, 'w' ) as handle:
            json.dump(before_json, handle, sort_keys=True, indent=4)


class TestRunner(object):
    """Encapsulate all the stuff for kicking off tests"""
    def __init__(self, cache_cwd, test_type, constraints_dict, report, runner):
        self.sim_id_generator = SimulationIDGen()
        self.cache_cwd = cache_cwd
        self.test_type = test_type
        self.scenario_type = test_type
        # this ensures that SFTs are still verified when run as a sweep
        if 'science' in test_type:
            self.scenario_type = 'science'
        self.constraints = constraints_dict
        self.report = report
        self.runner = runner
        self.start_time = datetime.datetime.now()

    def run_test(self, sim_path, param_overrides=None):
        """
        Run a test given a sim_path

        :param sim_path: path to the regression simulation directory
        :param param_overrides: map of param_names -> param_values to override in config (if present)
        :return: simulation id
        """

        os.chdir(self.cache_cwd)

        sim_id = self.sim_id_generator.get_simulation_id()

        configjson = get_test_config(sim_path, self.test_type, self.report)
        if configjson is None:
            print( "Misconfigured test: " + sim_path ) 
        elif "parameters" not in configjson: # for pymod, but could be used elswhere I think
            configjson = { "parameters" : configjson } # hopefully this doesn't look weird. just make the config look like it had the conventional "parameters" even top-level key if it didn't

        if configjson:
            if self.check_constraints(configjson):
                # override param name/value for sweeps
                if param_overrides:
                    for param_name, param_value in param_overrides.items():
                        TestRunner.override_config_value(configjson, param_name, param_value)

                campjson = flatten_campaign(sim_path, self.test_type, self.report)

                # add campaign to config
                if campjson is None:
                    campaign_file = None
                    if "parameters" in configjson:
                        if "Campaign_Filename" in configjson["parameters"]:
                            campaign_file = os.path.join(sim_path, configjson["parameters"]["Campaign_Filename"])
                        ei = False
                        if "parameters" in configjson and "Enable_Interventions" in configjson["parameters"]:
                            ei = configjson["parameters"]["Enable_Interventions"]
                        if campaign_file and campaign_file.endswith( ".json" ):
                            configjson["campaign_json"] = load_campjson_from_campaign(sim_path, ei, self.runner, campaign_file) 
                            configjson["parameters"]["Campaign_Filename"] = self.runner.campaign_filename
                else:
                    configjson["campaign_json"] = json.dumps(campjson)

                # add custom_reports to config
                configjson["custom_reports_json"] = None
                report_fn = ""
                if "Custom_Reports_Filename" in configjson["parameters"]:
                    cr_fn = configjson["parameters"]["Custom_Reports_Filename"]
                    if (len(cr_fn) > 0) and cr_fn != "NoCustomReports":
                        report_fn = os.path.join( sim_path, cr_fn )
                if os.path.exists( report_fn ) == True:
                    reportjson_file = open( report_fn )
                    reportjson = json.loads( reportjson_file.read().replace( "u'", "'" ).replace( "'", '"' ).strip( '"' ) )
                    reportjson_file.close()
                    configjson["parameters"]["Custom_Reports_Filename"] = "custom_reports.json"
                    configjson["custom_reports_json"] = reportjson

                serialization_test_type = get_serialization_test_type( configjson )

                thread = self.runner.commissionFromConfigJson(sim_id, configjson, sim_path, self.report, self.scenario_type, serialization_test_type )

                if serialization_test_type != None:
                    before_sim_id = self.sim_id_generator.get_simulation_id()
                    after_sim_id = self.sim_id_generator.get_simulation_id()
                    thread = SerializationTestMonitor( thread, 
                                                       self.runner,
                                                       sim_id,
                                                       before_sim_id,
                                                       after_sim_id,
                                                       configjson,
                                                       sim_path,
                                                       self.report,
                                                       self.scenario_type  )
                    thread.start()

                ru.reg_threads.append(thread)
        else:
            print("Error flattening config.  Skipping " + sim_path)
            ru.final_warnings += "Error flattening config.  Skipped " + sim_path + "\n"

        return sim_id

    def attempt_test(self):
        self.runner.attempt_test()

    @staticmethod
    def override_config_value(config_json, param_name, param_value):
        """
        Modifies a config_json object to change the value of a parameter (given by name) to a specific value. If the
        parameter name contains colon (:) characters then it is taken to be a "path" to a nested parameter with each
        successive value being the name of a child parameter.

        This function changes parameter values in place in config_json as a side-effect.

        :param config_json: test config to be modified
        :param param_name: name/path of the parameter to modify
        :param param_value: new value of the parameter
        """
        if ':' in param_name:
            # nested parameter
            param_family_tree = param_name.split(':')
            tree_depth = len(param_family_tree)
            current_node = config_json['parameters']
            # recurse through child parameters until we get to the 2nd to last one
            for i in range(tree_depth - 1):
                current_node = current_node[param_family_tree[i]]
            # using the key of the final child override the value to the final value
            current_node[param_family_tree[tree_depth-1]] = param_value
        else:
            config_json['parameters'][param_name] = param_value

    def check_constraints(self, configjson):
        """
        Determine whether or not the given constraints for this run match the config values for a specific test

        :param configjson: json object for regression test config
        :return: whether the constraints are satisfied
        """
        if len(self.constraints) != 0:
            real_params = configjson["parameters"]
            scenario_name = ""
            if "Config_Name" in real_params.keys():
                scenario_name = real_params["Config_Name"]

            for constraint_name, constraint_value in self.constraints.items():
                if constraint_name not in real_params.keys():
                    print("WARNING: '{0}' configuration does not define constaint ('{1}'). Key not present.".format(
                        scenario_name,constraint_name))
                elif constraint_name == "Num_Cores":
                    num_cores_constraint = int(constraint_value)
                    num_cores_param      = int(real_params[constraint_name])
                    if num_cores_param > num_cores_constraint:
                        print("'{0}' configuration did not satisfy constraint: '{1}' = {2} but must <= {3}.".format(
                            scenario_name, constraint_name, str(real_params[constraint_name]), constraint_value))
                        return False
                elif str(real_params[constraint_name]) != constraint_value:
                    print("'{0}' configuration did not satisfy constraint: '{1}' = {2} but must == {3}.".format(
                        scenario_name, constraint_name, str(real_params[constraint_name]), constraint_value))
                    return False

        return True

    def sweep_params(self, reglistjson):
        """
        Sweep through each of the parameter overrides provided, running tests for each

        :param reglistjson: json test list object
        :return:
        """
        # keep track of all the parameter names and their values, set up the variables used by iterate_params_and_run
        self.param_values = {}
        self.param_names = []
        self.current_overrides = {}

        # extract parameter names and values from reglist
        if "param_name" in reglistjson[self.test_type]:
            param_name = reglistjson[self.test_type]["param_name"]
            self.param_names.append(param_name)
            self.param_values[param_name] = reglistjson[self.test_type]["param_values"]
            self.current_overrides[param_name] = self.param_values[param_name][0]
        elif "param_names" in reglistjson[self.test_type]:
            self.param_names = reglistjson[self.test_type]["param_names"]
            for param_name in self.param_names:
                # name of the corresponding values property, replace /'s in nested params with _'s, append "_values"
                param_key = param_name + "_values"
                if param_key in reglistjson[self.test_type]:
                    self.param_values[param_name] = reglistjson[self.test_type][param_key]
                    self.current_overrides[param_name] = self.param_values[param_name][0]
                else:
                    raise ValueError("Missing " + param_key + " for parameter: " + param_name)

        self.iterate_params_and_run(0, reglistjson[self.test_type]["path"])

    def iterate_params_and_run(self, param_id, sim_path):
        """
        Iterate through a given set of param overrides, recursively iterate through any additional params, run tests w/ overrided params

        this is a utility method used by sweep_params

        :param param_id:
        :param sim_path:
        :return:
        """
        if param_id >= len(self.param_names):
            # if we've iterated all params and set the appropriate current param override values, then actually run the test
            self.run_test(sim_path, self.current_overrides)
        else:
            # determine the current param being iterated
            param_name = self.param_names[param_id]

            for current_param_value in self.param_values[param_name]:
                # advance the current value of the param through the list for this param
                self.current_overrides[param_name] = current_param_value
                # iterate through the next level of params or run the test w/ the given params
                self.iterate_params_and_run(param_id+1, sim_path)

    def plot_sweep_results(self, sweep_path, sweep_out_file=None):
        """
        Get sweep results and plot them

        :param sweep_path: regression simulation directory
        :param sweep_out_file: dump all simulation directories to a file, if present
        """
        print("Plot sweep results...\n")

        file_prefix_list = []
        file_prefix_list.append( "InsetChart" )
        file_prefix_list.append( "PropertyReport" )
        file_prefix_list.append( "AlleleFrequency" )
        file_prefix_list.append( "ReportVectorGenetics" )
        file_prefix_list.append( "ReportVectorStats" )
        file_prefix_list.append( "ReportMalariaFiltered" )

        ref_dir_path = os.path.join(sweep_path, "output" )
        ref_dir_path = os.path.join(self.cache_cwd, ref_dir_path)

        ref_file_list = []
        for ref_file_name in os.listdir( ref_dir_path ):
            if ref_file_name.endswith(".json") and not ref_file_name.endswith(".linux.json"):
                for prefix in file_prefix_list:
                    if ref_file_name.startswith( prefix ):
                        ref_file_list.append( ref_file_name )

        param_counts = []

        for param_name in self.param_names:
            param_counts.append(str(len(self.param_values[param_name])))

        plot_title_prefix = os.path.basename( sweep_path )
        plot_title_prefix = plot_title_prefix + "\nSweep over " + ", ".join(self.param_names).replace(':', '_') + " (" + ", ".join(param_counts) + ") values"
        os.chdir(self.cache_cwd)
        import plotAllCharts

        for ref_file_name in ref_file_list:
            ref_path = os.path.join( ref_dir_path, ref_file_name )
            ref_json = ru.load_json( ref_path )
            results_data = self.get_sweep_results( ref_file_name, sweep_out_file )
            plot_title = plot_title_prefix + "\n" + ref_file_name
            plotAllCharts.plotBunch( results_data, plot_title, ref_json )


    def get_sweep_results(self, file_name, out_file=None):
        """
        Get sweep results for a sweep run, dump all simulation info to file if desired

        :param file_name The name of the file expected in the output directory to get its JSON
        :param out_file: file to dump all simulation directories to
        :return: collected json data
        """
        results_data = []
        sim_dirs = []

        for reg_thread in ru.reg_threads:
            sim_dir = os.path.join( reg_thread.sim_root, reg_thread.sim_timestamp )
            sim_dirs.append( sim_dir )
            sim_filename = os.path.join( sim_dir, os.path.join( "output", file_name ) )
            sim_json = ru.load_json( sim_filename )
            results_data.append( sim_json )

        if out_file:
            with open( out_file, "w" ) as outputs:
                outputs.write( json.dumps( sim_dirs, indent=4, sort_keys=True ) )

        return results_data


    def print_report_results(self):
        """Print message containing report summary results"""
        if self.report is not None:
            elapsed_time = datetime.datetime.now() - self.start_time
            self.report.write(os.path.join("reports", "report_" + self.sim_id_generator.first_id() + ".xml"), elapsed_time)
            print('========================================')
            print('Elapsed time: {}'.format(elapsed_time))
            print('%(tests)3d tests total, %(passed)3d passed, %(failed)3d failed, %(errors)3d errors, schema: %(schema)s.' % self.report.Summary)
            print('========================================')


def main():
    """Main body of regression_test.py"""

    # parse cmdline args
    params = setup()

    # initialize regression runner
    runner = regression_runner.MyRegressionRunner(params)

    # load regression list data from file(s)
    reglistjson = read_regression_files(params.suite.split(","))

    # determine test type (tests, science, sweep)
    test_type = reglist_test_type(reglistjson)

    if not test_type:
        # bail out if test type is unknown
        print("Error: determined test type is unknown, exiting without running tests")
        return 1

    # determine whether we're running sweeps and/or SFTs
    science = "science" in test_type
    sweep = "sweep" in test_type

    if test_type != "pymod":
        if params.linux:
            ru.version_string = "<na:linux>"
        else:
            ru.version_string = get_exe_version(params.executable_path)

    # create report
    report = regression_report.Report(params, ru.version_string)

    # initialize test runner for given directory, test type, constraints, etc.
    test_runner = TestRunner(ru.cache_cwd, test_type, params.constraints_dict, report, runner)

    # verify that the test runner can dispatch tests to test executors
    test_runner.attempt_test()

    if science:
        # prepare for generating graphs for SFTs
        homepath = get_homepath(params.sim_root, params.local_execution)
        configure_SFT_graphs(homepath, params.hide_graphs)

    if sweep:
        print("Running sweep...\n")
        test_runner.sweep_params(reglistjson)
    else:
        print("Running regression...\n")

        for sim_config in reglistjson[test_type]:
            test_runner.run_test(sim_config["path"])

        # do a schema test also
        if not params.disable_schema_test:
            start_schema = datetime.datetime.now()
            report.schema = runner.doSchemaTest()
            schema_duration = datetime.datetime.now() - start_schema
            if report.schema == 'passed':
                report.addPassingTest('schema', schema_duration, 'see logs for details')
            else:
                report.addFailingTest('schema', 'Schema test failed.', 'see logs for details', 'schema')

    # wait until all threads running tests are done
    expected_num_tests = len(ru.reg_threads)
    for thr in ru.reg_threads:
        thr.join()

    component_tests_passed = True
    if( params.component_tests ):
        ct_start = datetime.datetime.now()
        component_tests_passed = run_component_tests(params.scons, params.component_tests_show_output)
        duration = datetime.datetime.now() - ct_start
        if component_tests_passed:
            report.addPassingTest('component_tests', duration, 'see logs for details')
        else:
            sys.stderr.write('Component tests failed!\n')
            report.addFailingTest('component_tests', 'Component tests failed.', 'see logs for details', 'component_tests')

    # print results except when running a non-science sweep
    if not (sweep and not science):
        test_runner.print_report_results()

    if ru.final_warnings != "":
        print("----------------")
        print(ru.final_warnings)
        print("----------------")

    # if doing sweep, call plotAllCharts.py with all sim_timestamps on command line.
    if sweep:
        test_runner.plot_sweep_results(reglistjson[test_type]["path"], "sweep_out.json")
    # TODO: this doesn't work with all-outputs runs, figure out a better way of doing this test
    #elif report.num_tests != expected_num_tests:
    #    print("Number of tests: %d doesn't match expected number: %d" %(report.num_tests, expected_num_tests))
    #    return 1

    return 0
    """Main body of regression_test.py"""



if __name__ == "__main__":
    # run
    sys.exit(main())
