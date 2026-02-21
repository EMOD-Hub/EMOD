#!/usr/bin/python
import os
if __name__ == '__main__':
    from pathlib import Path
    import sys
    os.chdir(str(Path(sys.argv[0]).parent))
    sys.path.append( str(Path('../../../../shared_embedded_py_scripts').resolve().absolute()) )

import matplotlib
from sys import platform
if platform == "linux" or platform == "linux2":
    print('Linux OS. Using non-interactive Agg backend')
    matplotlib.use('Agg')
from matplotlib import pyplot as plt
import seaborn as sns
import numpy as np

import dtk_test.dtk_sft as dtk_sft
import dtk_test.dtk_ARTMortalityTable_Support as amt_s
from dtk_test.dtk_OutputFile import ReportEventRecorder
"""
This test is testing the HIV survival duration on ARTMortalityTable with different age bins. (Test 2: one 
AntiretroviralTherapy follows by one ARTMortalityTable intervention after 1 year, make sure the mortality rate is 
selected based on the age when ARTMortalityTable started instead of the age when AntiretroviralTherapy started.)

Data to test: ReportEventRecorder.csv

main test: test whether the survival duration on ARTMortalityTable(duration after intervention starts) follows an
exponential distribution with rate defined in the MortalityTable based on the age when AntiretroviralTherapy started . 
All dimensions except age have only one bin.

Output: scientific_feature_report.txt
        survival_durations_onART_lt-10.png
        survival_durations_onART_10-20.png
        survival_durations_onART_20-30.png
        survival_durations_onART_30-40.png
        survival_durations_onART_gt-40.png
        age_distribution.png
        
"""


def plot_age_distribution(event_startART_df, df_to_test):
    fig, axarr = plt.subplots(2, 2)  # 2 * 2 plots
    axarr[0, 0].scatter(x=np.arange(0, len(event_startART_df)), y=event_startART_df[amt_s.EventReport.age])
    axarr[0, 0].set_title("when AntiretroviralTherapy started")
    axarr[0, 0].set_ylabel("Age(years)")
    axarr[0, 0].set_xlabel("data points")
    axarr[0, 1].scatter(x=np.arange(0, len(df_to_test)), y=df_to_test[amt_s.EventReport.age])
    axarr[0, 1].set_title("age tested")
    axarr[0, 1].set_xlabel("data points")

    sns.histplot(event_startART_df[amt_s.EventReport.age], kde=True, ax=axarr[1, 0])
    sns.histplot(df_to_test[amt_s.EventReport.age], kde=True, ax=axarr[1, 1])
    axarr[1, 0].set_ylabel("Probability")

    fig.suptitle('Age Distribution')
    fig.tight_layout()
    fig.subplots_adjust(top=0.88)
    plt.savefig("age_distribution.png")
    if dtk_sft.check_for_plotting():
        plt.show()
    plt.close()


