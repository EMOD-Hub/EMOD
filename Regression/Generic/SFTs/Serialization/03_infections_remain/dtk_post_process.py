import dtk_test.dtk_sft as sft
import os.path as path
import json
import dtk_test.dtk_serialization_support as d_ss

def application(output_folder="output", config_filename="config.json",
                report_name=sft.sft_output_filename, debug=False):
    if debug:
        print("output_folder: {0}".format(output_folder))
        print("config_filename: {0}".format(config_filename))
        print("report_name: {0}".format(report_name))
        print("debug: {0}".format(debug))

    config_object = d_ss.load_config_file(config_filename=config_filename, debug=debug)
    sft.start_report_file(output_filename=report_name,
                          config_name=config_object[d_ss.ConfigKeys.CONFIGNAME_KEY],
                          already_started=True)
    sft.wait_for_done()

    dtk_filename = path.join(output_folder, 'state-00005.dtk')
    dtk_file = d_ss.DtkFile(dtk_filename)
    infected_people_to_check = []
    with open("DEBUG_found_infected_persons.json") as infile:
        infected_people_to_check = json.load(infile)
        if debug:
            print(f"Infected_people_to_check: {infected_people_to_check}")
    infected_people_first={}
    infected_people_second={}
    for suid in infected_people_to_check:
        suid_int = int(suid)
        print("Trying suid: {0}".format(suid_int))
        infected_people_first[suid] = d_ss.SerializedHuman.from_file(f"human-{suid_int}-old.json")
        dtk_file.write_human_to_disk(node_index=0, suid=suid_int, debug=debug)
        infected_people_second[suid] = d_ss.SerializedHuman.from_file(f"human-{suid_int}-new.json")

    # Make sure each individual is Simulation_Duration days older
    messages = []
    expected_age_delta = config_object[d_ss.ConfigKeys.Serialization.TIMESTEPS_KEY][0]
    for person in infected_people_to_check:
        infection_suid = infected_people_to_check[person]
        if debug:
            # print(f"X is: {x}")
            print(f"person is: {person}")
            print(f"infection is: {infection_suid}")
            print(f"infected_people_first[person] = {infected_people_first[person]}")
            print(f"infected_people_second[person] = {infected_people_second[person]}")
        aging_messages = infected_people_first[person].compare_to_older(infected_people_second[person], expected_age_delta)
        messages += aging_messages if aging_messages else [f"GOOD: human {person} checks out"]
        older_infection = infected_people_first[person].get_infection_by_suid(infection_suid)
        newer_infection = infected_people_second[person].get_infection_by_suid(infection_suid)
        messages += newer_infection.compare_to_older(older_infection, expected_age_delta)
        messages += newer_infection.validate_own_timers()

    dtk_file.write_node_to_disk(node_index=0)
    dtk_file.write_simulation_to_disk()
    
    # Make sure each infection is Simulation_Duration days older


    #TODO: compare the simulation objects

    #TODO: compare the node objects
    success_message_formatter = sft.format_success_msg
    d_ss.generate_report_file(config_object, report_name, output_folder=output_folder,
                              messages=messages, success_formatter=success_message_formatter,
                              debug=debug)
    d_ss.clean_serialization_test(debug)

if __name__=="__main__":
    import argparse
    p = argparse.ArgumentParser()
    p.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    p.add_argument('-c', '--configname', default="config.json", help="config filename (config.json")
    p.add_argument('-r', '--reportname', default=sft.sft_output_filename, help="report filename ({0})".format(sft.sft_output_filename))
    args = p.parse_args()

    application(output_folder=args.output, report_name=args.reportname, config_filename=args.configname,
                debug=True)
