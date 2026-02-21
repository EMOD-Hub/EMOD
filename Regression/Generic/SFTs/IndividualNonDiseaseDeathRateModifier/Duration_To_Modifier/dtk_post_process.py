import os

if __name__ == '__main__':
    from pathlib import Path
    import sys

    os.chdir(str(Path(sys.argv[0]).parent))
    sys.path.append(str(Path('../../../shared_embedded_py_scripts').resolve().absolute()))
    sys.path.append(str(Path('../../../shared_embedded_py_scripts/dev').resolve().absolute()))

from dtk_test.dtk_INDDRM_Support import INDDRMTest, Channels
from dtk_test.dtk_sft_class import arg_parser
import dtk_test.dtk_sft as dtk_sft
import matplotlib.pyplot as plt
from dtk_test.dtk_General_Support import ConfigKeys
import numpy as np
import json
from scipy.stats import binom

"""
IndividualNonDiseaseDeathRateModifier
This test is testing the new campaign intervention: IndividualNonDiseaseDeathRateModifier

Duration_To_Modifier of the IndividualNonDiseaseDeathRateModifier intervention during the usage duration distribution 
in the campaign.json files are set to be the following:
"Start_Day": 50,
...
"Intervention_Config": {
                    "class": "IndividualNonDiseaseDeathRateModifier",
                    "Cost_To_Consumer": 1,
                    "Duration_To_Modifier" : {
                        "Times" : [  0.0, 100.0, 150, 200, 300.0, 400],
                        "Values": [ 1.0,   0.0,  0.0,  30, 20.0,  10.0]
                    },
                    "Expiration_Duration_Distribution": "CONSTANT_DISTRIBUTION",
                    "Expiration_Duration_Constant": 500,
                    "Expiration_Event": "Stopped_Death_Modification"
                }
    So the NonDiseaseDeathRate rate is set in Duration_To_Modifier during the usage duration.


Here are the things that the test is doing:
    Expiration_Duration_Distribution is set CONSTANT_DISTRIBUTION with 500 as the Duration_Constant, so the intervention
    will last longer than the last value in Times list. Birth and Disease Death are turn off in this test.
    
    Data collected: 
        1. IndividualNonDiseaseDeathRateModifier: Times and Values from Duration_To_Modifier, Start_Day and Expiration_Duration_Constant
        2. MortalityDistribution from demographics
        3. run simulation and get total Statistical Population from InsetChat.json
    
    Test Steps:
        1. Calculate the modifier based on the "Times" and "Values" pair.
           Plot the modifier per time step.
        2. Calculate the mortality rate based on "ResultScaleFactor" and "ResultValues" in demographics.
        3. Calculate actual mortality rate based on result from previous steps for these periods:
           a. from simulation start day to intervention start day.
           b. from the first day in Times list until the last day in Times list.
           c. from the last day in Times list until the expiration day of the intervention.
           d. from the expiration day to the end of the simulation.
           Plot the real mortality rates.
        4. Calculate deaths per day
        5. Calculate the 95% Binomial confidence intervals based on actual death rate and population.
        6. Test if the Deaths count fall within the Binomial ci for each time step:
           a. check for all time step in the simulation(include before and after the simulation)
           b. check only the time step with in the intervention duration.
           c. plot the deaths and Binomial ci for both cases
"""


