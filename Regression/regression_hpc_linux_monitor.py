#!/usr/bin/python

import subprocess
import datetime
import json
import time
import os
import pdb

import regression_local_monitor
import regression_utils as ru
import regression_clg as clg

class LinuxHpcMonitor(regression_local_monitor.Monitor):
    def __init__(self, sim_id, scenario_path, report, params, suffix, config_json=None, scenario_type='tests', serialization_test_type=None,
                 priority='Normal'):
        #super(regression_local_monitor.Monitor,self).__init__( sim_id, scenario_path, report, params, config_json, scenario_type )
        regression_local_monitor.Monitor.__init__( self, sim_id, scenario_path, report, params, config_json, scenario_type, serialization_test_type )
        #print "Running DTK execution and monitor thread for HPC commissioning."
        self.sim_root = self.transform_path( self.params.sim_root ) # override base Monitor which uses local sim directory
        #print( "sim_root = " + self.sim_root )
        self.config_json = config_json
        self.scenario_path = scenario_path
        self.suffix = suffix
        self.options = {'/priority:': priority}

        # Prepare job command line options as they're available at this point
        self.prepare_options()

    def transform_path( self, win_path ):
        return win_path.replace( "\\", "/" ).replace( "bayesianfil01", "mnt" ).replace( "IDM", "idm" ).replace( "//", "/" )

    def get_sim_path( self ):
        return os.path.join( self.params.sim_root, self.sim_timestamp )

    def prepare_options(self):
        self.options['/scheduler:'] = "bayesianhpc02"
        self.options['/nodegroup:'] = "LinuxNodes"
        self.options['/jobtemplate:'] = "Linux"
        if self.params.hpc_user != '':
            self.options['/user:'] = self.params.hpc_user
        # if self.params.hpc_password != '':
        #    self.options['/password:'] = self.params.hpc_password
        if self.params.measure_perf:
            self.options['/exclusive'] = ' '
        if self.scenario_type == 'tests':
            self.options['/stdout:'] = 'StdOut.txt'
        # else:
        #    print( "Going to redirect stdout, not using parameter." )
        #    self.options['/stdout:'] = 'Test.txt'
        self.options['/stderr:'] = 'StdErr.txt'

    def test_submission(self):

        jobsubmit_bin = 'job submit'
        jobsubmit_command = clg.CommandlineGenerator(jobsubmit_bin, self.options, ['dir'])
        hpc_command_line = jobsubmit_command.Commandline

        p = subprocess.Popen(hpc_command_line, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        p.wait(timeout=30)
        assert p.returncode == 0, hpc_command_line

    def run(self):
    
        self.__class__.sems.acquire()
        def get_num_cores( some_json ):
            num_cores = 1
            if ('parameters' in some_json) and ('Num_Cores' in some_json['parameters']) and some_json['parameters']["Num_Cores"]:
                #num_cores = 2 # some_json['parameters']['Num_Cores']
                num_cores = some_json['parameters']['Num_Cores']
            else:
               print( "Didn't find key 'parameters/Num_Cores' in '{0}'. Using 1.".format( self.scenario_path ) )
               
            return int(num_cores)
    
        input_dir = ".;"
        #input_dir = ""
        if "Geography" in self.config_json["parameters"]:
            input_dir += os.path.join( self.params.input_root.replace( "\\", "/" ).replace( "bayesianfil01", "mnt" ).replace( "IDM", "idm" ).replace( "//", "/" ), self.config_json["parameters"]["Geography"] )
        input_dir = "'" + input_dir + "'" 
        sim_dir = self.sim_root + "/" + self.sim_timestamp   # can't use os.path.join() here because on linux it'll give us the wrong dir-separator...
        if self.suffix is not None:
            job_name = self.config_json["parameters"]["Config_Name"].replace( ' ', '_' ) + "_" + self.suffix + "_(" + self.sim_timestamp + ")"
        else:
            job_name = self.config_json["parameters"]["Config_Name"].replace( ' ', '_' ) + "_(" + self.sim_timestamp + ")"
        job_name = job_name[:79]

        numcores = get_num_cores( self.config_json )

        hpc_resource_option = '/numcores:'
        hpc_resource_count  = str(numcores)
        mpi_core_option = '-n'
        mpi_core_count  = hpc_resource_count  

        """
        if self.params.measure_perf:
            if numcores % self.params.cores_per_node == 0:
                hpc_resource_option = '/numnodes:'
                hpc_resource_count  = str(numcores / self.params.cores_per_node)
                mpi_core_option = '-c'
                mpi_core_count  = str(self.params.cores_per_node)
            elif numcores == self.params.cores_per_socket:
                hpc_resource_option = '/numsockets:'
                hpc_resource_count  = '1'
                mpi_core_option = '-c'
                mpi_core_count  = str(self.params.cores_per_socket)
            # "bail" here, we don't have a multiple of cores per node nor can we fit on a single socket
        """

        #eradication.exe commandline
        eradication_bin = self.config_json['bin_path'].replace( "\\", "/" ).replace( "bayesianfil01", "mnt" ).replace( "IDM", "idm" ).replace( "//", "/" )
        eradication_options = {}
        if "Eradication" in eradication_bin:
            eradication_options = { '--config':'config.json', '--input-path':input_dir, '--progress':' ' }

        # python-script-path is optional parameter.
        if "PSP" in self.config_json:
            eradication_options[ "--python-script-path" ] = self.config_json["PSP"]
        #if params.dll_root is not None and params.use_dlls is True:
        #    eradication_options['--dll-path'] = params.dll_root
        eradication_params = []
        eradication_command = clg.CommandlineGenerator(eradication_bin, eradication_options, eradication_params)

        #mpiexec commandline
        mpi_bin = 'mpiexec'
        mpi_options = {}
        if mpi_core_option is not None:
            mpi_options[mpi_core_option] = mpi_core_count
        mpi_params = [eradication_command.Commandline]
        mpi_command = clg.CommandlineGenerator(mpi_bin, mpi_options, mpi_params)
        
        #job submit commandline
        jobsubmit_bin = 'job submit'
        self.options['/workdir:'] = sim_dir
        self.options['/jobname:'] = job_name
        self.options[hpc_resource_option] = hpc_resource_count
        jobsubmit_params = [mpi_command.Commandline]
        jobsubmit_command = clg.CommandlineGenerator(jobsubmit_bin, self.options, jobsubmit_params)

        #print( 'simulation command line:', eradication_command.Commandline )
        #print( 'mpiexec command line:   ', mpi_command.Commandline )
        #print( 'job submit command line:', jobsubmit_command.Commandline )

        hpc_command_line = jobsubmit_command.Commandline
        if self.scenario_type != 'tests':
            hpc_command_line = hpc_command_line  + " ^> Test.txt"

        job_id = -1
        num_retries = -1

        while job_id == -1:
            num_retries += 1
            #print( "executing hpc_command_line: " + hpc_command_line + "\n")

            #p = subprocess.Popen( hpc_command_line.split(), shell=False, stdout=subprocess.PIPE, stderr=subprocess.PIPE )
            #p = subprocess.Popen( hpc_command_line.split(), shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE )
            p = subprocess.Popen( hpc_command_line, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE )
            [hpc_pipe_stdout, hpc_pipe_stderr] = p.communicate()
            #print "Trying to read hpc response..."
            #print hpc_pipe_stdout
            line = hpc_pipe_stdout

            if p.returncode == 0:
                job_id = int(line.decode().split( ' ' )[-1].strip().rstrip('.'))
                if job_id > 0:
                    print( self.scenario_path + " submitted (as job_id " + str(job_id) + ")\n" )
                else:
                    print( "ERROR: What happened here?  Please send this to Jeff:\n" )
                    print( hpc_pipe_stdout )
                    print( hpc_pipe_stderr )
                    job_id = -1
            else:
                print( "ERROR: job submit of " + self.scenario_path + " failed!" )
                print( hpc_pipe_stdout )
                print( hpc_pipe_stderr )

            if job_id == -1 and num_retries >= 5 and self.params.hide_graphs:
                print( "Job submission failed multiple times for " + self.scenario_path + ".  Aborting this test and logging error." )
                self.report.addErroringTest( self.scenario_path, "", sim_dir, self.scenario_type )
                return

        monitor_cmd_line = "job view /scheduler:" + self.params.hpc_head_node + " " + str(job_id)

        check_status = True
        while check_status:
            #print "executing hpc_command_line: " + monitor_cmd_line
            #print "Checking status of job " + str(job_id)
            #hpc_pipe = os.popen( monitor_cmd_line )
            hpc_pipe = subprocess.Popen( monitor_cmd_line.split(), shell=False, stdout=subprocess.PIPE )
            [hpc_pipe_stdout, hpc_pipe_stderr] = hpc_pipe.communicate()
            lines = hpc_pipe_stdout
            #for line in hpc_pipe.readlines():
            #print lines
            for line in lines.decode().split('\n'):
                res = line.split( ':' )
                #print "DEBUG: " + str(res[0])
                if res[0].strip() == "State":
                    state = res[1].strip()
                    if state == "Failed":
                        self.__class__.completed = self.__class__.completed + 1
                        print( self.scenario_path + " FAILED!" )
                        check_status = False
                        self.report.addErroringTest( self.scenario_path, "", sim_dir, self.scenario_type )
                        #self.finish(sim_dir, False)
                    if state == "Canceled":
                        self.__class__.completed = self.__class__.completed + 1
                        print( "Canceled!" )
                        check_status = False
                        #self.finish(sim_dir, False)
                    elif state == "Completed" or state == "Finished":
                        self.__class__.completed = self.__class__.completed + 1
                        print( str(self.__class__.completed) + " out of " + str(len(ru.reg_threads)) + " completed." )
                        check_status = False

                        if self.scenario_type != 'pymod':
                            look_here = os.path.join( self.params.sim_root, self.sim_timestamp )
                            look_here = os.path.join( look_here, "status.txt")
                            with open( look_here, "r" ) as status_file:
                                for status_line in status_file.readlines():
                                    if status_line.startswith("Done"):
                                        time_split = status_line.split('-')[1].strip().split(':')
                                        self.duration = datetime.timedelta(hours=int(time_split[0]), minutes=int(time_split[1]), seconds=int(time_split[2]))
                                        break

                        self.verify_test()

                    break
            time.sleep(5)
        self.__class__.sems.release()


