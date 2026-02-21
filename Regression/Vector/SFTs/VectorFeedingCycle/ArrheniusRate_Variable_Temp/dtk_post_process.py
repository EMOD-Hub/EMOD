#!/usr/bin/python

import dtk_test.dtk_sft as sft
import dtk_test.dtk_Vector_Support as veds
import pandas as pd
import numpy as np

"""
Spec: 
    The duration of Aedes feeding cycle shall be a function of air temperature on the day of previous feed. 
    The function shall behave according to the following functional form: the Arrhenius equation, a1 * exp( -a2 / T ), 
    where T is temperature in degrees Kelvin. The 1/the rate calculated using this function signifies the average of 
    an exponentially distributed duration of the feeding cycle.
    Relevant parameters:
    Temperature_Dependent_Feeding_Cycle: "ARRHENIUS_DEPENDENCE"
    Cycle_Arrhenius_1	4.09E+10 (a1)
    Cycle_Arrhenius_2	7740     (a2)

In this test, we will be recording the calculation, the temperature at the calculation and the actual feed duration, 
grouping the mosquitoes by the day they fed on to verify that the average of their feeding times are close to the 
calculated as mosquitoes feed on whole days.

Note: This does not verify that vectors feed on the days that are calculated. I didn't see individual vector id that 
I could track
TODO: add observing vectors feeding. 

"""


def parse_output_file(stdout, debug=False):
    """
    Parses following lines, example:

    Mean gonotrophic cycle duration = 10.54870 days at 15.77 degrees C.

    Args:
        stdout: the logging file to parse
        debug: flag, if true writes out the lines that were used to collect the data

    Returns: data frame

    """
    time = 0

    data_rows = []
    with open(stdout) as logfile:
        for line in logfile:
            if "Time: " in line:
                time += 1
            if "[VectorPopulationIndividual] Mean gonotrophic" in line:
                temperature_k = float(sft.get_val("days at ", line)) + 273.15
                duration_in_days = float(sft.get_val("gonotrophic cycle duration = ", line))
                timer = int(sft.get_val("timer set to ", line))
                data_rows.append({'Time': time, 'Temperature': temperature_k,
                                                        'Duration': duration_in_days, 'Timer': timer})

    gonothropic_df = pd.DataFrame(data=data_rows, columns='Time Temperature Duration Timer'.split())
    if debug:
        with open("DEBUG_filtered_lines.txt", "w") as debug_file:
            debug_file.write(str(gonothropic_df))
    return gonothropic_df


def create_report_file(param_obj, report_name, gonothropic_df, debug=False):
    success = True
    error_epsilon = 5e-2
    with open(report_name, "w") as outfile:
        if len(gonothropic_df) == 0:
            outfile.write(sft.sft_no_test_data)
            raise ValueError("No data in DataFrame!")
        outfile.write("# Test name: " + param_obj[veds.ConfigKeys.CONFIG_NAME] + ", Run number: " +
                      str(param_obj[veds.ConfigKeys.RUN_NUMBER]) +
                      "\n# Test compares between the actual_feeding cycle and expected feeding cycle durations at "
                      "varying temperatures to verify vectors feed at Arrhenius rate as a function of temperature.\n")
        gonothropic_df["theoretical rate"] = None

        for index in gonothropic_df.index:
            temperature_k = float(gonothropic_df.at[index, "Temperature"])
            theoretical_duration = 1/veds.vector_dur_calc(temperature_k, param_obj)
            gonothropic_df.at[index, "Theoretical Duration"] = theoretical_duration
            actual_duration = float(gonothropic_df.at[index, "Duration"])
            if abs(actual_duration-theoretical_duration) > error_epsilon * theoretical_duration:
                low_acceptance_bound = theoretical_duration - error_epsilon * theoretical_duration
                high_acceptance_bound = theoretical_duration + error_epsilon * theoretical_duration
                success = False
                outfile.write("BAD: At temperature {}, feed duration = {}, which is not within acceptance range of "
                              "({}, {}). Expected {}.\n".format(temperature_k, actual_duration, low_acceptance_bound,
                                                                high_acceptance_bound, theoretical_duration))
        if success:
            outfile.write("GOOD: For all temperatures, feeding cycle was within {}% of "
                          "theoretical.\n".format(error_epsilon*100))

        grouped_std = gonothropic_df.groupby('Time', as_index=True).agg(['mean', 'count'])
        grouped_std['feed error'] = abs(grouped_std['Theoretical Duration']['mean'] - grouped_std['Timer']['mean'])\
                                    / grouped_std['Theoretical Duration']['mean']
        # We are checking that the feeding duration on average makes sense for specific day/temperature
        # ignoring days with not enough feeds (<=20) and 0th time step as it has two temperatures
        minimum_samples = 20
        grouped_std = grouped_std[grouped_std['Temperature']['count'] > minimum_samples].truncate(before=1)

        if (grouped_std["feed error"] > error_epsilon).any():  # making sure calculated and mean feeding times are
                                                            # withing 5% of each other
            success = False
            outfile.write("BAD: One or more feed errors exceed {error_percent}% between theoretical duration and "
                          "actual mean feeding duration time.\n".format(error_percent=error_epsilon*100))
        else:
            outfile.write("GOOD: All temperatures with more than {} samples, have a mean feeding duration within "
                          "{}% of theoretical.\n".format(minimum_samples, error_epsilon*100))
        for_plot_df = pd.DataFrame({"Temperature": grouped_std['Temperature']['mean'],
                                    "Theoretical Duration": grouped_std['Theoretical Duration']['mean'],
                                    "Timer": grouped_std['Timer']['mean']})
        ax1 = for_plot_df.plot(kind='scatter', x='Temperature', y='Theoretical Duration', color='r', label='Theoretical')
        ax2 = for_plot_df.plot(kind='scatter', x='Temperature', y='Timer', color='k', ax=ax1, label='Mean Actual', marker="+",
                               title="Theoretical and actual mean feeding times")
        ax2.set(xlabel='Temperature in K', ylabel="Feeding cycle duration")
        fig = ax2.get_figure()
        fig.savefig("ArrheniusRate_Var_Temp_plot.png")

        outfile.write(sft.format_success_msg(success))


def application(output_folder="output",
                config_filename="config.json",
                stdout="test.txt",
                report_name=sft.sft_output_filename,
                debug=False):
    if debug:
        print("output_folder: " + output_folder)
        print("config_filename: " + config_filename + "\n")
        print("stdout_filename: " + stdout + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    sft.wait_for_done()
    param_obj = veds.load_emod_parameters(config_filename)
    output_df = parse_output_file(stdout, debug)
    create_report_file(param_obj, report_name, output_df, debug)


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