class DurationToModifierTest(INDDRMTest):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.params_keys = [ConfigKeys.Simulation_Duration,
                            ConfigKeys.Demographics_Filenames]

    def load_config(self):
        super(DurationToModifierTest, self).load_config(params_keys=self.params_keys)

    def parse_campaign(self):
        campaign_obj = {}
        # Consider: If you are using event 0 for something interesting, you could load that here too.
        with open(self.campaign_filename) as infile:
            campaign_json = json.load(infile)
            event_intervention = campaign_json["Events"][0]
            intervention_config = event_intervention['Event_Coordinator_Config']['Intervention_Config']
            dist, list_param, event = self.get_expiration_duration_dist_param(intervention_config)
            campaign_obj['duration'] = {}
            campaign_obj['duration']['distribution'] = dist
            campaign_obj['duration']['constant'] = list_param[0]
            campaign_obj['expiration_event'] = event
            campaign_obj['start_day'] = event_intervention['Start_Day']
            campaign_obj['times'] = intervention_config['Duration_To_Modifier']['Times']
            campaign_obj['values'] = intervention_config['Duration_To_Modifier']['Values']
        return campaign_obj

    def calculate_modifier(self, campaign_obj):
        campaign_start_time = campaign_obj['start_day']
        duration_to_modifier_times = campaign_obj['times']
        duration_to_modifier_values = campaign_obj['values']

        expiration_duration_constant = campaign_obj['duration']['constant']
        simulation_total_time = self.params[ConfigKeys.Simulation_Duration]

        base_modifier = 1

        time_step_mapped = np.arange(duration_to_modifier_times[-1])
        time_step_between_map_expiration = expiration_duration_constant - duration_to_modifier_times[-1]
        time_step_remain = int(simulation_total_time - (campaign_start_time + expiration_duration_constant))
        prob_tmp = np.interp(time_step_mapped, duration_to_modifier_times, duration_to_modifier_values)
        modifier = [base_modifier] * campaign_start_time
        modifier.extend(prob_tmp)
        modifier.extend([duration_to_modifier_values[-1]] * time_step_between_map_expiration)
        modifier.extend([base_modifier] * time_step_remain)
        return modifier

    @staticmethod
    def plot_modifier(modifier):
        dtk_sft.plot_data(modifier, dist2=None, label1="Death Rate Modifier", label2=None,
                                   title='Death Rate Modifier during the simulation',
                                   xlabel='Time step', ylabel="Modifier",
                                   category='Death_Rate_Modifier', show=True, line=True, alpha=0.8,
                                   overlap=False)

    @staticmethod
    def plot_death_rate(death_rate):
        dtk_sft.plot_data(death_rate, dist2=None, label1="Death Rate applied with Modifier", label2=None,
                                   title='Actual Death Rate during the simulation',
                                   xlabel='Time step', ylabel="Death Rate",
                                   category='Actual_Death_Rate', show=True, line=True, alpha=0.8,
                                   overlap=False)

    def get_death_rate(self):
        with open(self.params[ConfigKeys.Demographics_Filenames][0]) as infile:
            death_rate_params = json.load(infile)['Defaults']['IndividualAttributes']['MortalityDistribution']
        death_rate = (death_rate_params['ResultScaleFactor']) * (death_rate_params['ResultValues'][0][0])
        return death_rate

    @staticmethod
    def calculate_new_death(total_pop):
        new_deaths = [0]
        for i in range(1, len(total_pop)):
            new_deaths.append(total_pop[i - 1] - total_pop[i])
        return new_deaths

    @staticmethod
    def test_new_deaths_with_bounds(ci_lower, ci_upper, new_deaths):
        failed_count = 0
        for lower, upper, deaths in zip(ci_lower, ci_upper, new_deaths):
            if deaths > upper or deaths < lower:
                failed_count += 1
        return failed_count

    @staticmethod
    def plot_deaths_per_time_step(ci_lower, ci_upper, new_deaths, off_set, title):
        x = np.arange(off_set, len(new_deaths) + off_set)
        fig = plt.figure()
        ax = fig.add_axes([0.12, 0.15, 0.76, 0.76])
        ax.set_xticks(np.arange(off_set, len(new_deaths) + off_set, (len(new_deaths) + off_set) // 10))
        ax.set_yticks(np.arange(-5, max(new_deaths) + 5, 5))
        plt.plot(x, ci_lower)
        plt.plot(x, ci_upper)
        plt.plot(x, new_deaths, 'o', markersize=2)
        plt.grid(axis='y')
        plt.title(f'Binomial 95% CI of {title}')
        plt.xlabel('Time step')
        plt.ylabel('New deaths')
        plt.legend(['CI lower bound', 'CI upper bound', 'Observed'], loc='upper left')
        fig.savefig(f'{title}.png')
        if dtk_sft.check_for_plotting():
            plt.show()
        plt.close(fig)

    @staticmethod
    def calculate_binomial_ci(total_pop, death_rate):
        ci_lower = []
        ci_upper = []
        if len(total_pop) == len(death_rate):
            for i in range(len(total_pop)):
                if death_rate[i] == 0 or total_pop[i] == 0:
                    ci_lower.append(0)
                    ci_upper.append(0)
                else:
                    ci = binom.interval(confidence=0.95, n=total_pop[i], p=death_rate[i])
                    ci_lower.append(ci[0])
                    ci_upper.append(ci[1] + 1)
        return ci_lower, ci_upper

    def test_and_plot_new_deaths_with_bounds(self, ci_lower, ci_upper, new_deaths, off_set, title):
        self.plot_deaths_per_time_step(ci_lower, ci_upper, new_deaths, off_set, title)
        failed_count = self.test_new_deaths_with_bounds(ci_lower, ci_upper, new_deaths)

        percentage_failed_test = failed_count / len(new_deaths)
        if percentage_failed_test > 0.1:
            self.succeed = False
            self.msg.append(
                f"BAD: Number of times deaths out of CI: {failed_count} out of {len(new_deaths)} time step. "
                f"Percentage of times pregnancy out of CI: {percentage_failed_test}%. \n")
        else:
            self.msg.append(
                f"GOOD: Number of times deaths out of CI: {failed_count} out of {len(new_deaths)} time step. "
                f"Percentage of times pregnancy in CI: {1 - percentage_failed_test}%. \n")

    # overwrite the test method
    def test(self):
        self.parse_json_report(channel_names=[Channels.StatPop])
        inset_chart_df = self.json_report.df

        self.msg.append(f"Testing {self.event_report_name}: \n")
        # calculate death rate modifier from campaign and plot it
        campaign_obj = self.parse_campaign()
        modifier = self.calculate_modifier(campaign_obj)
        self.plot_modifier(modifier)
        # calculate read death rate
        death_rate_without_intervention = self.get_death_rate()
        death_rate = [x * death_rate_without_intervention for x in modifier]
        self.plot_death_rate(death_rate)
        # calculate new death per day
        total_pop = list(inset_chart_df[Channels.StatPop])
        new_deaths = self.calculate_new_death(total_pop)

        ci_lower, ci_upper = self.calculate_binomial_ci(total_pop, death_rate)
        # test for the whole simulation duration
        self.test_and_plot_new_deaths_with_bounds(ci_lower, ci_upper, new_deaths, off_set=0, title="Death-all_simulation_duration")
        # test for only this intervention period
        intervention_start_day = int(campaign_obj['start_day'])
        intervention_duration = int(campaign_obj['duration']['constant'])
        ci_lower = ci_lower[intervention_start_day:intervention_start_day + intervention_duration]
        ci_upper = ci_upper[intervention_start_day:intervention_start_day + intervention_duration]
        new_deaths = new_deaths[intervention_start_day:intervention_start_day + intervention_duration]
        self.test_and_plot_new_deaths_with_bounds(ci_lower, ci_upper, new_deaths, off_set=intervention_start_day,
                                                  title="Death-intervention")


def application(output_folder="output", my_arg=None):
    if not my_arg:
        my_sft = DurationToModifierTest()
    else:
        my_sft = DurationToModifierTest(
            output=my_arg.output, stdout=my_arg.stdout, json_report=my_arg.json_report, event_csv=my_arg.event_csv,
            config=my_arg.config, campaign=my_arg.campaign, report_name=my_arg.report_name, debug=my_arg.debug)
    my_sft.run()


if __name__ == "__main__":
    # execute only if run as a script
    my_arg = arg_parser()
    application(my_arg=my_arg)
