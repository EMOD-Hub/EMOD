import shutil
import json
import os.path as path
import dtk_test.dtk_serialization_support as d_ss
import dtk_test.dtk_sft as sft

def has_acceptable_infection(human, remaining_simulation):
    if "infections" not in human:
        raise ValueError("this person doesn't seem to have infections.")
    that_will_work = False
    infection_suid = None
    for infection in human["infections"]:
        remaining_duration = infection["total_duration"] - infection["duration"]
        if remaining_duration > remaining_simulation:
            that_will_work = True
            infection_suid = infection['suid']['id']
            break
    return infection_suid

def application(config_filename="config.json", debug=False):
    with open(config_filename) as infile:
        config_json = json.load(infile)
        config_params = config_json[d_ss.ConfigKeys.PARAMETERS_KEY]
    config_object = d_ss.load_config_file(config_filename=config_filename, debug=debug)
    sft.start_report_file(output_filename=sft.sft_output_filename,
                          config_name=config_object[d_ss.ConfigKeys.CONFIGNAME_KEY])
    d_ss.start_serialization_test()
    expected_dtk_filepath = config_params[d_ss.ConfigKeys.Serialization.POPULATION_PATH]
    expected_dtk_filename = config_params[d_ss.ConfigKeys.Serialization.POPULATION_FILENAMES_KEY][0]
    expected_dtk_fullpath = path.join(expected_dtk_filepath, expected_dtk_filename)

    dtk_file = d_ss.DtkFile(expected_dtk_fullpath, file_suffix="old")
    found_people={}
    ignored_people=[]
    testable_infections = 5
    remaining_sim_duration = config_object[d_ss.ConfigKeys.Serialization.TIMESTEPS_KEY][0]
    while len(found_people) < testable_infections:
        maybe_person = dtk_file.find_human_from_node(node_index=0, min_infections=1, ignore_suids=ignored_people)
        human_suid = maybe_person["suid"]["id"]
        if debug:
            print(f"Trying human {human_suid}.\n")
        acceptable_infection = has_acceptable_infection(maybe_person, remaining_sim_duration)
        if acceptable_infection:
            dtk_file.write_human_to_disk(node_index=0, suid=human_suid)
            found_people[human_suid] = acceptable_infection
        ignored_people.append(human_suid)
    dtk_file.write_json_file(found_people, "DEBUG_found_infected_persons.json")
    dtk_file.write_node_to_disk(node_index=0)
    dtk_file.write_simulation_to_disk()

if __name__=="__main__":
    import argparse
    p = argparse.ArgumentParser()
    p.add_argument('-c', '--configname', default="config.json", help="config filename (config.json")
    args = p.parse_args()

    application(config_filename=args.configname,
                debug=True)
