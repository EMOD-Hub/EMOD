import os

if __name__ == '__main__':
    from pathlib import Path
    import sys

    os.chdir(str(Path(sys.argv[0]).parent))
    sys.path.append(str(Path('../../../shared_embedded_py_scripts').resolve().absolute()))
    sys.path.append(str(Path('../../../shared_embedded_py_scripts/dev').resolve().absolute()))

from dtk_test.dtk_sft_class import SFT, arg_parser
import dtk_test.dtk_sft as dtk_sft
from dtk_test.dtk_General_Support import ConfigKeys


"""
This test is testing the InsetChat.json channels that are related to pregnancies and births.

In the test the "Birth_Rate_Dependence" is set to "INDIVIDUAL_PREGNANCIES_BY_AGE_AND_YEAR" and there is a 
FemaleContraceptive intervention started at day 100 and last for 100 days for 50% efficacy. 

Age distribution in demographics file is configured to have everyone age from 20 to 40 so there will be no women enter 
or exit the possible mother status due to aging. 

New births should match New Pregnancies 9 months/280 days/40 weeks ago.
Currently Pregnant should match: previous Currently Pregnant +  previous new_pregnancies - previous New Births
Possible mothers should not change.
    
    Data collected: 
        1. InsetChat.json
"""


class Channels:
    statPop = "Statistical Population"
    births = "Births"
    currentlyPregnant = "Currently Pregnant"
    newPregnancies = "New Pregnancies"
    possibleMothers = "Possible Mothers"


class InsetChartTest(SFT):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.params_keys = [ConfigKeys.Simulation_Duration,
                            ConfigKeys.Demographics_Filenames]

    def load_config(self):
        super(InsetChartTest, self).load_config(params_keys=self.params_keys)

    @staticmethod
    def calculate_new_births(births):
        new_births = [births[0]]
        for i in range(1, len(births)):
            new_births.append(births[i] - births[i - 1])
        return new_births

    def write_result(self, fail_count, test_channel_name, expected_channel_names):
        if fail_count:
            self.msg.append(
                f"BAD: there are {len(fail_count)} time steps that {test_channel_name} doesn't match "
                f"information from {expected_channel_names}.\n")
            if len(fail_count) <= 10:
                self.msg.append(f"These are the time steps that the test failed: {fail_count}.\n")
        else:
            self.msg.append(f"GOOD: {test_channel_name} does match "
                            f"information from {expected_channel_names}.\n")

    # overwrite the test method
    def test(self):
        self.parse_json_report(channel_names=[Channels.statPop, Channels.births, Channels.possibleMothers,
                                              Channels.newPregnancies, Channels.currentlyPregnant])
        inset_chart_df = self.json_report.df

        self.msg.append(f"Testing {self.json_report}: \n")
        # calculate new births per day
        births = list(inset_chart_df[Channels.births])
        new_births = self.calculate_new_births(births)

        # new_births should match new_pregnancies 9 months/280 days ago
        new_pregnancies = list(inset_chart_df[Channels.newPregnancies])
        # predict new births based on new pregnancies
        predict_new_births = [0] * 280 + new_pregnancies[:-280]
        fail_count = []
        for i in range(280, len(new_births)):
            if predict_new_births[i] != new_births[i]:
                fail_count.append(i)
                self.success = False
        self.write_result(fail_count, Channels.births, Channels.newPregnancies)

        dtk_sft.plot_data(new_births, predict_new_births, label1="new births", label2="new pregnancies(off_set 280 days)",
                                   title='New births vs. new pregnancies(off_set 280 days)', xlabel='time step', ylabel='count',
                                   category='New_births_vs_new_pregnancies', show=True, line=False, alpha=1, overlap=False)

        # Currently Pregnant should match  previous Currently Pregnant + new_pregnancies - New Births
        currently_pregnancies = list(inset_chart_df[Channels.currentlyPregnant])
        expected_currently_pregnancies = [currently_pregnancies[0]]
        fail_count = []
        for i in range(1, len(currently_pregnancies)):
            expected_currently_pregnancies.append(currently_pregnancies[i - 1] + new_pregnancies[i - 1] - new_births[i - 1])
            if currently_pregnancies[i] != expected_currently_pregnancies[-1]:
                self.success = False
                fail_count.append(i)
        self.write_result(fail_count, Channels.currentlyPregnant, [Channels.newPregnancies, Channels.births])

        dtk_sft.plot_data(currently_pregnancies, expected_currently_pregnancies, label1="currently_pregnancies",
                                   label2="pre_cur + new_pregnancies - new births",
                                   title='currently_pregnancies vs. expected_currently_pregnancies',
                                   xlabel='time step', ylabel='pregnancies',
                                   category='currently_pregnancies_vs_expected_currently_pregnancies',
                                   show=True, line=False, alpha=1, overlap=True)

        # Possible Mothers should remain the same, it only depends on gender and age restriction. 14 < age < 45.
        possible_mothers = list(inset_chart_df[Channels.possibleMothers])
        if len(set(possible_mothers)) != 1:
            self.msg.append(f"BAD: {Channels.possibleMothers} should remain the same value during the whole simulations.\n")
            self.success = False
        else:
            self.msg.append(f"GOOD: {Channels.possibleMothers} remains the same value during the whole simulations.\n")
        dtk_sft.plot_data(possible_mothers, dist2=None,
                                   label1="possible_mothers",
                                   label2=None,
                                   title='possible_mothers',
                                   xlabel='time step', ylabel='possible_mothers',
                                   category='possible_mothers',
                                   show=True, line=False, alpha=1, overlap=False)


def application(output_folder="output", my_arg=None):
    if not my_arg:
        my_sft = InsetChartTest()
    else:
        my_sft = InsetChartTest(
            output=my_arg.output, stdout=my_arg.stdout, json_report=my_arg.json_report, event_csv=my_arg.event_csv,
            config=my_arg.config, campaign=my_arg.campaign, report_name=my_arg.report_name, debug=my_arg.debug)
    my_sft.run()


if __name__ == "__main__":
    # execute only if run as a script
    my_arg = arg_parser()
    application(my_arg=my_arg)
