#!/usr/bin/python
import os

if __name__ == '__main__':
    from pathlib import Path
    import sys

    os.chdir(str(Path(sys.argv[0]).parent))
    sys.path.append(str(Path('../../../../shared_embedded_py_scripts').resolve().absolute()))

import dtk_test.dtk_sft as dtk_sft
import dtk_test.dtk_ARTMortalityTable_Support as amt_s
from dtk_test.dtk_OutputFile import ReportEventRecorder
from scipy import stats

"""
This test is testing the HIV survival duration on ARTMortalityTable with different ART duration bins. (Test 1: make sure 
ARTMortalityTable doesn't stop or restart ART if a person is already on ART; and test if ART_Duration_Days_Bins is 
working correctly with large duration bins(duration bin > days until death), P-value is 5% in this test.)

Data to test: ReportEventRecorder.csv

main test: test whether the survival duration on ARTMortalityTable(duration after intervention starts) follows an
exponential distribution with rate defined in the MortalityTable. All dimensions except ART duration have only one bin.

Output: scientific_feature_report.txt
        survival_durations_onART_ARTDuraiton-0.png
        survival_durations_onART_ARTDuraiton-4005.png
        survival_durations_onART_ARTDuraiton-8004.png

"""


def create_report_file(event_df, config_filename, event_report_name, report_name):
    succeed = True
    messages = []
    with open(report_name, 'w') as output_report_file:
        # load parameters to test from config
        base_year, simulation_duration, simulation_duration_year, simulation_timestep, campaign_filename = \
            amt_s.load_config(config_filename, output_report_file)

        # load campaign file
        campaign_object = amt_s.load_campaign_ART(campaign_filename)
        if len(campaign_object) != 3:
            messages.append(f"BAD: {len(campaign_object)} {amt_s.Campaign.ARTMortalityTable} and "
                            f"{amt_s.Campaign.ART} intervention(s) in the {campaign_filename}, expected 2 "
                            f"{amt_s.Campaign.ARTMortalityTable} and {amt_s.Campaign.ART} intervention, "
                            f"please check the test.\n")

        # parse campaign object
        art_start_days, art_duration_days_bins, art_mortality_table, art_mortality_table_start_day = \
            amt_s.parse_campaign(campaign_object, messages)

        # make sure ARTMortalityTable doesn't stop ART
        if not event_df[event_df[amt_s.EventReport.event_name] == amt_s.EventReport.stopped_art].empty:
            messages.append(f"BAD: {amt_s.Campaign.ARTMortalityTable} should not stop the ART intervention. "
                            f"there are {amt_s.EventReport.stopped_art} events broadcast in "
                            f"{event_report_name}.\n")
        else:
            messages.append(f"GOOD: {amt_s.Campaign.ARTMortalityTable} does not stop the ART intervention.\n")

        # load ip overlay file
        ip_name, ip_values = amt_s.load_ip_group(dtk_sft.get_config_parameter(config_filename,
                                                                              amt_s.ConfigParam.demographics_filenames)[
                                                     0][-1])

        # make sure ARTMortalityTable doesn't restart ART
        amt_s.check_ART_start_event(event_df, ip_name, art_start_days, base_year, event_report_name, messages)

        # main test
        # filter by: time > ARTMortalityTable start day and event = DiseaseDeath
        df_to_test = event_df[((event_df[amt_s.EventReport.year]) >
                               art_mortality_table_start_day / dtk_sft.DAYS_IN_YEAR + base_year) &
                              (event_df[amt_s.EventReport.event_name] == amt_s.EventReport.death)]

        for ip_value in ip_values:
            ip_group = ip_name + "-" + ip_value
            start_day = art_start_days[ip_group] if ip_group in art_start_days else art_mortality_table_start_day

            art_duration = art_mortality_table_start_day - start_day
            # filter by property group
            ip_df = df_to_test[df_to_test[ip_name] == ip_value]

            # calculate survival duration
            ip_df[amt_s.EventReport.survival_duration] = ip_df[amt_s.EventReport.year] - base_year - \
                                                         art_mortality_table_start_day / dtk_sft.DAYS_IN_YEAR
            duration_to_test = ip_df[amt_s.EventReport.survival_duration].tolist()

            # get rate and max_duration_current_bin
            rate, max_duration_current_bin = amt_s.get_rate_by_ARTduration(art_duration,
                                                                           art_duration_days_bins,
                                                                           art_mortality_table)

            messages.append(f"Test property group {ip_name}: {ip_value} with ART Duration = {art_duration}\n")

            messages.append(f"\tTest if survival duration on ARTMortalityTable for property group "
                            f"{ip_name}: {ip_value} follows exponential distribution with rate = {rate}:\n")

            # data prep step: cap dist_expon_scipy on simulation duration;
            #                 cap dist_expon_scipy and duration_to_test on max_duration_current_bin - art_duration
            dist_expon_scipy, duration_to_test = amt_s.data_prep(rate, duration_to_test, art_mortality_table,
                                                                 simulation_duration_year,
                                                                 art_mortality_table_start_day,
                                                                 max_duration_current_bin, art_duration, ip_group,
                                                                 messages, None, None, None)

            # run statistical test and generate plots.
            amt_s.test_survival_time_on_duration(duration_to_test, dist_expon_scipy, rate, messages, art_duration,
                                                 p=5e-2, cdf_function=stats.expon.cdf)

        for msg in messages:
            if "BAD" in msg:
                succeed = False
            output_report_file.write(msg)
        output_report_file.write(dtk_sft.format_success_msg(succeed))

        return succeed


def application(output_folder="output", stdout_filename="test.txt",
                event_report_name="ReportEventRecorder.csv",
                config_filename="config.json",
                report_name=dtk_sft.sft_output_filename,
                debug=False):
    if debug:
        print("output_folder: " + output_folder + "\n")
        print("stdout_filename: " + stdout_filename + "\n")
        print("event_report_name: " + event_report_name + "\n")
        print("config_filename: " + config_filename + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    dtk_sft.wait_for_done(stdout_filename)

    event_df = ReportEventRecorder(os.path.join(output_folder, event_report_name)).df

    create_report_file(event_df, config_filename, event_report_name, report_name)


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
                report_name=args.reportname, debug=args.debug)

