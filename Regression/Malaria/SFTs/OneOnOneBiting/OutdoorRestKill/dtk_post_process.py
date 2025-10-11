#!/usr/bin/python

import dtk_test.dtk_sft as sft
import dtk_test.dtk_Vector_Support as veds
import pandas as pd
import os

"""
The OutdoorRestKill intervention class imposes node-targeted mortality to a vector
that is at rest in an outdoor environment after an outdoor feed on a human.
With 0.7 of vectors feeding outside and Initial_Effect: 0.25, we
expect 0.25 of outdoor feeding vectors to die every day for 3 days.
Male vectors rest outside every day and so are reduced every day by Initial_Effect.
"""


def create_biting_report_file(param_obj,report_name, output_folder, report_vector_stats, debug=False):
    success = True
    with open(report_name, "w") as outfile:
        outfile.write(f"Test name: {str(param_obj[veds.ConfigKeys.CONFIG_NAME])} \n"
                      f"Run number: {str(param_obj[veds.ConfigKeys.RUN_NUMBER])} \n")
        rsv_path = os.path.join(output_folder, report_vector_stats)
        vectors_df = pd.read_csv(rsv_path)
        # check females, 0.25 of all OutdoorBitesCount - which is the females that fed outdoors that day,
        #  should be dying for three days we look at outdoor bites and outdoor deaths per day in the report_vector_stats

        # check males, 0.25 of all males should be dying every day for the duration of the intervention (3 days)
        # for males, we look at the population the day before and on the day of and calculate the deaths that way,
        # with 730 day lifespan daily "natural" death rate for male is negligible for this test
        initial_effect = 0.25
        allowable_error = 0.01
        for day in range(2, 5):
            today_df = vectors_df[vectors_df["Time"] == day]
            outdoor_bites = today_df.iloc[0]["OutdoorBitesCount"]
            indoor_bites = today_df.iloc[0]["IndoorBitesCount"]
            died_feeding_outdoor = today_df.iloc[0]["DiedDuringFeedingOutdoor"]
            died_feeding_indoor = today_df.iloc[0]["DiedDuringFeedingIndoor"]
            if abs(died_feeding_outdoor/outdoor_bites - initial_effect) > allowable_error:
                success = False
                outfile.write(f"BAD: We were expecting {initial_effect} of {outdoor_bites} outdoor bites "
                              f"to end in death ({initial_effect*outdoor_bites}), but {died_feeding_outdoor} "
                              f"have \nand that's not close enough to "
                              f"the right proportion. Please check.\n")
            else:
                outfile.write(f"GOOD: We were expecting {initial_effect} of {outdoor_bites} outdoor bites "
                              f"to end in death ({initial_effect*outdoor_bites}) and {died_feeding_outdoor} "
                              f"have \nand that's close enough to "
                              f"the right proportion.\n")
            if abs(died_feeding_indoor/indoor_bites - initial_effect) > allowable_error:
                success = False
                outfile.write(f"BAD: We were expecting {initial_effect} of {indoor_bites} indoor bites "
                              f"to end in death ({initial_effect*indoor_bites}), but {died_feeding_indoor} "
                              f"have \nand that's not close enough to "
                              f"the right proportion. Please check.\n")
            else:
                outfile.write(f"GOOD: We were expecting {initial_effect} of {indoor_bites} indoor bites "
                              f"to end in death ({initial_effect*indoor_bites}) and {died_feeding_indoor} "
                              f"have \nand that's close enough to "
                              f"the right proportion.\n")

            # Male population gets affected by the intervention on the same timestep while the female feeding population
            # does not get affected until the timestep after the intervention is applied
            male_day = day - 1
            today_df = vectors_df[vectors_df["Time"] == male_day]
            male_previous_day_pop = vectors_df[vectors_df["Time"] == male_day - 1].iloc[0]["STATE_MALE"]
            male_pop = today_df.iloc[0]["STATE_MALE"]
            if abs((male_previous_day_pop - male_pop)/male_previous_day_pop - initial_effect) > allowable_error:
                success = False
                outfile.write(f"BAD: We were expecting {initial_effect} of {male_previous_day_pop} male vectors "
                              f"to die,({initial_effect*male_previous_day_pop}) but {male_previous_day_pop - male_pop} "
                              f"have \nand that's not close enough to "
                              f"the right proportion. Please check.\n")
            else:
                outfile.write(f"GOOD: We were expecting {initial_effect} of {outdoor_bites} male vectors "
                              f"to die ({initial_effect*male_previous_day_pop}) and {male_previous_day_pop - male_pop} "
                              f"have \nand that's close enough to "
                              f"the right proportion.\n")

        outfile.write(sft.format_success_msg(success))


def application(output_folder="output",
                config_filename="config.json",
                report_vector_stats="ReportVectorStats.csv",
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
                              report_vector_stats=report_vector_stats, debug=debug)

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
