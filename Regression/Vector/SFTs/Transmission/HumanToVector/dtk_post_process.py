#!/usr/bin/python

import json
import os.path as path
import math
import pandas as pd
import numpy as np
import dtk_test.dtk_sft as sft
import dtk_test.dtk_sft_svetlana as svet

KEY_START_TIMESTEP = "curr_timestep"
KEY_TOTAL_TIMESTEPS = "total_timesteps"
KEY_AQUIRE_MODIFIER = "Acquire_Modifier"


def load_emod_parameters(config_filename="config.json"):
    """
        Loading relevant parameters from the config file
    Args:
        config_filename: Config filename

    Returns: a dictionary with relevant parameters

    """
    cdj = None
    with open(config_filename) as infile:
        cdj = json.load(infile)["parameters"]
    param_obj = {KEY_TOTAL_TIMESTEPS: cdj["Simulation_Duration"],
                 KEY_AQUIRE_MODIFIER: cdj["Vector_Species_Params"][0]["Acquire_Modifier"]}
    return param_obj


def parse_stdout_file(stdout_filename="test.txt", debug=False):
    """
        Parsing log fife
    Args:
        stdout_filename: Name of file
        debug: if flag is set to true, the log lines of interest and final data are written out as a text and csv files

    Returns: dataframe of daily feed counts and daily vector infection counts

    """
    with open(stdout_filename, "r") as log_file:
        filtered_lines = []
        infections_today = 0
        infections_per_day = []
        feedings_today = 0
        feedings_per_day = []
        for line in log_file:
            if "Update(): Time:" in line:
                filtered_lines.append(line)
                infections_per_day.append(infections_today)
                feedings_per_day.append(feedings_today)
                infections_today = 0
                feedings_today = 0
            elif "acquired infection from HUMAN" in line:
                filtered_lines.append(line)
                infections_today += 1
            elif "The mosquito successfully fed" in line:
                if "STATE_ADULT" in line:  # only looking at un-infected vectors
                    filtered_lines.append(line)
                    feedings_today += 1

    infections_and_feeds_df = pd.DataFrame.from_dict({"Infections": infections_per_day, "Feeds": feedings_per_day})
    if 0 in feedings_per_day:
        infections_and_feeds_df.to_csv("Infections_and_feeds.csv")
        raise ValueError("ERROR: There are days with 0 feeds recorded. Please check the logs. \n")

    infections_and_feeds_df["Proportion"] = infections_and_feeds_df["Infections"] / infections_and_feeds_df["Feeds"]

    if debug:
        infections_and_feeds_df.to_csv("Infections_and_feeds.csv")
        with open("DEBUG_filtered_lines.txt", "w") as outfile:
            outfile.writelines(filtered_lines)

    return infections_and_feeds_df


def parse_json_report(output_folder="output", insetchart_name="InsetChart.json", debug=False):
    """
       Parsing InsetChart.json
    Args:
        output_folder: folder, starting in the working folder where the file is
        insetchart_name: file name (usually InsetChart.json by default)
        debug: flag, if true, data of interest is separately written out in the csv file

    Returns: DataFrame of daily infectious reservoir.

    """
    insetchart_path = path.join(output_folder, insetchart_name)
    with open(insetchart_path) as infile:
        icj = json.load(infile)["Channels"]
        human_infectious_reservoir_tmp = icj["Human Infectious Reservoir"]["Data"]
        #Channel calculation of Human Infectious Reservoir is now per person
        #Multiply each value times the number of people in the sim
        human_infectious_reservoir = [i * 1000 for i in human_infectious_reservoir_tmp]
    infectivity_df = pd.DataFrame.from_dict({"Infectivity": human_infectious_reservoir})
    if debug:
        infectivity_df.to_csv("Infectivity.csv")

    return infectivity_df


def create_report_file(param_obj, infections_and_feeds_df, infectivity_df, report_name, debug=False):
    with open(report_name, "w") as outfile:
        test_details = "scientific_feature_report_details.txt"
        with open(test_details, "w") as details_file:
            success = True
            acquire_modifier = param_obj[KEY_AQUIRE_MODIFIER]
            all_data_df = infections_and_feeds_df.join(infectivity_df)
            valid_tests = 0
            total_tests = 0
            successful_tests = 0
            # recalculating with aquire_modifier for true infectivity
            all_data_df["Infectivity"] = all_data_df["Infectivity"] * acquire_modifier
            all_data_df["Expected"] = all_data_df["Infectivity"] * all_data_df["Feeds"]
            all_data_df["Binomial Test"] = np.nan
            for i in all_data_df.index:
                if all_data_df["Infectivity"].loc[i] == 0:
                    if all_data_df["Infections"].loc[i] != 0:
                        success = False
                        outfile.write("BAD: On day {}, Human Infectious Reservoir was 0, but there is/are {} new"
                                      "vector infection(s).\n".format(i, all_data_df["Infections"].loc[i]))
                else:
                    total_tests += 1
                    result, valid = svet.test_binomial_95ci(all_data_df["Infections"].loc[i],
                                                            all_data_df["Feeds"].loc[i],
                                                            all_data_df["Infectivity"].loc[i],
                                                            details_file, "infections to vector from feeds")
                    if valid:
                        valid_tests += 1
                        successful_tests += 1 if result else 0
                        all_data_df.iloc[i, all_data_df.columns.get_loc('Binomial Test')] = 1 if result else 0

            outfile.write("Out of {} total tests, {} were valid, out of those {} were successful."
                          "\n".format(total_tests, valid_tests, successful_tests))
            good_enough = 0.945
            percentage_passed = float(successful_tests)/valid_tests
            if percentage_passed < good_enough:
                success = False
                all_data_df.to_csv("all_data_df.csv")
                outfile.write("BAD: Only {}% of {} valid tests passed, that's less than our {}% threshold. Please see {}"
                              " for details of failures.\n".format(percentage_passed*100, valid_tests,
                                                                   good_enough*100, test_details))
            else:
                outfile.write("GOOD: {}% of {} valid tests passed, that's more than our {}% threshold."
                              "\n".format(percentage_passed*100, valid_tests, good_enough*100))
            outfile.write("SUMMARY: Success={0}\n".format(success))

            if debug:
                all_data_df.to_csv("all_data_df.csv")


def application( output_folder="output", stdout_filename="test.txt",
                 config_filename="config.json",
                 insetchart_name="InsetChart.json",
                 report_name=sft.sft_output_filename,
                 debug=False):
    if debug:
        print("output_folder: " + output_folder)
        print("stdout_filename: " + stdout_filename + "\n")
        print("config_filename: " + config_filename + "\n")
        print("insetchart_name: " + insetchart_name + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")
    sft.wait_for_done()
    param_obj = load_emod_parameters(config_filename)
    infections_and_feeds_df = parse_stdout_file(stdout_filename, debug)
    infectivity_df = parse_json_report(output_folder, insetchart_name, debug)
    create_report_file(param_obj, infections_and_feeds_df, infectivity_df, report_name)


if __name__ == "__main__":
    # execute only if run as a script
    application( "output" )
