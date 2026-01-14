import dtk_test.dtk_sft as sft
import os.path as path
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

    dtk_filename = path.join(output_folder, 'state-00070.dtk')
    dtk_file = d_ss.DtkFile(dtk_filename)
    dtk_file.write_human_to_disk(node_index=0, suid=10)
    dtk_file.write_human_to_disk(node_index=0, suid=100)
    dtk_file.write_human_to_disk(node_index=0, suid=1000)
    dtk_file.write_node_to_disk(node_index=0)
    dtk_file.write_simulation_to_disk()

    #TODO: make sure each individual is Simulation_Duration days older
    human_1_first = d_ss.SerializedHuman.from_file('human-10-old.json')
    human_1_second = d_ss.SerializedHuman.from_file('human-10-new.json')

    human_100_first = d_ss.SerializedHuman.from_file('human-100-old.json')
    human_100_second = d_ss.SerializedHuman.from_file('human-100-new.json')

    human_1000_first = d_ss.SerializedHuman.from_file('human-1000-old.json')
    human_1000_second = d_ss.SerializedHuman.from_file('human-1000-new.json')

    messages = []
    expected_age_delta = config_object[d_ss.ConfigKeys.SIMULATION_DURATION_KEY]
    human_1_messages = human_1_first.compare_to_older(human_1_second, expected_age_delta)
    human_100_messages = human_100_first.compare_to_older(human_100_second, expected_age_delta)
    human_1000_messages = human_1000_first.compare_to_older(human_1000_second, expected_age_delta)

    messages += human_1_messages if human_1_messages else ["GOOD: Human 1 checks out"]
    messages += human_100_messages if human_100_messages else ["GOOD: Human 100 checks out"]
    messages += human_1000_messages if human_1000_messages else ["GOOD: Human 1000 checks out"]

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
