import json
import os
import xml.dom.minidom

class SimpleReport:
    def __init__(self, params):
        print( "Writing a human-readable report" )
        self.params = params

        
class Report:
    def __init__(self, params, version_string):
        self.num_tests = 0
        self.num_failures = 0
        self.num_errors = 0
        
        self.params = params
    
        self.doc = xml.dom.minidom.Document()
        
        self.suite_el = self.doc.createElement("testsuite")
        self.doc.appendChild(self.suite_el)
        
        prop_el = self.doc.createElement("property")
        prop_el.setAttribute("name", "Version string")
        prop_el.setAttribute("value", version_string)

        headnode_el = self.doc.createElement("property")
        headnode_el.setAttribute("name", "HPC Headnode")
        headnode_el.setAttribute("value", params.hpc_head_node)

        nodegroup_el = self.doc.createElement("property")
        nodegroup_el.setAttribute("name", "HPC Nodegroup")
        nodegroup_el.setAttribute("value", params.hpc_node_group)

        prop_els = self.doc.createElement("properties")
        prop_els.appendChild(prop_el)
        prop_els.appendChild(headnode_el)
        prop_els.appendChild(nodegroup_el)

        self.suite_el.appendChild(prop_els)
        self.schema = "skipped"

        self._test_failures = set()
        self._science_failures = set()

    def addPassingTest(self, name, time, insetchart_path):
        testcase_el = self.doc.createElement("testcase")
        testcase_el.setAttribute("name", name)
        if time:
            testcase_el.setAttribute("time", str(time.total_seconds()))
        testcase_el.setAttribute("message", name + " PASSED, result data found at " + insetchart_path)
        
        self.suite_el.appendChild(testcase_el)
        
        self.num_tests += 1
    
    def addFailingTest(self, scenario_path, failure_txt, insetchart_path, scenario_type, serialization_test_type=None):
        testcase_el = self.doc.createElement("testcase")
        testcase_el.setAttribute("name", scenario_path)
        testcase_el.setAttribute("time", "-1")
        
        msg = scenario_path
        if serialization_test_type != None:
            msg = msg + " " + serialization_test_type
        msg = msg + " failed validation!  Result data can be found at " + insetchart_path

        failure_el = self.doc.createElement("failure")
        failure_el.setAttribute("type", "Validation failure")
        failure_el.setAttribute("message", msg)
        
        failure_txt_el = self.doc.createTextNode(failure_txt)
        failure_el.appendChild(failure_txt_el)
        
        sysout_el = self.doc.createElement("system-out")
        sysout_txt = self.doc.createTextNode("n/a") # could fill this out more in the future, but not right now...
        sysout_el.appendChild(sysout_txt)

        syserr_el = self.doc.createElement("system-err")
        syserr_txt = self.doc.createTextNode("n/a") # could fill this out more in the future, but not right now...
        syserr_el.appendChild(syserr_txt)

        testcase_el.appendChild(failure_el)
        testcase_el.appendChild(sysout_el)
        testcase_el.appendChild(syserr_el)

        self.suite_el.appendChild(testcase_el)
        
        self.num_tests += 1
        self.num_failures += 1

        if scenario_type == 'tests':
            self._test_failures.add(scenario_path)
        elif scenario_type == 'science':
            self._science_failures.add(scenario_path)

    def addErroringTest(self, scenario_path, error_txt, simulation_path, scenario_type):
        testcase_el = self.doc.createElement("testcase")
        testcase_el.setAttribute("name", scenario_path)
        testcase_el.setAttribute("time", "-1")
        
        error_el = self.doc.createElement("error")
        error_el.setAttribute("type", "FUNCTIONAL FAILURE")
        error_el.setAttribute("message", scenario_path + " EXITED UNEXPECTEDLY!  Simulation output can be found at " + simulation_path)
        
        error_txt_el = self.doc.createTextNode(error_txt)
        error_el.appendChild(error_txt_el)
        
        sysout_el = self.doc.createElement("system-out")
        sysout_txt = self.doc.createTextNode("n/a") # could fill this out more in the future, but not right now...
        sysout_el.appendChild(sysout_txt)

        stderr_filename = simulation_path + "/StdErr.txt"
        stderr_txt = "n/a"
        if( os.path.exists( stderr_filename ) ):
            with open( stderr_filename, "r" ) as stderr_file:
                stderr_txt = "\n"
                stderr_txt += stderr_file.read()

        syserr_el = self.doc.createElement("system-err")
        syserr_txt = self.doc.createTextNode(stderr_txt)
        syserr_el.appendChild(syserr_txt)

        testcase_el.appendChild(error_el)
        testcase_el.appendChild(sysout_el)
        testcase_el.appendChild(syserr_el)

        self.suite_el.appendChild(testcase_el)
        
        self.num_tests += 1
        self.num_errors += 1

        if scenario_type == 'tests':
            self._test_failures.add(scenario_path)
        elif scenario_type == 'science':
            self._science_failures.add(scenario_path)

        if self.params.print_error:
            print(stderr_txt)

    def write(self, filename, time):
        self.suite_el.setAttribute("name", self.params.suite + " regression suite")
        self.suite_el.setAttribute("tests", str(self.num_tests))
        self.suite_el.setAttribute("failures", str(self.num_failures))
        self.suite_el.setAttribute("errors", str(self.num_errors))
        self.suite_el.setAttribute("time", str(time.total_seconds()))
        
        report_path = os.path.dirname(filename)
        if not os.path.exists(report_path):
            os.makedirs(report_path)

        with open( filename, "a" ) as report_file:
            report_file.write(self.doc.toprettyxml())

        def failure_file_name(filename, key):
            prefix = filename.replace("report_", "failed_{0}_".format(key))
            suffix = prefix.replace("xml", "json")

            return suffix

        def write_failed_file(failures, key):
            with open( failure_file_name(filename, key), 'w') as failure_file:
                failures_json = {key: [{"path": s} for s in sorted(failures)]}
                json.dump(failures_json, failure_file, indent=2, separators=(',',':'))

        if len(self._test_failures) > 0:
            write_failed_file(self._test_failures, 'tests')

        if len(self._science_failures) > 0:
            write_failed_file(self._science_failures, 'science')

        return

    @property
    def Summary(self):
        return { "tests" : self.num_tests, "passed" : (self.num_tests - self.num_errors - self.num_failures), "failed" : self.num_failures, "errors" : self.num_errors, "schema": self.schema }


