#!/usr/bin/python

import sys
import os
import subprocess
import datetime
import threading
import json
import tempfile
from pathlib import Path
from hashlib import md5
import pdb
import io

import regression_utils as ru

MAX_ACTIVE_JOBS=10
class Monitor(threading.Thread):
    sems = threading.Semaphore( MAX_ACTIVE_JOBS )
    completed = 0

    def __init__(self, sim_id, scenario_path, report, params, config_json=None, scenario_type='tests', serialization_test_type=None):
        threading.Thread.__init__( self )
        #print "Running DTK execution and monitor thread."
        sys.stdout.flush()
        self.sim_timestamp = sim_id
        self.scenario_path = scenario_path
        self.report = report
        self.config_json = config_json
        self.duration = None
        # can I make this static?
        self.params = params
        self.scenario_type = scenario_type
        self.sim_dir = None
        self.serialization_test_type = serialization_test_type

    def get_input_path_from_geog( self, input_root ):
        #actual_input_dir = "."
        actual_input_dir = None
        if "Geography" in self.config_json["parameters"]:
            if self.config_json["parameters"]["Geography"] == "LOCAL": # not used yet but forward-looking proposal for non-geography scenarios
                actual_input_dir = "."
            else:
                actual_input_dir = ".;" + os.path.join( input_root, self.config_json["parameters"]["Geography"] )
        return actual_input_dir 

    def get_num_cores( self ):
        num_cores = 1
        if ('parameters' in self.config_json) and ('Num_Cores' in self.config_json['parameters']):
            num_cores = self.config_json['parameters']['Num_Cores']
        else:
           print( "Didn't find key 'parameters/Num_Cores' in '{0}'. Using 1.".format( self.scenario_path ) )
               
        return int(num_cores)
    
    def run(self):
        self.__class__.sems.acquire()
        self.sim_root = self.params.local_sim_root
        self.sim_dir = os.path.join( self.sim_root, self.sim_timestamp )
        numcores = self.get_num_cores()
        #os.chdir( self.sim_dir )    # NOT THREAD SAFE!

        starttime = datetime.datetime.now()

        stdoutfile = "stdout.txt"
        if self.scenario_type != 'tests':
            stdoutfile = "test.txt"
        with open(os.path.join(self.sim_dir, stdoutfile), "w") as stdout, open(os.path.join(self.sim_dir, "stderr.txt"), "w") as stderr:
            actual_input_dir = self.get_input_path_from_geog( self.params.input_path )
            # Call Eradication.exe through mpiexec to avoid Windows security warnings (see GitHub issue #1439)
            cmd = None
            if "Eradication" in self.config_json["bin_path"]:
                cmd = ['mpiexec', "-n", str(numcores), self.config_json["bin_path"], "-C", "config.json" ]
            else:
                cmd = self.config_json["bin_path"].split()
                if self.scenario_type != 'pymod':
                    cmd.extend( ["-C", "config.json" ] )
            if actual_input_dir:
                cmd.extend( [ "--input-path", actual_input_dir ] )
            # python-script-path is optional parameter.
            if "PSP" in self.config_json:
                cmd.extend( [ "--python-script-path", self.config_json["PSP"] ] )
            #print( "Calling '" + str(cmd) + "' from " + self.sim_dir + "\n" )
            print( "Running '" + str(self.config_json["parameters"]["Config_Name"]) + "' in " + self.sim_dir + "\n" )
            shell_val = False
            #if self.scenario_type == 'pymod' and self.params.local_execution:
            #    shell_val = True

            proc = subprocess.Popen( cmd, stdout=stdout, stderr=stderr, cwd=self.sim_dir, shell=shell_val )
            proc.wait()
            if (os.name != "nt") and (proc.returncode == 134) and ("40_STI_Trivial_All_Reporters" in self.scenario_path):
                print("40_STI_Trivial_All_Reporters: expected issue seen")
                # The expected issue is a double-free corruption issue with using shared objects on Linux
                # The sim can run completely fine but run into the error when destructing the objects.
                # We want to keep the test because it is the only one exercising the use of shared objects.
            elif proc.returncode != 0:
                err_str = "received error code=" +str(proc.returncode)
                self.report.addErroringTest( self.scenario_path, err_str, self.sim_dir, self.scenario_type )
                
        # JPS - do we want to append config_json["parameters"]["Geography"] to the input_path here too like we do in the HPC case?
        endtime = datetime.datetime.now()
        self.duration = endtime - starttime
        os.chdir( ru.cache_cwd )
        self.__class__.completed = self.__class__.completed + 1
        print( str(self.__class__.completed) + " out of " + str(len(ru.reg_threads)) + " completed." )
        # JPS - should check here and only do the verification if it passed... ?
        self.verify_test()

        self.__class__.sems.release()

    def verify_test( self ):
        if self.scenario_type == 'tests':
            if self.params.all_outputs == False:
                # Following line is for InsetChart.json only
                self.verify(self.sim_dir)
            else:
                # Every .json file in output (not hidden with . prefix) will be used for validation
                for file in os.listdir( os.path.join( self.scenario_path, "output" ) ):
                    known_file = (file.endswith( ".json" ) or
                                  file.endswith( ".csv"  ) or
                                  file.endswith( ".kml"  ) or
                                  file.endswith( ".npy"  ) or
                                  file.endswith( ".txt"  ) or
                                  file.endswith( ".bin"  ) or
                                  file.endswith( ".h5"   ) or
                                  file.endswith( ".db"   ))
                    if known_file and (file[0] != ".") and (file != "transitions.json"):
                        self.verify( self.sim_dir, file, "Channels" )
        elif self.scenario_type == 'science':   # self.report <> None:
            self.science_verify( self.sim_dir )
        elif self.scenario_type == 'pymod':   # self.report <> None:
            self.pymod_verify( self.sim_dir )

    def get_json_data_hash( self, data ):
        with tempfile.TemporaryFile() as handle:
            json_data = json.dumps( obj=data ).encode('utf-8')
            handle.write( json_data )
            hash = ru.md5_hash( handle )
        return hash

    def compareJsonOutputs( self, sim_dir, report_name, ref_path, test_path, failures ):
        fail_validation = False
        failure_txt = ""


        try:
            ru.load_json( os.path.join(ru.cache_cwd, ref_path) )
        except Exception:
            fail_validation = True
            failure_txt = "Exception {0} {1} loading json file: {2}.".format(sys.exc_info()[0], sys.exc_info()[1], (os.path.join(ru.cache_cwd, ref_path)))
            print(failure_txt)
            return True, failure_txt

        ref_json = ru.load_json( os.path.join( sim_dir, ref_path ) )

        if "Channels" not in ref_json.keys():
            # switched from MD5 to equality so files generated on different OS can still pass
            # without dealing with line endings
            test_json = ru.load_json( os.path.join( sim_dir, test_path ) )
            if ref_json == test_json:
                return False, ""
            else:
                stt = ""
                if self.serialization_test_type != None:
                    stt = " " + self.serialization_test_type
                print( self.scenario_path + stt + " completed but did not match reference! (" + str(self.duration) + ") - " + report_name )
                return True, "Non-Channel JSON failed equality."
        else:
            test_json = ru.load_json( os.path.join( sim_dir, test_path ) )

            if "Channels" not in test_json.keys():
                return True, "Reference has Channel data and Test file does not."

            ref_md5  = self.get_json_data_hash( ref_json["Channels"] )
            test_md5 = self.get_json_data_hash( test_json["Channels"] )

            ref_channels = set(ref_json["Channels"])
            test_channels = set(test_json["Channels"])

            if ref_md5 == test_md5:
                return False, ""

            missing_channels = ref_channels - test_channels
            new_channels = test_channels - ref_channels

            if len(missing_channels) > 0:
                #fail_validation = True
                print("ERROR: Missing channels - " + ', '.join(missing_channels))
                failure_txt += "Missing channels:\n" + '\n'.join(missing_channels) + "\n"
                self.report.addFailingTest( self.scenario_path, failure_txt, os.path.join( sim_dir, ( "output/" + report_name ) ), self.scenario_type, self.serialization_test_type )
                failures.append("ERROR: Missing channels - " + ', '.join(missing_channels))

            if len(new_channels) > 0:
                print("WARNING: The test "+report_name+" has " + str(len(new_channels)) + " channels not found in the reference.  Please update the reference "+report_name+".")
                ru.final_warnings += self.scenario_path + " - New channels not found in reference:\n  " + '\n  '.join(new_channels) + "\nPlease update reference from " + os.path.join( sim_dir, os.path.join( "output", report_name ) ) + "!\n"
                self.report.addFailingTest( self.scenario_path, failure_txt, os.path.join( sim_dir, ( "output/" + report_name ) ), self.scenario_type, self.serialization_test_type )

            if "Header" in ref_json.keys() and ref_json["Header"]["Timesteps"] != test_json["Header"]["Timesteps"]:
                warning_msg = "WARNING: test "+report_name+" has timesteps " + str(test_json["Header"]["Timesteps"])  + " DIFFERRING from ref "+report_name+" timesteps " + str(ref_json["Header"]["Timesteps"]) + "!\n"
                if self.params.hide_graphs:
                    # This is treated as automated running mode (or bamboo nightly build mode)
                    fail_validation = True
                    failure_txt += warning_msg
                else:
                    # This is treated as manual running mode
                    ru.final_warnings += warning_msg
                    print(warning_msg)

            if not fail_validation:
                #print( "Hasn't failed validation on second level review. Time to look channel by channel, timestep by timestep." )
                # BinnedReport and its derived classes have "Subchannel_Metadata" in the header
                if "Header" in ref_json.keys() and "Subchannel_Metadata" in ref_json["Header"].keys():
                    self.compareBinnedReportType( ref_json, test_json, failures )
                elif "Header" in ref_json.keys() and "Report_Type" in ref_json["Header"].keys() and ref_json["Header"]["Report_Type"] =="InsetChart":
                    # Assuming a BaseChannelReport
                    self.compareChannelReportType( ref_json, test_json, failures )
                else:
                    fail_validation = True
                    failures.append(report_name + " - Files are different but cannot do deep dive.")

            if len(failures) > 0:
                fail_validation = True
                failure_txt += "Channel Timestep Reference_Value Test_Value\n" + ''.join(failures)
                stt = ""
                if self.serialization_test_type != None:
                    stt = " " + self.serialization_test_type
                print( self.scenario_path + stt + " completed but did not match reference! (" + str(self.duration) + ") - " + report_name )

        return fail_validation, failure_txt

    def compareCsvOutputs( self, ref_path, test_path, failures ):
        # print( "Comparing CSV files: ref = " + ref_path + ", test = " + test_path )
        # Do Md5 comp first.
        ref_md5 = ru.md5_hash_of_file( ref_path )
        test_md5 = ru.md5_hash_of_file( test_path )
        if ref_md5 == test_md5:
            # print( "CSV files passed MD5 comparison test." )
            return False, ""

        fail_validation = False
        err_msg = ""

        # print( "CSV files failed MD5 comparison test." )
        # First (md5) test failed. Do line length, then line-by-line
        ref_length = ru.file_len( ref_path )
        test_length = ru.file_len( test_path )
        if ref_length != test_length:
            fail_validation = True
            err_msg = "Reference output {0} has {1} lines but test output {2} has {3} lines".format( ref_path, ref_length, test_path, test_length )

        else:
            with open(ref_path, "r") as ref_file, open(test_path, "r") as test_file:
                line_num = 0
                for ref_line in ref_file:
                    line_num = line_num + 1
                    test_line = test_file.readline().rstrip()
                    ref_line = ref_line.rstrip()
                    if ref_line != test_line:
                        ref_line_tokens = ref_line.split(',')
                        test_line_tokens = test_line.split(',')
                        if len(ref_line_tokens) != test_line_tokens:
                            err_msg = "First mismatch at line {0} of {1}:\nDifferent number of columns: {2} vs {3}:\nreference line...\n{4}\nvs test line...\n{5}".format( line_num, ref_path, len(ref_line_tokens), len(test_line_tokens), ref_line, test_line )
                            fail_validation = True
                            break
                        for col_idx in range( len( ref_line_tokens) ):
                            if ref_line_tokens[col_idx] != test_line_tokens[col_idx]:
                                break
                        err_msg = "First mismatch at line {0} of {1} column {2}: reference line...\n{3}vs test line...\n{4}{5} vs {6}".format( line_num, ref_path, col_idx, ref_line, test_line, ref_line_tokens[col_idx], test_line_tokens[col_idx] )
                        fail_validation = True
                        break

        print( err_msg )
        failure_txt = err_msg
        #self.report.addFailingTest( self.scenario_path, failure_txt, test_path, self.scenario_type )
        return fail_validation, failure_txt

    def compareOtherOutputs( self, report_name, ref_path, test_path, failures ):
        ref_md5 = ru.md5_hash_of_file( ref_path )
        test_md5 = ru.md5_hash_of_file( test_path )
        if ref_md5 == test_md5:
            # print( "CSV files passed MD5 comparison test." )
            return False, ""
        else:
            print( self.scenario_path + " completed but did not match reference! (" + str(self.duration) + ") - " + report_name )
            return True, "Failes MD5 check."

    # Compare Binned Report Types
    def compareBinnedReportType( self, ref_json, test_json, failures ):
        num_bins_ref  = ref_json[ "Header"]["Subchannel_Metadata"]["NumBinsPerAxis"][0][0]
        num_bins_test = test_json["Header"]["Subchannel_Metadata"]["NumBinsPerAxis"][0][0]

        if num_bins_ref != num_bins_test:
            error_txt = report_name + ": Reference(NumBinsPerAxis=" + str(num_bins_ref) +") != Test(NumBinsPerAxis="+ str(num_bins_test) +")"
            print( error_txt )
            failures.append( error_txt )
        else:
            ref_channels  = set(ref_json[ "Channels"])
            test_channels = set(test_json["Channels"])

            min_tstep_ind = min(ref_json["Header"]["Timesteps"], test_json["Header"]["Timesteps"])

            for chan_title in (ref_channels & test_channels):
                num_lines = 10
                num_skipped = 0
                for bin_idx in range( 0, num_bins_ref ):
                    for tstep_idx in range( 0, min_tstep_ind ):
                        val_ref  = ref_json[ "Channels"][chan_title]["Data"][bin_idx][tstep_idx]
                        val_test = test_json["Channels"][chan_title]["Data"][bin_idx][tstep_idx]
                        if val_ref != val_test:
                            if num_lines > 0:
                                failures.append(chan_title + " " + str(bin_idx) + " " + str(tstep_idx) + " " + str( val_ref ) + " " + str( val_test ) + "\n")
                                num_lines -= 1
                            else:
                                num_skipped += 1
                if num_skipped > 0:
                    failures.append( "And another " + str( num_skipped  ) + " lines skipped.\n" )
        return

    # Compare Channel Report Types
    def compareChannelReportType( self, ref_json, test_json, failures ):
        # Use some value N as the max number of rows to include in the report for each channel
        # With an additional summary line stating how many rows were cropped. 
        # Should make N overrideable with values from an a .rtpy config file?
        ref_channels  = set(ref_json[ "Channels"])
        test_channels = set(test_json["Channels"])

        min_tstep_ind = min(ref_json["Header"]["Timesteps"], test_json["Header"]["Timesteps"])

        for chan_title in (ref_channels & test_channels):
            #print( "Looking at channel {0}".format( chan_title ) )
            num_steps_ref  = len(ref_json["Channels"][chan_title]["Data"])
            num_steps_test = len(test_json["Channels"][chan_title]["Data"])
            if( (min_tstep_ind > num_steps_ref) or (min_tstep_ind > num_steps_test) ):
                failures.append("Reference has "+str(num_steps_ref) + " steps and test has "+str(num_steps_test)+" steps, but the header says the min Timesteps is "+str(min_tstep_ind))
                print("!!!! Reference["+str(chan_title)+"] has "+str(num_steps_ref) + " steps and test has "+str(num_steps_test)+" steps, but the header says the min Timesteps is "+str(min_tstep_ind))
                return
            num_lines = 10
            num_skipped = 0
            for tstep_idx in range( 0, min_tstep_ind ):
                if test_json["Channels"][chan_title]["Data"][tstep_idx] != ref_json["Channels"][chan_title]["Data"][tstep_idx]:
                    if num_lines > 0:
                        failures.append(chan_title + " " + str(tstep_idx) + " " + str( ref_json["Channels"][chan_title]["Data"][tstep_idx] ) + " " + str( test_json["Channels"][chan_title]["Data"][tstep_idx] ) + "\n")
                        num_lines -= 1
                    else:
                        num_skipped += 1
            if num_skipped > 0:
                failures.append( "And another " + str( num_skipped  ) + " lines skipped.\n" )
        return

    def get_sim_path(self):
        return self.sim_dir

    # Adding optional report_name parameter, defaults to InsetChart
    def verify(self, sim_dir, report_name="InsetChart.json", key="Channels" ) -> bool:
        #print( "Checking if report " + report_name + " based on key " + key + " matches reference..." )
        # check if insetchart matched
        # since ICJ now has header, just calculate md5 on data section
        # Read whole file and write channel data to temp file. Calculate md5 on that.
        
        # different reports obviously have different structure. The only report we currently have defailt 
        # structural knowledge about is InsetChart, but we should know the top level keys. PolioPatientSurvey
        # has "patient_array" as the top level key. 
        fail_validation = False
        failures = []
        failure_txt = ""

        # if nothing to compare then can't fail
        if self.report is None:
            return True

        test_path = os.path.join( self.get_sim_path(), os.path.join( "output", report_name ) )
        ref_path = os.path.join( ru.cache_cwd, os.path.join( str(self.scenario_path), os.path.join( "output", report_name ) ) )

        # Do not compare the partial sims because the save output is for the full sim
        if (self.serialization_test_type == "BEFORE") or (self.serialization_test_type == "AFTER"):
            return True

        # we will compare to InsetChart.linux.json when we look at the InsetChart.json report on linux by changing
        # the name of the InsteChart.json and looking for the .linux version in the references
        if report_name == "InsetChart.linux.json":
            return True

        # If on linux, check if there is a .linux version of reference report in folder - test against it if there is
        # This is currently only used for InsetChart.json
        if os.name != "nt" or self.params.linux:
            linux_name = str(Path(report_name).with_suffix(f'.linux{Path(report_name).suffix}'))
            alt_ref_path = os.path.join( ru.cache_cwd, os.path.join( str(self.scenario_path), os.path.join( "output", linux_name ) ) )
            if os.path.exists( alt_ref_path ):
                ref_path = alt_ref_path
        elif report_name != "InsetChart.json":
            print( "WARNING: Running on Windows - all reports that are not 'InsetChart.json' have been created on "
                   "Linux for testing on linux and will likely fail validation due to differences in the systems\n" )

        if not os.path.exists(test_path):
            print( "Test file \"" + test_path + "\" -- for " + self.scenario_path + " -- does not exist." )
            failure_txt = "Report not generated by executable - " + report_name
            self.report.addFailingTest( self.scenario_path, failure_txt, sim_dir, self.scenario_type, self.serialization_test_type )
            return False

        if test_path.endswith( ".csv" ):
            fail_validation, failure_txt = self.compareCsvOutputs( ref_path, test_path, failures )

        elif test_path.endswith( ".json" ):
            fail_validation, failure_txt = self.compareJsonOutputs( self.get_sim_path(), report_name, ref_path, test_path, failures )

        elif test_path.endswith( ".kml" ) or\
             test_path.endswith( ".bin" ) or\
             test_path.endswith( ".npy" ) or\
             test_path.endswith( ".txt" ) or\
             test_path.endswith( ".h5"  ) or\
             test_path.endswith( ".db" ):
            fail_validation, failure_txt = self.compareOtherOutputs( report_name, ref_path, test_path, failures )

        if fail_validation:
            #print( "Validation failed, add to failing tests report." )
            self.report.addFailingTest( self.scenario_path, failure_txt, os.path.join( sim_dir, ( "output/" + report_name ) ), self.scenario_type )

            plotable_report = (report_name.startswith( "InsetChart" ) or
                               report_name.startswith( "ReportMalariaFiltered" ) or
                               report_name.startswith( "ReportVectorStats.json" ) or
                               report_name.startswith( "ReportVectorGenetics_arabiensis_Female_GENOME.json" ) or
                               report_name.startswith( "AlleleFrequency.json" ))
            if len(failures) > 0 and not self.params.hide_graphs and plotable_report:
                #print( "Plotting charts for failure deep dive." )  
                # Note: Use python version 2 for plotAllCharts.py
                subprocess.Popen( ["python", "plotAllCharts.py", ref_path, test_path ] )
        else:
            ser_test_str = ""
            if (self.serialization_test_type == "FULL") or (self.serialization_test_type == "CONCAT"):
                ser_test_str = " " + self.serialization_test_type
            print( self.scenario_path + ser_test_str + " passed (" + str(self.duration) + ") - " + report_name )
            self.report.addPassingTest(self.scenario_path, self.duration, os.path.join(sim_dir, ("output/" + report_name)))
            
            if ru.version_string is not None:
                try:
                    with open( os.path.join( self.scenario_path, "time.txt" ), 'a' ) as timefile:
                        timefile.write(ru.version_string + '\t' + str(self.duration) + '\n')
                except Exception:
                    print("Problem writing time.txt file (repeat of error Jonathan was seeing on linux?)\n")
                    print("{} - {}".format(sys.exc_info()[0], sys.exc_info()[1]))

        return True

    def science_verify( self, sim_dir ):
        #print( "Scientific Feature Testing: check scientific_feature_report.txt" )
        report_name = "scientific_feature_report.txt"
        sfr = os.path.join( sim_dir, report_name )
        if os.path.exists( sfr ):
            with open( sfr ) as sfr_file:
                sfr_data = sfr_file.read()
                if "SUMMARY: Success=True" in sfr_data:
                    print( self.scenario_path + " passed (" + str(self.duration) + ") - " + report_name )
                    #print( self.scenario_path + " passed." )
                    self.report.addPassingTest(self.scenario_path, self.duration, os.path.join(sim_dir, report_name))
                    #os.remove( os.path.join( sim_dir, "test.txt" ) )
                else:
                    fail_text = self.scenario_path + " SFT failed."
                    print( self.scenario_path + " failed (" + str(self.duration) + ") - " + report_name )
                    print( sfr_data )
                    self.report.addFailingTest( self.scenario_path, fail_text, os.path.join( sim_dir, report_name ), self.scenario_type )
        else:
            print( self.scenario_path + " failed (" + str(self.duration) + ") - " + report_name + " not generated. This could mean an error in the dtk_post_process.py script, imported scripts, including import errors, which can include not finding a shared python module in the import path.")
            self.report.addFailingTest( self.scenario_path, "No " + report_name, os.path.join( sim_dir, report_name ), self.scenario_type )
        

    def pymod_verify( self, sim_dir ):
        # pymod verification, which consists entirely of looking for an 'OK' at the end of the stdout which happens to be StdErr.txt
        fail = True
        for report_name in [ "stderr.txt", "test.txt" ]:
            pmr = os.path.join( sim_dir, report_name )
            if os.path.exists( pmr ):
                with open( pmr ) as pmr_file:
                    line = None
                    for line in pmr_file:
                        pass
                    pmr_data = line
                    if pmr_data is not None and pmr_data.strip() == "OK":
                        print( self.scenario_path + " passed (" + str(self.duration) + ") - " + report_name )
                        self.report.addPassingTest(self.scenario_path, self.duration, os.path.join(self.sim_dir, report_name))
                        fail = False
            #else:
               #print( "Failed to find 'report' file for pymod test: " + report_name )
        if fail:
            report_name = "test.txt"
            fail_text = self.scenario_path + " PyMod failed."
            print( self.scenario_path + " failed (" + str(self.duration) + ") - " + report_name )
            #print( pmr_data )
            self.report.addFailingTest( self.scenario_path, fail_text, os.path.join( self.sim_dir, report_name ), self.scenario_type )
