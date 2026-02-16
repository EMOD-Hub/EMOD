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


"""
This test is testing the HIV survival duration on ARTMortalityTable with different ART Adherence levels. 

Data to test: ReportEventRecorder.csv

main test: test whether the survival duration on ARTMortalityTable(duration after intervention starts) follows an
exponential distribution with rate defined in the MortalityTable. All dimensions except Adherence levels have only one 
bin.
            
Output: scientific_feature_report.txt
        survival_durations_onART_Adherence-HIGH.png
        survival_durations_onART_Adherence-LOW.png
        
"""


def create_report_file( event_df, config_filename, report_name):
    succeed = True
    with open(report_name, 'w') as output_report_file:
        # load parameters to test from config
        base_year, simulation_duration, simulation_duration_year, simulation_timestep, campaign_filename = \
            amt_s.load_config(config_filename, output_report_file)

        # load campaign file
        campaign_object = amt_s.load_campaign(campaign_filename)
        if len(campaign_object) <= 1:
            succeed = False
            output_report_file.write(f"BAD: There are {len(campaign_filename)} {amt_s.Campaign.ARTMortalityTable} "
                                     f"intervention in the {campaign_filename}, expect at least 2 "
                                     f"{amt_s.Campaign.ARTMortalityTable} interventions, please check the test.\n")

        # main test
        for start_day, ip_name, ip_value, art_mortality_table in campaign_object:
            output_report_file.write(f"Test {ip_name}: {ip_value} group:\n")
            # prepare test data
            event_df_ip = event_df[(event_df[ip_name] == ip_value) &
                                   (event_df[amt_s.EventReport.event_name] == amt_s.EventReport.death)]
            death_year = event_df_ip[amt_s.EventReport.year]
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
                    output_report_file.write(f"\tBAD: test is expected 1 bin in {param_name} for {ip_name}: {ip_value},"
                                             f" got {len(value)}, please check the test.\n")
            if check_campaign_result:
                output_report_file.write(f"\tGOOD: {expected_one_bin} all have 1 bin. Mortality rate is the same within"
                                         f" the IP group.\n")

            rate = art_mortality_table[amt_s.Campaign.mortality_table][0][0][0] # only one bin in each dimension.

            output_report_file.write(f"\tTest if survival duration on ARTMortalityTable for {amt_s.EventReport.ip_name}: "
                                     f"{ip_value} follows exponential distribution with rate = {rate}:\n")
            output_report_file.write(f"\tTest sample size = {len(duration_to_test)}.\n")

            # comment out, need to round theoretical data and check maximum duraiton before passing them to test.
            # output_report_file.write("\t")
            # result = dtk_sft.test_exponential(duration_to_test, rate, report_file=output_report_file,
            #                                   integers=False, roundup=False, round_nearest=False)
            # if not result:
            #     succeed = False

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

            output_report_file.write(f"\tPlease see survival_durations_onART_{ip_name}-{ip_value}.png.\n")

            # plot data, histogram and CDF with test data and theoretical data/cdf.
            dtk_sft.three_plots(duration_to_test,
                                cdf_function=stats.expon.cdf, args=(0, 1 / rate),
                                dist2=dist_expon_scipy[:len(duration_to_test)],
                                label1="Emod", label2="scipy",
                                title=f"survival_durations_onART_{ip_name}-{ip_value}",
                                xlabel="Data Points", ylabel="Duration(Years)",
                                category=f"survival_durations_onART_{ip_name}-{ip_value}",
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

