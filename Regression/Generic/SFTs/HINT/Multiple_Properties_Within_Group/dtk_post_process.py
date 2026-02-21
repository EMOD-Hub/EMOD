#!/usr/bin/python

import dtk_test.dtk_sft as sft
import math
from dtk_test.dtk_General_Support import ConfigKeys, CampaignKeys, DemographicsKeys, InsetKeys
import numpy as np
from scipy import stats
import dtk_test.dtk_HINT_Support as hint_support
"""
This SFT tests multiple properties Heterogeneous Intranode Transmission within the same property group. 

Outbreak intervention is delivered to QualityOfCare:Group_1 and Risk:High at time step 0 with coverrage = 0.05.

Similar to Multiple_Properties_Between_Groups, we only expect the new infection in the target groups and no new 
infection in all other groups. Also this SFT is testing contagion, infection probablity and new infections 
channel(from PropertyReport.csv) at the group level for every time step. 
"""

def create_report_file(param_obj, campaign_obj, stdout_df, property_df, property_list, report_name, debug):
    with open(report_name, "w") as outfile:
        config_name = param_obj[ConfigKeys.Config_Name]
        base_infectivity = param_obj[ConfigKeys.Base_Infectivity]
        outfile.write("Config_name = {}\n".format(config_name))
        outfile.write("{0} = {1} {2} = {3}\n".format(
            ConfigKeys.Base_Infectivity, base_infectivity,
            ConfigKeys.Run_Number, param_obj[ConfigKeys.Run_Number]))

        success = True

        outfile.write("Test 1: checking test conditions/setup in config.json and campaign.json:\n")
        if int(param_obj[ConfigKeys.Enable_Heterogeneous_Intranode_Transmission]) != 1:
            success = False
            outfile.write("BAD: HINT is not enabled, please check the test.\n")
        else:
            if not all(x == 0 for x in campaign_obj[CampaignKeys.Start_Day]):
                success = False
                outfile.write("BAD: All intervention should start at day 0, please check campaign.json.\n")

            if not all(x == 0.05 for x in campaign_obj[CampaignKeys.Demographic_Coverage]):
                success = False
                outfile.write("BAD: {} should be 0.05, please check campaign.json.\n".format(CampaignKeys.Demographic_Coverage))

            expected_property_restrictions = []
            test_groups = []
            non_test_groups= []
            for property_obj in property_list:
                property_values = property_obj[DemographicsKeys.PropertyKeys.Values]
                test_groups.append(property_values[0])
                non_test_groups.append(property_values[1])
                expected_property_restrictions.append(["{0}:{1}".format(
                    property_obj[DemographicsKeys.PropertyKeys.Property], property_values[0])])
            if campaign_obj[CampaignKeys.Property_Restrictions] != expected_property_restrictions:
                success = False
                outfile.write(
                    "BAD: {0} should be {1}, got {2} in campaign.json, please check campaign.json.\n".format(
                        CampaignKeys.Property_Restrictions, expected_property_restrictions,
                        campaign_obj[CampaignKeys.Property_Restrictions]))

        outfile.write("Test 1: campaign and config met preconditions: {}.\n".format(success))

        if success:
            outfile.write("Test 2: Testing contagion and probability with calculated values for every time step:\n")
            outfile.write("Test 3: Testing New Infection channel from property report based on transmission matrix:\n")
            outfile.write("Test 2 and 3 and running at the same time:\n")
            result_2 = result_3 = True
            stat_pop = stdout_df[hint_support.Stdout.stat_pop]
            # infected = stdout_df[Stdout.infected]
            duration = param_obj[ConfigKeys.Simulation_Duration]

            contagion_list = []
            prob_list = []

            non_test_cols = []
            for non_test_group in non_test_groups:
                non_test_cols += [c for c in property_df.columns if non_test_group in c]
            non_test_cols = list(set(non_test_cols))
            # get all the column names that only contains the test group, which should be property_df.columns - non_test_groups
            test_only_cols = [c for c in property_df.columns if c not in non_test_cols]

            # test data for test groups
            infected_test = property_df[[c for c in test_only_cols if hint_support.channels[0] in c]]
            population_test = property_df[[c for c in test_only_cols if hint_support.channels[2] in c]]
            new_infection_test = property_df[[c for c in test_only_cols if hint_support.channels[1] in c]]

            expected_new_infection_list = []
            failed_count = 0
            for t in range(duration - 1):
                calculated_contagion = 0
                for col in infected_test:
                    # calculate infectivity of seed group
                    # nomalized with total population
                    infectivity_seed = base_infectivity * infected_test[col].iloc[t] / stat_pop[t]
                    infectivity_mod = 1
                    for i in range(len(test_groups)):
                        test_group = test_groups[i]
                        if test_group in col:
                            for property_obj in property_list:
                                property_values = property_obj[DemographicsKeys.PropertyKeys.Values]
                                if test_group in property_values:
                                    transmission_matrix = property_obj[DemographicsKeys.PropertyKeys.Matrix]
                                    infectivity_mod *= transmission_matrix[property_values.index(test_group)][
                                                       property_values.index(test_group)]

                    # calculate contagion of susceptible group
                    calculated_contagion += infectivity_seed * infectivity_mod

                # round the calculated value to 6 Decimal numbers
                calculated_contagion = round(calculated_contagion, 6)

                # get contagion of susceptible group from stdout
                # group_id is the first element in all group_ids
                group_id = 0
                # actual contagion is for next time step
                actual_contagion = stdout_df[(stdout_df[ConfigKeys.Simulation_Timestep] == t + 1) &
                                             (stdout_df[hint_support.Stdout.group_id] == group_id)][hint_support.Stdout.contagion].values[0]
                contagion_list.append([actual_contagion, calculated_contagion])
                if math.fabs(calculated_contagion - actual_contagion) > 5e-2 * calculated_contagion:
                    result_2 = success = False
                    outfile.write("    BAD: at time step {0}, for group {1} id {2}, the total contagion is {3}, "
                                  "expected {4}.\n".format(t + 1, test_group, group_id, actual_contagion,
                                                           calculated_contagion
                    ))

                # calculate infection probability based on contagion
                calculated_prob = 1.0 - math.exp(-1 * calculated_contagion * param_obj[ConfigKeys.Simulation_Timestep])
                # round the calculated value to 6 Decimal numbers
                calculated_prob = round(calculated_prob, 6)
                # get infection probability of susceptible group from stdout_df
                actual_prob = stdout_df[(stdout_df[ConfigKeys.Simulation_Timestep] == t + 1) &
                                             (stdout_df[hint_support.Stdout.group_id] == group_id)][hint_support.Stdout.prob].values[0]
                prob_list.append([actual_prob, calculated_prob])
                if math.fabs(calculated_prob - actual_prob) > 5e-2 * calculated_prob:
                    result_2 = success = False
                    outfile.write("    BAD: at time step {0}, for group {1} id {2}, the infected probability is "
                                  "{3}, expected {4}.\n".format( t + 1, test_group, group_id, actual_prob,
                                                                 calculated_prob
                    ))

                # calculate expected new infection for test group
                susceptible_population = population_test.iloc[t, 0] - infected_test.iloc[t,0]
                expected_new_infection = susceptible_population * calculated_prob
                expected_new_infection_list.append(expected_new_infection)
                actual_new_infection = new_infection_test.iloc[t + 1, 0]
                with open("DEBUG_binomial_test_{}.txt".format(test_group), 'w') as file:
                    if expected_new_infection < 5 or susceptible_population * (1 - calculated_prob) < 5:
                        binom_pmf = stats.binom.pmf(k=actual_new_infection, n=susceptible_population, p=calculated_prob)
                        if binom_pmf < 1e-3:
                            failed_count += 1
                            outfile.write("WARNING: at timestep {0}, new infections for {1} group is {2}, expected "
                                          " = {3}, calculated binomial pmf is {4}.\n"
                                          "".format(t + 1, test_group, actual_new_infection,
                                                    expected_new_infection, binom_pmf))

                    elif not sft.test_binomial_99ci(num_success=actual_new_infection, num_trials=susceptible_population,
                                               prob=calculated_prob, report_file=file,
                                               category="new infections for {0} at time {1}".format(test_group, t + 1)):
                        failed_count += 1
                        standard_deviation = math.sqrt(
                            calculated_prob * (1 - calculated_prob) * susceptible_population)
                        # 99% confidence interval
                        lower_bound = expected_new_infection - 3 * standard_deviation
                        upper_bound = expected_new_infection + 3 * standard_deviation
                        outfile.write("WARNING: at timestep {0}, new infections for {1} group is {2}, expected "
                                      "within 99% binomial interval ({3}, {4}) with mean = {5}\n".format(t + 1, test_group,
                                                                                      actual_new_infection,
                                                                                      lower_bound, upper_bound,
                                                                                      expected_new_infection))
            # make sure other groups has no new infections after outbreak
            new_infection_non_test = property_df[[c for c in non_test_cols if hint_support.channels[1] in c]]
            message_template = "{0}: total new infection after outbreak for {1} is {2}, expected 0.\n"
            for col in new_infection_non_test:
                total_new_infection = new_infection_non_test.loc[1:, col].sum()
                if total_new_infection != 0:
                    success = False
                    outfile.write(message_template.format("BAD", col, total_new_infection))
                else:
                    outfile.write(message_template.format("GOOD", col, total_new_infection))

            # plotting
            # get the group name for the test group
            group_name = test_only_cols[0].replace(":", " ", 1).split()[-1]
            # ":" is not allowed in filename, replace it with "-" to avoid OSError
            group_name_modified = group_name.replace(":", "-")
            sft.plot_data(np.array(contagion_list)[:, 0], np.array(contagion_list)[:, 1], label1='contagion from logging', label2="calculated contagion",
                              title="{}\ncontagion".format(group_name), xlabel='day',ylabel='contagion',category="contagion_{}".format(group_name_modified),
                              line=True, alpha=0.5, overlap=True)
            sft.plot_data(np.array(prob_list)[:, 0], np.array(prob_list)[:, 1], label1='probability from logging', label2="calculated probability",
                              title="{}\nprobability".format(group_name), xlabel='day',ylabel='probability',category="probability_{}".format(group_name_modified),
                              line=True, alpha=0.5, overlap=True)

            sft.plot_data(new_infection_test.iloc[1:, 0].tolist(), expected_new_infection_list,
                              label1='from property report',
                              label2="calculated data",
                              title="{}\nnew infections".format(group_name),
                              xlabel='day', ylabel='new_infections',
                              category="new_infections_{}".format(group_name_modified),
                              line=False, alpha=0.5, overlap=True, sort=False)

            message_template = "{0}: binomial test for {1} failed {2} times within {3} total timesteps, which is " \
                               "{4}% fail rate, test is {5}.\n"
            if failed_count / duration > 5e-2:
                result_3 = success = False
                outfile.write(message_template.format("BAD", new_infection_test.columns.values, failed_count,
                                                      duration, (failed_count / duration) * 100, "failed"))
            else:
                outfile.write(message_template.format("GOOD", new_infection_test.columns.values, failed_count,
                                                      duration, (failed_count / duration) * 100, "passed"))

            outfile.write("Test 2: result is: {}.\n".format(result_2))
            outfile.write("Test 3: result is: {}.\n".format(result_3))

        outfile.write(sft.format_success_msg(success))
    if debug:
        print(sft.format_success_msg(success))
    return success


