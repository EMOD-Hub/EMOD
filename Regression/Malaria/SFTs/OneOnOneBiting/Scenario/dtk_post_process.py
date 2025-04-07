#!/usr/bin/python

import dtk_test.dtk_sft as sft
import dtk_test.dtk_Vector_Support as veds
import pandas as pd
import os

"""
This is a four node four core scenario with vectors migrating in a closed loop: 76>77>78>79>76
With AAA... outbreak in 76 and GAA... outbreak in 78, after some time (at 114), we expect the AAA../GAA.. numbers
to follow the migration with the highest infections being in the outbreak node and then descending
along the path. 

"""


def create_biting_report_file(param_obj,report_name, output_folder, report_malaria, debug=False):
    success = True
    with open(report_name, "w") as outfile:
        outfile.write(f"Test name: {str(param_obj[veds.ConfigKeys.CONFIG_NAME])} \n"
                      f"Run number: {str(param_obj[veds.ConfigKeys.RUN_NUMBER])} \n")
        rsv_path = os.path.join(output_folder, report_malaria)
        malaria_df = pd.read_csv(rsv_path)
        time = 429
        today = malaria_df[malaria_df["Time"] == time]
        # check AAAAAAAAAAAAAAAAAAAAAAAA, we want the numbers to go down the nodes
        gene = "AAAAAAAAAAAAAAAAAAAAAAAA"
        node_id = "NodeID"
        node_76 = today[today[node_id] == 340461476][gene].iloc[0]
        node_77 = today[today[node_id] == 340461477][gene].iloc[0]
        node_78 = today[today[node_id] == 340461478][gene].iloc[0]
        node_79 = today[today[node_id] == 340461479][gene].iloc[0]
        if node_76 > node_77 > node_78 > node_79:
            outfile.write(f"GOOD: We're expecting there to be descending malaria numbers with descending"
                          f" nodes, and there are! \nwe are seeing that: nodes 76, 77, 78, 79  malaria: {node_76}, "
                          f"{node_77},{node_78},{node_79} .\n\n")
        else:
            success = False
            outfile.write(f"BAD: We're expecting there to be descending malaria numbers with descending"
                          f" nodes, \nbut we are not seeing that: nodes 76, 77, 78, 79  malaria: {node_76}, "
                          f"{node_77},{node_78},{node_79} .\n")
        # check GAAAAAAAAAAAAAAAAAAAAAAA the numbers should be going down starting with 78
        gene = "GAAAAAAAAAAAAAAAAAAAAAAA"
        node_76 = today[today[node_id] == 340461476][gene].iloc[0]
        node_77 = today[today[node_id] == 340461477][gene].iloc[0]
        node_78 = today[today[node_id] == 340461478][gene].iloc[0]
        node_79 = today[today[node_id] == 340461479][gene].iloc[0]
        if node_78 > node_79 > node_76 > node_77:
            outfile.write(f"GOOD: We're expecting there to be descending malaria numbers with nodes in order of"
                          f" 78, 79, 76, 77 and we see that!  malaria: {node_78}, "
                          f"{node_79},{node_76},{node_77}\n")
        else:
            success = False
            outfile.write(f"BAD: We're expecting there to be descending malaria numbers with nodes in order of"
                          f" 78, 79, 76, 77  malaria: {node_78}, "
                          f"{node_79},{node_76},{node_77} .\n")

        outfile.write(sft.format_success_msg(success))


def application(output_folder="output",
                config_filename="config.json",
                report_malaria="ReportNodeDemographicsMalariaGenetics.csv",
                report_name=sft.sft_output_filename,
                debug=False):
    if debug:
        print("output_folder: " + output_folder)
        print("config_filename: " + config_filename + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    sft.wait_for_done()
    param_obj = veds.load_emod_parameters(config_filename)
    create_biting_report_file(param_obj=param_obj, report_name=report_name, output_folder=output_folder,
                              report_malaria=report_malaria, debug=debug)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-r', '--reportname', default=sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', action='store_true', help="Turns on debugging")
    args = parser.parse_args()

    application(output_folder=args.output,
                config_filename=args.config,
                report_name=args.reportname, debug=args.debug)
