#!/usr/bin/python

import dtk_test.dtk_sft as sft
import math
from dtk_test.dtk_General_Support import ConfigKeys, CampaignKeys, DemographicsKeys, InsetKeys
import numpy as np
from scipy import stats
import dtk_test.dtk_HINT_Support as hint_support

"""
This SFT tests single property Heterogeneous Intranode Transmission within the same property group. 

Outbreak intervention is delivered to High_Group at time step 0 with coverage = 0.05.

This SFT first checks if we have the correct test setup. Then it looks at the total contagion pool for each group at 
the end of each time step. Contagion is calculated as base_infectivity * # of infected individual who shed into
this contagion pool * transmission multiplier. The Transmission multiplier are read from demo_overlay file and can be 
any positive floating number. Contagion need to be normalized with total population (transmission 
scaling). Then it checks the infection probability based on the contagion for each group at every time step.
Most importantly, it checks if the # of new infections is within 99% binomial confident interval or the pmf of 
corresponding binomial distribution is within expected value at every time step for both groups.

If any of the actual contagion and infection probability is off by 5% or more compared to the calculated value, the test 
will fail. If the statistical test for new infections fails more than 5% of the simulation duration, the test will fail.

Test values for contagion and infection probability are collect from stdout.text, they are at individual level. 
New infections data are from PropertyReport.json and at the group level.
"""

channels = [InsetKeys.ChannelsKeys.Infected,
            InsetKeys.ChannelsKeys.New_Infections,
            InsetKeys.ChannelsKeys.Statistical_Population]


config_keys = [ConfigKeys.Config_Name, ConfigKeys.Simulation_Timestep,
               ConfigKeys.Simulation_Duration, ConfigKeys.Base_Infectivity,
               ConfigKeys.Run_Number, ConfigKeys.Demographics_Filenames,
               ConfigKeys.Enable_Heterogeneous_Intranode_Transmission]


matches = ["Update(): Time: ",
           "total contagion = ",
           "group_id = "
           ]


class Stdout:
    prob = "prob="
    stat_pop = "StatPop: "
    infected = "Infected: "
    contagion = "Contagion"
    group_id = "group_id"