def create_report_file(event_df, config_filename, report_name, debug):

    succeed = True
    with open(report_name, 'w') as output_report_file:
        # load parameters to test from config
        base_year, simulation_duration, simulation_duration_year, simulation_timestep, campaign_filename = \
            amt_s.load_config(config_filename, output_report_file)

        # load campaign file with both types of interventions
        campaign_object = amt_s.load_campaign_ART(campaign_filename)

        art_start_day = art_mortality_table_start_day = None
        for campaign in campaign_object:
            start_day, ip_name, ip_value, mortality_table = campaign
            if ip_value or ip_name:
                output_report_file.write(f"WARNING: {amt_s.Campaign.property_restrictions} for "
                                         f"{amt_s.Campaign.ARTMortalityTable} is {ip_name}: {ip_value}, expect empty"
                                         f"string, please fix the test.\n")
            if mortality_table:
                art_mortality_table_start_day = start_day
                art_mortality_table = mortality_table
                expected_one_bin = [amt_s.Campaign.art_duration_days_bins,
                                    amt_s.Campaign.cd4_count_bins]
                check_campaign_result = True
                for param_name in expected_one_bin:
                    value = art_mortality_table[param_name]
                    if len(value) != 1:
                        check_campaign_result = succeed = False
                        output_report_file.write(f"BAD: test is expected 1 bin in {param_name}, got {len(value)}, "
                                                 f"please check the test.\n")
                if check_campaign_result:
                    output_report_file.write(
                        f"GOOD: {expected_one_bin} all have 1 bin. Mortality rate is the same within"
                        f" the same {amt_s.Campaign.age_years_bins}.\n")
                age_years_bins = art_mortality_table[amt_s.Campaign.age_years_bins]
                if len(age_years_bins) == 1:
                    output_report_file.write(
                        f"WARNING: {amt_s.Campaign.age_years_bins} only has one element, please test more than"
                        f"one age_years_bins.\n")
            else:
                art_start_day = start_day

        if len(campaign_object) != 2 or not art_start_day or not art_mortality_table_start_day:
            succeed = False
            output_report_file.write(f"BAD: {len(campaign_object)} {amt_s.Campaign.ARTMortalityTable} and "
                                     f"{amt_s.Campaign.ART} intervention(s) in the {campaign_filename}, expected 1 "
                                     f"{amt_s.Campaign.ARTMortalityTable} and 1 {amt_s.Campaign.ART} intervention, "
                                     f"please check the test.\n")

        # prepare test data
        # filter by DiseaseDeath event and time >= ARTMortalityTable start day
        event_death_df = event_df[((event_df[amt_s.EventReport.year]) >
                                    art_mortality_table_start_day / dtk_sft.DAYS_IN_YEAR + base_year) &
                                  (event_df[amt_s.EventReport.event_name] == amt_s.EventReport.death)][[
            amt_s.EventReport.year, amt_s.EventReport.ind_id]]

        # filter by StartedART event and time >= ARTMortalityTable start day
        event_startART_df = event_df[event_df[amt_s.EventReport.event_name] == amt_s.EventReport.started_art]
        event_startART_df[amt_s.EventReport.age] = event_startART_df[amt_s.EventReport.age] / dtk_sft.DAYS_IN_YEAR


        event_startART_df = event_startART_df[[amt_s.EventReport.ind_id,
                                               amt_s.EventReport.age]]

        event_startART_df.sort_values(amt_s.EventReport.age, inplace=True)
        # dropping ALL duplicate values and keep the first value which is the values when AntiretroviralTherapy starts
        event_startART_df.drop_duplicates(subset=amt_s.EventReport.ind_id, keep='first', inplace=True)
        # dropping ALL duplicate values and keep the last value which is the values when ARTMortalityTable starts
        # event_startART_df.drop_duplicates(subset=amt_s.EventReport.ind_id, keep='last', inplace=True)

        # merge the age information when ART starts to the event_death_df
        df_to_test = event_death_df.merge(event_startART_df, on=amt_s.EventReport.ind_id, how="inner")
        df_to_test.sort_values(amt_s.EventReport.age, inplace=True)
        df_to_test[amt_s.EventReport.age] += (art_mortality_table_start_day - art_start_day)/dtk_sft.DAYS_IN_YEAR
        if debug:
            with open("DEBUG_df_to_test.csv", "w") as csv_file:
                df_to_test.to_csv(csv_file)

        # Plot age distribution at the times when AntiretroviralTherapy and ARTMortalityTable started
        plot_age_distribution(event_startART_df, df_to_test)

        df_to_test[amt_s.EventReport.age_bin] = df_to_test[amt_s.EventReport.age].\
            apply(lambda x: amt_s.convert_value_to_bin(
            x, age_years_bins))
            #(x - (art_mortality_table_start_day - art_start_day)) / dtk_sft.DAYS_IN_YEAR, age_years_bins))

        age_bins = df_to_test[amt_s.EventReport.age_bin].unique()
        mortality_table = art_mortality_table[amt_s.Campaign.mortality_table]
        # main test
        res_msg = amt_s.test_survival_time_on_age(df_to_test, age_bins, age_years_bins, mortality_table,
                                                  art_mortality_table_start_day, base_year, simulation_duration_year)
        for msg in res_msg:
            output_report_file.write(msg)
            if "BAD" in msg:
                succeed = False

        output_report_file.write(dtk_sft.format_success_msg(succeed))

        return succeed


def application(output_folder="output", stdout_filename="test.txt",
                event_report_name="ReportEventRecorder.csv",
                config_filename="config.json",
                report_name=dtk_sft.sft_output_filename,
                debug=False):
    if debug:
        print("output_folder: " + output_folder+ "\n")
        print("stdout_filename: " + stdout_filename + "\n")
        print("event_report_name: " + event_report_name + "\n")
        print("config_filename: " + config_filename + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    dtk_sft.wait_for_done(stdout_filename)

    event_df = ReportEventRecorder(os.path.join(output_folder, event_report_name)).df

    create_report_file(event_df,  config_filename, report_name, debug)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt)")
    parser.add_argument('-e', '--event_report', default="ReportEventRecorder.csv",
                        help="Report Event Recorder to parse (ReportEventRecorder.csv)")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-r', '--reportname', default=dtk_sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', help="debug flag", action='store_true')
    args = parser.parse_args()

    application(output_folder=args.output, stdout_filename=args.stdout,
                event_report_name=args.event_report,
                config_filename=args.config,
                report_name=args.reportname, debug=True)#args.debug)