def application( output_folder="output", stdout_filename="test.txt",
                 property_report_name="PropertyReport.json",
                 config_filename="config.json", campaign_filename="campaign.json",
                 report_name=sft.sft_output_filename,
                 debug=False):
    if debug:
        print( "output_folder: " + output_folder )
        print( "stdout_filename: " + stdout_filename+ "\n" )
        print( "property_report_name: " + property_report_name+ "\n" )
        print( "config_filename: " + config_filename + "\n" )
        print( "campaign_filename: " + campaign_filename + "\n" )
        print( "report_name: " + report_name + "\n" )
        print( "debug: " + str(debug) + "\n" )

    sft.wait_for_done(stdout_filename)

    config_obj = hint_support.load_config_parameters(config_filename, hint_support.config_keys, debug)
    campaign_obj = hint_support.load_campaign_file(campaign_filename, debug)
    stdout_df = hint_support.parse_stdout_file(stdout_filename, config_obj[ConfigKeys.Simulation_Timestep], debug)
    demo_path = "Assets" if stdout_filename == "StdOut.txt" else ""
    property_list = hint_support.load_demo_overlay_file(
        config_obj[ConfigKeys.Demographics_Filenames][1], demo_path, debug)
    property_keys = hint_support.build_channel_string_for_property(property_list, hint_support.channels, debug)
    property_df = hint_support.parse_property_report_json(property_report_name, output_folder, property_keys, debug)
    create_report_file(config_obj, campaign_obj, stdout_df, property_df, property_list, report_name, debug)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt)")
    parser.add_argument('-p', '--propertyreport', default="PropertyReport.json", help="Property report to load (PropertyReport.json)")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-C', '--campaign', default="campaign.json", help="campaign name to load (campaign.json)")
    parser.add_argument('-r', '--reportname', default=sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', help="debug flag", action='store_true')
    args = parser.parse_args()

    application(output_folder=args.output, stdout_filename=args.stdout,
                property_report_name=args.propertyreport,
                config_filename=args.config, campaign_filename=args.campaign,
                report_name=args.reportname, debug=args.debug)

