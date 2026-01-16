import json
import os.path as path
import dtk_test.dtk_serialization_support as d_ss
import dtk_test.dtk_sft as sft


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
    dtk_file.write_human_to_disk(node_index=0, suid=10)
    dtk_file.write_human_to_disk(node_index=0, suid=100)
    dtk_file.write_human_to_disk(node_index=0, suid=1000)
    dtk_file.write_node_to_disk(node_index=0)
    dtk_file.write_simulation_to_disk()

if __name__=="__main__":
    import argparse
    p = argparse.ArgumentParser()
    p.add_argument('-c', '--configname', default="config.json", help="config filename (config.json")
    args = p.parse_args()

    application(config_filename=args.configname,
                debug=True)
