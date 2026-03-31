from __future__ import print_function
import os
import sys
if sys.version_info[0] < 3:
    import ConfigParser as configparser
else:
    import configparser
import pdb

class RuntimeParameters:
    def __init__(self, args):
        print( "os = " + os.name )
        self.args = args
        if not os.path.exists(args.config) :
            print("Couldn't find configuration-file \"" + args.config + "\"")
            sys.exit()

        user = None
        username_key = None
        if os.name == "posix":
            self.os_type = "POSIX"
            username_key = "USER"
            user = ""
        else:
            self.os_type = "WINDOWS"
            username_key = "USERNAME"
            user = os.environ["USERDOMAIN"] + '\\'
            
        user += os.environ[username_key]
        # This silly '2' thing is just a wacky change I made that worked. Leaving it as config was broken
        self.config2 = configparser.ConfigParser({'password':'', 'username':user})
        self.config2.read(args.config)
        self.config2.set('ENVIRONMENT', 'username', os.environ[username_key])
        self._use_user_input_root = False
        self.PSP = None
        self.display()

    def display(self):
        print( "[arg] Suite:                      ", self.suite )
        print( "[arg] Executable path:            ", self.executable_path )
        print( "[arg] Run in perf mode:           ", self.measure_perf )
        print( "[arg] Hide graphs on mismatch:    ", self.hide_graphs )
        print( "[arg] Debug:                      ", self.debug )
        print( "[arg] Quickstart:                 ", self.quick_start )
        print( "[arg] Use DLLs:                   ", self.use_dlls )
        print( "[arg] SCons:                      ", self.scons )
        print( "[arg] Print error msg to screen:  ", self.print_error )
        print( "[arg] Job name suffix:            ", self.label )
        print( "[arg] Config file:                ", self.regression_config )
        print( "[arg] Compare all outputs:        ", self.all_outputs )
        print( "[arg] Disable schema test:        ", self.disable_schema_test )
        print( "[arg] Component tests:            ", self.component_tests )
        print( "[arg] Component tests show output:", self.component_tests_show_output )
        print( "[arg] Skip emodule test:          ", self.sec )
        print( "[arg] Config constraints:         ", self.constraints_dict )
        print( "[arg] Run sims locally:           ", self.local_execution )
        print( "[arg] Run Linux binary:           ", self.linux )
        # print( "", self.config2 )
        print( "[cfg] HPC head node/group:         {0} / {1}".format(self.hpc_head_node, self.hpc_node_group) )
        print( "[cfg] HPC user/password:           {0} / {1}".format(self.hpc_user if self.hpc_user else 'empty', self.hpc_password if self.hpc_password else 'empty') )
        print( "[cfg] Cores per node/socket:       {0} / {1}".format(self.cores_per_node, self.cores_per_socket) )

        print( "[cfg] Bin root:                   ", self.bin_root )
        print( "[cfg] DLL root:                   ", self.dll_root )
        print( "[cfg] Input root:                 ", self.input_root )
        print( "[cfg] Shared input:               ", self.shared_input )
        print( "[cfg] User input:                 ", self.user_input )
        print( "[cfg] Use user input:             ", self.use_user_input_root )
        print( "[cfg] Sim root:                   ", self.sim_root )

        print( "[cfg] Local bin root:             ", self.local_bin_root )
        print( "[cfg] Local input path:           ", self.input_path )
        print( "[cfg] Local sim root:             ", self.local_sim_root )
        print( "[cfg] DLL path:                   ", self.dll_path )
        print( "[cfg] Source root:                ", self.src_root )
        return
    
    @property
    def suite(self):
        return self.args.suite
        
    @property
    def executable_path(self):
        path = self.args.exe_path
        if not path:
            if self.scons:
                if self.debug:
                    path = "../build/x64/Debug/Eradication/Eradication"
                else:
                    path = "../build/x64/Release/Eradication/Eradication"

                if os.name == "nt":
                    path += ".exe"

            else:
                if self.debug:
                    path = "../Eradication/x64/Debug/Eradication.exe"
                else:
                    path = "../Eradication/x64/Release/Eradication.exe"

        return path
    
    @property
    def measure_perf(self):
        return self.args.perf
    
    @property
    def hide_graphs(self):
        return self.args.hidegraphs
    
    @property
    def debug(self):
        return self.args.debug
    
    @property
    def quick_start(self):
        return self.args.quick_start
    
    @property
    def use_dlls(self):
        if self.args.dll_path is not None:
            return True
        else:
            return self.args.use_dlls
    
    @property
    def scons(self):
        return self.args.scons

    @property
    def print_error(self):
        return self.args.print_error
    
    @property
    def label(self):
        return self.args.label

    @property
    def regression_config(self):
        return self.args.config
        
    @property
    def config(self):
        return self.config2

    @property
    def hpc_head_node(self):
        return self.config2.get('HPC', 'head_node')
        
    @property
    def hpc_node_group(self):
        return self.config2.get('HPC', 'node_group')
        
    @property
    def hpc_user(self):
        try:
            return self.config2.get('HPC', 'username')
        except configparser.NoOptionError as ex:
            return ""

    @property
    def hpc_password(self):
        try:
            return self.config2.get('HPC', 'password')
        except configparser.NoOptionError as ex:
            return ""
        
    @property
    def cores_per_socket(self):
        return self.config2.getint('HPC', 'num_cores_per_socket')
        
    @property
    def cores_per_node(self):
        return self.config2.getint('HPC', 'num_cores_per_node')

    @property
    def local_sim_root(self):
        return self.config2.get(self.os_type, 'local_sim_root')
        
    @property
    def input_path(self):
        return self.config2.get(self.os_type, 'home_input')
        
    @property
    def local_bin_root(self):
        return self.config2.get(self.os_type, 'local_bin_root')

    @property
    def sim_root(self):
        if self.local_execution or os.name=="posix":
            return self.config2.get(self.os_type, 'local_sim_root')
        else:
            return self.config2.get('ENVIRONMENT', 'sim_root')

    @property
    def shared_input(self):
        if self.local_execution or os.name=="posix":
            return self.config2.get(self.os_type, 'local_input_root')
        else:
            return self.config2.get('ENVIRONMENT', 'input_root')
        
    @property
    def bin_root(self):
        if self.local_execution or os.name=="posix":
            return self.config2.get(self.os_type, 'local_bin_root')
        else:
            return self.config2.get('ENVIRONMENT', 'bin_root')

    @property
    def user_input(self):
        if self.local_execution or os.name=="posix":
            return self.config2.get(self.os_type, 'home_input')
        else:
            return self.config2.get('ENVIRONMENT', 'home_input')
        
    """
    @property
    def py_input(self):
        if self.local_execution or os.name=="posix":
            return self.config2.get(self.os_type, 'py_input')
        else:
            return self.config2.get('ENVIRONMENT', 'py_input')
    """
        
    @property
    def use_user_input_root(self):
        return self._use_user_input_root
        
    @use_user_input_root.setter
    def use_user_input_root(self, value):
        self._use_user_input_root = value
        
    @property
    def input_root(self):
        if not self.use_user_input_root:
            return self.shared_input
        else:
            return self.user_input
        
    @property
    def dll_path(self):
        return self.args.dll_path

    @property
    def dll_root(self):
        try:
            if self.local_execution:
                dll_root = self.config2.get(self.os_type, 'local_dll_root')
            else:
                dll_root = self.config2.get('ENVIRONMENT', 'dll_root')
        except Exception as ex:
            if self.local_execution:
                dll_root = self.config2.get(self.os_type, 'local_bin_root')
            else:
                dll_root = self.config2.get('ENVIRONMENT', 'bin_root')
        return dll_root

    @property
    def src_root(self):
        try:
            src_root = self.config2.get('LOCAL-ENVIRONMENT', 'src_root')
        except Exception as ex:
            src_root = "./.."
        return src_root

    @property
    def all_outputs(self):
        return self.args.all_outputs
        
    @property
    def disable_schema_test(self):
        return self.args.disable_schema_test
        
    @property
    def component_tests(self):
        return self.args.component_tests

    @property
    def component_tests_show_output(self):
        return self.args.component_tests_show_output

    @property
    def sec(self):
        return self.args.skip_emodule_check

    @property
    def constraints_dict(self):
        constraints_dict = {}
        if self.args.config_constraints and len(self.args.config_constraints) > 0:
            constraints_list = self.args.config_constraints.split(",")
            for raw_nvp in constraints_list:
                nvp = raw_nvp.split(":")
                constraints_dict[ nvp[0] ] = nvp[1]
        return constraints_dict
    
    @property
    def local_execution(self):
        return self.args.local

    @property
    def linux(self):
        return self.args.linux

