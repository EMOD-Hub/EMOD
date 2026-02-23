#!/usr/bin/python
import os
if __name__ == '__main__':
    from pathlib import Path
    import sys
    os.chdir(str(Path(sys.argv[0]).parent))
    sys.path.append( str(Path('../../../../shared_embedded_py_scripts').resolve().absolute()) )

import dtk_test.dtk_sft as dtk_sft
import dtk_test.dtk_ARTMortalityTable_Support as amt_s
from dtk_test.dtk_OutputFile import ReportEventRecorder
from scipy import stats
import json

"""
This test is testing the HIV survival duration on ARTMortalityTable for female and male. 

Data to test: ReportEventRecorder.csv

main test: test whether the survival duration on ARTMortalityTable(duration after intervention starts) follows an
exponential distribution with rate defined in the MortalityTable. All dimensions except Target_Gender have only one 
bin.
            
Output: scientific_feature_report.txt
        survival_durations_onART_gender-Female.png
        survival_durations_onART_gender-Male.png
        
"""


def load_campaign_gender(campaign_filename):
    with open(campaign_filename, 'r') as campaign_file:
        cf = json.load(campaign_file)
        try:
            events = cf[amt_s.Campaign.events]
            res = []
            for event in events:
                iv = event[amt_s.Campaign.event_coordinator_config][amt_s.Campaign.intervention_config]
                if iv[amt_s.Campaign.class_name] == amt_s.Campaign.ARTMortalityTable:
                    start_day = event[amt_s.Campaign.start_day]
                    target_gender = event[amt_s.Campaign.event_coordinator_config][amt_s.Campaign.target_gender]
                    art_duration_days_bins = iv[amt_s.Campaign.art_duration_days_bins]
                    age_years_bins = iv[amt_s.Campaign.age_years_bins]
                    cd4_count_bins = iv[amt_s.Campaign.cd4_count_bins]
                    mortality_table = iv[amt_s.Campaign.mortality_table]
                    res.append([start_day, target_gender,
                                {amt_s.Campaign.art_duration_days_bins: art_duration_days_bins,
                                 amt_s.Campaign.age_years_bins: age_years_bins,
                                 amt_s.Campaign.cd4_count_bins: cd4_count_bins,
                                 amt_s.Campaign.mortality_table: mortality_table}])
            return res

        except KeyError as ke:
            raise KeyError(f"{ke} is not found in {campaign_filename}.\n")


def create_report_file( event_df, config_filename, report_name):
    succeed = True
    with open(report_name, 'w') as output_report_file:
        # load parameters to test from config
        base_year, simulation_duration, simulation_duration_year, simulation_timestep, campaign_filename = \
            amt_s.load_config(config_filename, output_report_file)

        # load campaign file
        campaign_object = load_campaign_gender(campaign_filename)
        if len(campaign_object) <= 1:
            succeed = False
            output_report_file.write(f"BAD: There are {len(campaign_filename)} {amt_s.Campaign.ARTMortalityTable} "
                                     f"intervention in the {campaign_filename}, expect at least 2 "
                                     f"{amt_s.Campaign.ARTMortalityTable} interventions, please check the test.\n")

        # main test
        for start_day, target_gender, art_mortality_table in campaign_object:
            output_report_file.write(f"Test gender: {target_gender} group:\n")
            # prepare test data
            if target_gender == "Male":
                gender = amt_s.EventReport.m
            elif target_gender == "Female":
                gender = amt_s.EventReport.f
            else:
                raise ValueError(f"{amt_s.Campaign.target_gender} must be Male or Female in this test, please fix"
                                 f" the test.\n")
            event_df_gender = event_df[(event_df[amt_s.EventReport.gender] == gender) &
                                   (event_df[amt_s.EventReport.event_name] == amt_s.EventReport.death)]
            death_year = event_df_gender[amt_s.EventReport.year]
            duration_to_test = [(x - base_year - start_day / dtk_sft.DAYS_IN_YEAR)
                                for x in death_year
                                if (x - base_year) * dtk_sft.DAYS_IN_YEAR > start_day]

            # check each intervention configuration
            expected_one_bin = [amt_s.Campaign.art_duration_days_bins,
                                amt_s.Campaign.age_years_bins,
                                amt_s.Campaign.cd4_count_bins]
            check_campaign_result = True
            for param_name in expected_one_bin:
                value = art_mortality_table[param_name]
                if len(value) != 1:
                    check_campaign_result = succeed = False
                    output_report_file.write(f"\tBAD: test is expected 1 bin in {param_name} for gender: {target_gender},"
                                             f" got {len(value)}, please check the test.\n")
            if check_campaign_result:
                output_report_file.write(f"\tGOOD: {expected_one_bin} all have 1 bin. Mortality rate is the same within"
                                         f" the IP group.\n")

            rate = art_mortality_table[amt_s.Campaign.mortality_table][0][0][0] # only one bin in each dimension.

            output_report_file.write(f"\tTest if survival duration on ARTMortalityTable for gender: "
                                     f"{target_gender} follows exponential distribution with rate = {rate}:\n")
            output_report_file.write(f"\tTest sample size = {len(duration_to_test)}.\n")

            # Statistic test
            dist_expon_scipy = stats.expon.rvs(loc=0, scale=1 / rate, size=max(100000, len(duration_to_test)))
            # make sure the simulation duration is long enough:
            if max(dist_expon_scipy) > simulation_duration_year * 1.1:
                output_report_file.write(f"\tWARNING: simulation_duration = {simulation_duration_year} years may not "
                                         f"be long enough here. Maximum value draw from scipy exponential distribution"
                                         f"with rate = {rate} is {max(dist_expon_scipy)}.\n")
                output_report_file.write(f"\tCap theoretical data at {simulation_duration_year * 1.1} years.\n")
                dist_expon_scipy = [x for x in dist_expon_scipy if x < simulation_duration_year * 1.1]

            # reporting precision for year in csv report is 2 decimal places.
            dist_expon_scipy = [dtk_sft.round_up(x, 2) for x in dist_expon_scipy]
            result = stats.anderson_ksamp([duration_to_test, dist_expon_scipy])
            if hasattr(result, "pvalue"):  # SciPy > v1.17
                p_value = result.pvalue
            else:
                p_value = result.significance_level
            s = result.statistic
            msg = f"anderson_ksamp() with rate = {rate}(per year) return p value = " \
                f"{p_value} and statistic = {s}.\n"
            if p_value < 5e-2:
                succeed = False
                output_report_file.write("\tBAD: statistical test on survival duration failed! " + msg)
            else:
                output_report_file.write("\tGOOD: statistical test on survival duration passed! " + msg)

            output_report_file.write(f"\tPlease see survival_durations_onART_gender-{target_gender}.png and "
                                     f"expon_cdf_gender-{target_gender}.png.\n")

            # plot data, histogram and CDF with test data and theoretical data/cdf.
            dtk_sft.three_plots(duration_to_test,
                                cdf_function=stats.expon.cdf, args=(0, 1 / rate),
                                dist2=dist_expon_scipy[:len(duration_to_test)],
                                label1="Emod", label2="scipy",
                                title=f"survival_durations_onART_gender-{target_gender}",
                                xlabel="Data Points", ylabel="Duration(Years)",
                                category=f"survival_durations_onART_gender-{target_gender}",
                                show=True, sort=True)
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

    create_report_file(event_df,  config_filename, report_name)


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

