import json
import os
import os.path as path
import glob
import dtk_test.dtk_FileTools as dft


class ConfigKeys:
    PARAMETERS_KEY = "parameters"
    CONFIGNAME_KEY = "Config_Name"
    DEMOGRAPHICS_FILENAMES_KEY = "Demographics_Filenames"
    SIMULATION_DURATION_KEY = "Simulation_Duration"
    START_TIME_KEY = "Start_Time"
    NUM_CORES_KEY = "Num_Cores"

    class Serialization:
        TIMESTEPS_KEY = "Serialization_Time_Steps"
        POPULATION_FILENAMES_KEY = "Serialized_Population_Filenames"
        POPULATION_PATH = "Serialized_Population_Path"


class SneakyStuff:
    CHANNELS_KEY = "Channels"
    FAKE_CHARTNAME = "FakeChart.json"
    DEBUG_FILELIST = "DEBUG_filelist.txt"


def load_config_file(config_filename="config.json", debug=False):

    with open(config_filename) as infile:
        params = json.load(infile)[ConfigKeys.PARAMETERS_KEY]

    config_object = {}
    config_params = [ConfigKeys.Serialization.TIMESTEPS_KEY,
                     ConfigKeys.Serialization.POPULATION_FILENAMES_KEY,
                     ConfigKeys.Serialization.POPULATION_PATH,
                     ConfigKeys.CONFIGNAME_KEY,
                     ConfigKeys.DEMOGRAPHICS_FILENAMES_KEY,
                     ConfigKeys.SIMULATION_DURATION_KEY,
                     ConfigKeys.START_TIME_KEY]
    for key in config_params:
        if key in params:
            config_object[key] = params[key]
    num_cores_tmp = 1 # Num cores is not required by schema. Assume 1
    if ConfigKeys.NUM_CORES_KEY in params:
        config_object[ConfigKeys.NUM_CORES_KEY] = params[ConfigKeys.NUM_CORES_KEY]
    else:
        config_object[ConfigKeys.NUM_CORES_KEY] = num_cores_tmp

    if debug:
        with open("DEBUG_config_object.json","w") as outfile:
            json.dump(config_object, outfile, indent=4)
    return config_object


def check_nodes(config_object, nodes_object):
    demographics_filename = config_object[ConfigKeys.DEMOGRAPHICS_FILENAMES_KEY][0]
    with open(demographics_filename) as infile:
        demo_json = json.load(infile)

    messages = []
    demo_nodes = demo_json["Nodes"]
    success = False
    if len(demo_nodes) == len (nodes_object):
        success = True
        messages.append("GOOD: Nubmer of nodes is correct")
    else:
        messages.append("BAD: Expected {0} nodes, got {1}".format(len(demo_nodes), len(nodes_object)))
    for n in demo_nodes:
        n_id = n['NodeID']
        foundIt = False
        for no in nodes_object:
            if no['externalId'] == n_id:
                foundIt = True
                break
        if not foundIt:
            success = False
            messages.append("BAD: Couldn't find node with ID {0} in nodes object from dtk file".format(n_id))


def generate_report_file(config_object, report_name, output_folder="output",
                         messages=[], success_formatter=None,
                         debug=False):
    with open(report_name, 'a') as outfile:
        success = False
        current_directory = os.getcwd()
        output_fullpath = path.join(current_directory, output_folder)
        glob_pathname = path.join(output_fullpath, "*.dtk")
        dtk_filelist = glob.glob(glob_pathname, recursive=False)
        if ConfigKeys.Serialization.TIMESTEPS_KEY in config_object:
            file_count_message_template = "{0}: Expected count of {1} files, got these {2}\n"
            expected_count = len(config_object[ConfigKeys.Serialization.TIMESTEPS_KEY]) * config_object[ConfigKeys.NUM_CORES_KEY]
            actual_count = len(dtk_filelist)
            if actual_count == expected_count:
                success = True
                outfile.write(file_count_message_template.format("GOOD",expected_count, dtk_filelist))
            else:
                outfile.write(file_count_message_template.format("BAD", expected_count, dtk_filelist))

        for m in messages:
            outfile.write(m+ "\n")
            if m.startswith("BAD"):
                success = False
        if success_formatter:
            outfile.write(success_formatter(success))
        else:
            outfile.write("Did it pass? {0}\n".format(success))


def start_serialization_test(debug=False):
    debug_filepath = SneakyStuff.DEBUG_FILELIST
    if os.path.isfile(debug_filepath):
        if debug:
            print("Found file at {0}, deleting.\n".format(debug_filepath))
        os.unlink(debug_filepath)
    with open(debug_filepath, 'w') as outfile:
        outfile.write(debug_filepath + '\n')


def clean_serialization_test(debug=False):
    flist = []
    with open(SneakyStuff.DEBUG_FILELIST) as infile:
        flist = infile.readlines()
    for f in flist:
        if not debug:
            if path.isfile(f.rstrip()):
                os.unlink(f.rstrip())
            else:
                print(f"Couldn't find {f}")
        else:
            print (f"DEBUG: would delete {f.rstrip()}")