def create_report_file(param_obj, campaign_obj, stdout_df, property_df, property_obj, report_name, debug):
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
            coverage = 0.05
            if not all(x == coverage for x in campaign_obj[CampaignKeys.Demographic_Coverage]):
                success = False
                outfile.write("BAD: {0} should be {1}, please check campaign.json.\n".format(
                    CampaignKeys.Demographic_Coverage, coverage))

            expected_property_restrictions = []
            # seed_groups = list(filter(lambda value: 'seed' in value.lower(),
            #                           property_obj[DemographicsKeys.PropertyKeys.Values]))
            # easier to read
            property_values = property_obj[DemographicsKeys.PropertyKeys.Values]
            for value in property_values:
                expected_property_restrictions.append(["{0}:{1}".format(
                    property_obj[DemographicsKeys.PropertyKeys.Property], value)])
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
            stat_pop = stdout_df[Stdout.stat_pop]
            infected = stdout_df[Stdout.infected]
            duration = param_obj[ConfigKeys.Simulation_Duration]
            transmission_matrix = property_obj[DemographicsKeys.PropertyKeys.Matrix]
            for group in property_values:
                contagion_list = []
                prob_list = []
                cols = [c for c in property_df.columns if group in c]

                # test data from property report
                infected = property_df[[c for c in cols if channels[0] in c]]
                new_infection = property_df[[c for c in cols if channels[1] in c]]
                population = property_df[[c for c in cols if channels[2] in c]]

                expected_new_infection_list = []
                failed_count = 0
                for t in range(duration - 1):
                    # calculate infectivity
                    # infectivity = base_infectivity * infected.iloc[t, 0] / population.iloc[t, 0]
                    # normalized with total population
                    infectivity = base_infectivity * infected.iloc[t, 0] / stat_pop[t]

                    # calculate contagion
                    calculated_contagion = infectivity * transmission_matrix[property_values.index(group)][
                        property_values.index(group)]

                    # get contagion of this group from stdout_df
                    group_id = property_values.index(group)
                    # actual contagion is for next time step
                    actual_contagion = stdout_df[(stdout_df[ConfigKeys.Simulation_Timestep] == t + 1) &
                                                 (stdout_df[Stdout.group_id] == group_id)][Stdout.contagion].values[0]
                    contagion_list.append([actual_contagion, calculated_contagion])
                    if math.fabs(calculated_contagion - actual_contagion) > 5e-2 * calculated_contagion:
                        result_2 = success = False
                        outfile.write("    BAD: at time step {0}, for group {1} id {2}, the total contagion is {3}, "
                                      "expected {4}.\n".format(t + 1, group, group_id, actual_contagion,
                                                               calculated_contagion
                        ))

                    # calculate infection probability based on contagion
                    calculated_prob = 1.0 - math.exp(-1 * calculated_contagion * param_obj[ConfigKeys.Simulation_Timestep])
                    # get infection probability of susceptible group from stdout_df
                    actual_prob = stdout_df[(stdout_df[ConfigKeys.Simulation_Timestep] == t + 1) &
                                                 (stdout_df[Stdout.group_id] == group_id)][Stdout.prob].values[0]
                    prob_list.append([actual_prob, calculated_prob])
                    if math.fabs(calculated_prob - actual_prob) > 5e-2 * calculated_prob:
                        result_2 = success = False
                        outfile.write("    BAD: at time step {0}, for group {1} id {2}, the infected probability is "
                                      "{3}, expected {4}.\n".format(t + 1, group, group_id, actual_prob,
                                                                    calculated_prob
                        ))

                    # calculate expected new infection for this group
                    susceptible_population = population.iloc[t, 0] - infected.iloc[t, 0]
                    expected_new_infection = susceptible_population * calculated_prob
                    expected_new_infection_list.append(expected_new_infection)
                    actual_new_infection = new_infection.iloc[t + 1, 0]
                    with open("DEBUG_binomial_test_{}.txt".format(group), 'w') as file:
                        if expected_new_infection < 5 or susceptible_population * (1 - calculated_prob) < 5:
                            binom_pmf = stats.binom.pmf(k=actual_new_infection, n=susceptible_population, p=calculated_prob)
                            if binom_pmf < 1e-3:
                                failed_count += 1
                                outfile.write("WARNING: at timestep {0}, new infections for {1} group is {2}, expected "
                                              " = {3}, calculated binomial pmf is {4}.\n"
                                              "".format(t + 1, group, actual_new_infection,
                                                        expected_new_infection, binom_pmf))

                        elif not sft.test_binomial_99ci(num_success=actual_new_infection, num_trials=susceptible_population,
                                                   prob=calculated_prob, report_file=file,
                                                   category="new infections for {0} at time {1}".format(group, t + 1)):
                            failed_count += 1
                            standard_deviation = math.sqrt(
                                calculated_prob * (1 - calculated_prob) * susceptible_population)
                            # 99% confidence interval
                            lower_bound = expected_new_infection - 3 * standard_deviation
                            upper_bound = expected_new_infection + 3 * standard_deviation
                            outfile.write("WARNING: at timestep {0}, new infections for {1} group is {2}, expected "
                                          "within 99% binomial interval ({3}, {4}) with mean = {5}\n".format(t + 1, group,
                                                                                          actual_new_infection,
                                                                                          lower_bound, upper_bound,
                                                                                          expected_new_infection))


                sft.plot_data(np.array(contagion_list)[:, 0], np.array(contagion_list)[:, 1], label1='contagion from logging', label2="calculated contagion",
                                  title="actual vs. expected contagion", xlabel='day',ylabel='contagion',category="contagion_{}".format(group),
                                  line=True, alpha=0.5, overlap=True)
                sft.plot_data(np.array(prob_list)[:, 0], np.array(prob_list)[:, 1], label1='probability from logging', label2="calculated probability",
                                  title="actual vs. expected probability", xlabel='day',ylabel='probability',category="probability_{}".format(group),
                                  line=True, alpha=0.5, overlap=True)

                # skip the first time step(outbreak start day) for new infection channel
                sft.plot_data(new_infection.iloc[1:, 0].tolist(), expected_new_infection_list,
                                  label1='from property report',
                                  label2="calculated data",
                                  title="new infections for {}".format(group),
                                  xlabel='day', ylabel='new_infections',
                                  category="new_infections_{}".format(group),
                                  line=False, alpha=0.5, overlap=True, sort=False)

                message_template = "{0}: binomial test for {1} failed {2} times within {3} total timesteps, which is " \
                                   "{4}% fail rate, test is {5}.\n"
                if failed_count / duration > 5e-2:
                    result_3 = success = False
                    outfile.write(message_template.format("BAD", new_infection.columns.values, failed_count,
                                                          duration, (failed_count / duration) * 100, "failed"))
                else:
                    outfile.write(message_template.format("GOOD", new_infection.columns.values, failed_count,
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
                 debug=True):
    if debug:
        print( "output_folder: " + output_folder )
        print( "stdout_filename: " + stdout_filename+ "\n" )
        print( "property_report_name: " + property_report_name+ "\n" )
        print( "config_filename: " + config_filename + "\n" )
        print( "campaign_filename: " + campaign_filename + "\n" )
        print( "report_name: " + report_name + "\n" )
        print( "debug: " + str(debug) + "\n" )

    sft.wait_for_done(stdout_filename)

    config_obj = hint_support.load_config_parameters(config_filename, config_keys, debug)
    campaign_obj = hint_support.load_campaign_file(campaign_filename, debug)
    stdout_df = hint_support.parse_stdout_file(stdout_filename, config_obj[ConfigKeys.Simulation_Timestep], debug)
    demo_path = "Assets" if stdout_filename == "StdOut.txt" else ""
    property_list = hint_support.load_demo_overlay_file(
        config_obj[ConfigKeys.Demographics_Filenames][1], demo_path, debug)
    property_keys = hint_support.build_channel_string_for_property(property_list, channels, debug)
    property_df = hint_support.parse_property_report_json(property_report_name, output_folder, property_keys, debug)
    property_obj = property_list[0] # this test only has one property object
    create_report_file(config_obj, campaign_obj, stdout_df, property_df, property_obj, report_name, debug)


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