class DtkFile:
    def __init__(self, dtk_filename, file_suffix="new"):
        self.filename_suffix = file_suffix
        self.file_extension = "json"
        self.f = dft.read(dtk_filename)
        self.ns = self.f.nodes

    def write_json_file(self, json_content, json_filename):
        with open(json_filename, 'w') as outfile:
            json.dump(json_content, outfile, indent=4)
        with open(SneakyStuff.DEBUG_FILELIST,'a') as outfile:
            outfile.write(json_filename + '\n')

    def write_simulation_to_disk(self, sim_filename="simulation"):
        self.s = self.f.simulation
        json_filename = "{0}-{1}.{2}".format(sim_filename,
                                                 self.filename_suffix,
                                                 self.file_extension)
        self.write_json_file(self.s, json_filename)

    def write_node_to_disk(self, node_index=0, preserve_people=False,
                           node_filename="node"):
        n = self.ns[node_index]
        if not preserve_people:
            n.individualHumans = []
            n.home_individual_ids = []
        json_filename = "{0}-{1}.{2}".format(node_filename,
                                             self.filename_suffix,
                                             self.file_extension)
        self.write_json_file(n.get_json(), json_filename)

    def find_human_from_node(self, node_index=0, suid=None,
                             min_infections=None,
                             min_interventions=None,
                             desired_intervention=None,
                             ignore_suids=[],
                             debug=False):
        if desired_intervention and not min_interventions:
            raise AssertionError("Should not have a desired intervention with no min interventions. Set min_interventions to 1")
        n = self.ns[node_index]
        found_human = None
        all_humans = n.individualHumans
        if suid:
            if debug:
                print("Looking for suid: {0}".format(suid))
            for h in all_humans:
               if h["suid"]["id"] == suid:
                    found_human = h
                    break
        else:
            while not found_human:
                for h in all_humans:
                    maybe_human = h
                    if min_infections:
                        if len(maybe_human["infections"]) < min_infections:
                            maybe_human = None
                            continue
                    if min_interventions:
                        interventions = maybe_human["interventions"]["interventions"]
                        if len(interventions) < min_interventions:
                            maybe_human = None
                            continue
                        elif desired_intervention:
                            found_one = False
                            # Not sure yet, pass
                            pass
                    if maybe_human and maybe_human["suid"]["id"] not in ignore_suids:
                        found_human = maybe_human
        if found_human:
            return found_human
        else:
            return None

    def write_human_to_disk(self, node_index=0, suid=None,
                            min_infections=None,
                            min_interventions=None,
                            desired_intervention=None,
                            ignore_suids=[],
                            debug=False):
        found_human = self.find_human_from_node(node_index=node_index,
                                                suid=suid,
                                                min_infections=min_infections,
                                                min_interventions=min_interventions,
                                                desired_intervention=desired_intervention,
                                                ignore_suids=ignore_suids,
                                                debug=debug)
        if debug:
            print(f"Tried to find human with suid {suid}.")
        if found_human:
            human_filename = "human-{0}".format(found_human["suid"]["id"])
            json_filename = "{0}-{1}.{2}".format(human_filename,
                                                 self.filename_suffix,
                                                 self.file_extension)
            print("Json filename is {0}".format(json_filename))
            self.write_json_file(found_human, json_filename)
            return found_human["suid"]["id"]
        else:
            raise ValueError("Couldn't find an acceptable human in node index {0}".format(node_index))


class SerializedHuman:
    def __init__(self, json_human):
        self.suid = json_human["suid"]["id"]
        self.age = json_human["m_age"]
        self.gender = json_human["m_gender"]
        self.monte_carlo_weight = json_human["m_mc_weight"]
        self.infections = json_human["infections"]

    @classmethod
    def from_file(cls, json_human_filename):
        with open(json_human_filename) as infile:
            json_human = json.load(infile)
        return cls(json_human)

    def compare_to_older(self, older_human, expected_age_delta):
        messages = []
        if self.gender != older_human.gender:
            messages.append("BAD: human suid={0} changed gender from {1} to {2}".format(
                self.suid, self.gender, older_human.gender
            ))
        actual_age_delta = older_human.age - self.age
        if actual_age_delta != expected_age_delta:
            messages.append("BAD: human suid={0} expected age delta of {1} but got {2}".format(
                self.suid, expected_age_delta, actual_age_delta
            ))
        if self.monte_carlo_weight != older_human.monte_carlo_weight:
            messages.append("BAD: human suid={0} should not change mc_weight. Went from {1} to {2}".format(
                self.suid, older_human.monte_carlo_weight, self.monte_carlo_weight
            ))
        return messages

    def get_infection_by_suid(self, suid):
        for infection in self.infections:
            if infection['suid']['id'] == suid:
                return SerializedInfection(infection, self.suid)
        return None

class SerializedInfection:
    def __init__(self, json_infection, human_suid):
        self.suid = json_infection['suid']['id']
        self.human_suid = human_suid
        self.name = f"Infection {self.suid} from human {self.human_suid}"
        # timers
        self.duration_sofar = json_infection['duration']
        self.duration_total = json_infection['total_duration']
        self.duration_incubation = json_infection['incubation_timer']
        self.duration_infectiousness = json_infection['infectious_timer']

        self.infectiousness = json_infection['infectiousness']
        infection_strain = json_infection['infection_strain']
        self.strain_antigen = infection_strain['antigenID']
        self.strain_genetic = infection_strain['geneticID']

    def compare_to_older(self, older_infection, time_since_older):
        messages = []
        expected_duration_sofar = older_infection.duration_sofar + time_since_older
        if self.strain_antigen != older_infection.strain_antigen:
            messages.append(f"BAD: {self.name} used to have antigen {older_infection.strain_antigen} and now has {self.strain_antigen}")
        if self.strain_genetic != older_infection.strain_genetic:
            messages.append(f"BAD: {self.name} used to have geneID {older_infection.strain_genetic} and now has {self.strain_genetic}")
        if self.duration_sofar != expected_duration_sofar:
            messages.append("BAD: Infection {0} from human {1} should be {2} days old, is {3}".format(
                self.suid,
                self.human_suid,
                expected_duration_sofar,
                self.duration_sofar
            ))
        return messages

    def validate_own_timers(self):
        messages = []
        if self.duration_sofar > self.duration_total:
            messages.append(f"BAD: {self.name} is {self.duration_sofar} old, longer than final age of {self.duration_total}")
        return messages
