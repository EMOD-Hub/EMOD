#!/usr/bin/python
import os
import matplotlib.pyplot as plt
import numpy as np
import math

if __name__ == '__main__':
    from pathlib import Path
    import sys
    os.chdir(str(Path(sys.argv[0]).parent))
    sys.path.append( str(Path('../../../../shared_embedded_py_scripts/').resolve().absolute()) )

from dtk_test.dtk_sft_class import SFT, arg_parser
from dtk_test.dtk_General_Support import ConfigKeys
from dtk_test.dtk_OutputFile import CsvOutput

"""
Testing tickets:
HIV_Ongoing: RelationshipConsummated.csv - Add more filtering & Transmission Info
https://github.com/InstituteforDiseaseModeling/DtkTrunk/issues/4492

This test is testing the transmission information in the RelationshipConsummated.csv including:
    Risk_Multiplier 
        --with CoitalActRiskFactors intervention and Condom_Transmission_Blocking_Probability
    Transmission_Multiplier 
        --based on HIV stages: Acute, Latent and AIDS
    Acquisition_Multiplier 
        --with MaleCircumcision intervention and Male_To_Female_Relative_Infectivity_Multipliers
    Infection_Was_Transmitted
"""


class ReportRelationshipStartTest(SFT):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        # self.json_report_name = "InsetChart.json"
        self.params_keys = [ConfigKeys.Report_Event_Recorder,
                            ConfigKeys.Report_Coital_Acts,
                            ConfigKeys.STI_Coinfection_Acquisition_Multiplier,
                            ConfigKeys.STI_Coinfection_Transmission_Multiplier,
                            ConfigKeys.Base_Infectivity,
                            ConfigKeys.Acute_Stage_Infectivity_Multiplier,
                            ConfigKeys.AIDS_Stage_Infectivity_Multiplier,
                            ConfigKeys.Male_To_Female_Relative_Infectivity_Ages,
                            ConfigKeys.Male_To_Female_Relative_Infectivity_Multipliers,
                            ConfigKeys.Condom_Transmission_Blocking_Probability,
                            ConfigKeys.Base_Year,
                            ConfigKeys.Simulation_Duration]

    def load_config(self):
        super(ReportRelationshipStartTest, self).load_config(params_keys=self.params_keys)

    @staticmethod
    def find_infectivity_multiplier(m_to_f_age, m_to_f_multiplier, b_age):
        if b_age < m_to_f_age[0]:
            return m_to_f_multiplier[0]
        if b_age > m_to_f_age[-1]:
            return m_to_f_multiplier[-1]
        i = 0
        while i < len(m_to_f_age):
            if b_age < m_to_f_age[i]:
                return (m_to_f_multiplier[i] - m_to_f_multiplier[i-1])/(m_to_f_age[i] - m_to_f_age[i-1]) * \
                       (b_age - m_to_f_age[i-1]) + m_to_f_multiplier[i-1]
            else:
                i += 1
                continue

    @staticmethod
    def check_hiv_status(id, csv_event_reporter, time):
        if id not in csv_event_reporter['Individual_ID'].values:
            return None, None
        else:
            infection_time = csv_event_reporter[(csv_event_reporter['Individual_ID'] == id) &
                                                (csv_event_reporter["Event_Name"] == "NewInfectionEvent")]["Time"].values[0].astype(int)
            if time < infection_time:
                return None, infection_time
            elif time == infection_time:
                return "NewInfection", infection_time
            else:
                aids_df = csv_event_reporter[(csv_event_reporter['Individual_ID'] == id) &
                                             (csv_event_reporter["Event_Name"] == "HIVInfectionStageEnteredAIDS")]
                if not aids_df.empty:
                    aids_time = aids_df['Time'].values[0].astype(int)
                    if time >= aids_time:
                        return "AIDS", infection_time
                    else:
                        return "HIV", infection_time
                else:
                    return "HIV", infection_time

    @staticmethod
    def check_multiples_coital_acts_cause_infection(relationship_consummated_df_transmission, a_id, b_id, time, row, r_id):
        # check if this is the only coital act in this time step
        df = relationship_consummated_df_transmission[
            (relationship_consummated_df_transmission['A_ID'] == a_id) &
            (relationship_consummated_df_transmission['B_ID'] == b_id) &
            (relationship_consummated_df_transmission['Time'] == time)]
        if len(df) > 1:
            coital_act_id = row['Coital_Act_ID']
            return f"\tWARNING: Rel_ID {r_id} has multiple coital acts at time {time} and one of them transmits " \
                   f"infection, we can't determine which coital act cause the infection, skip this coital act " \
                   f"{coital_act_id}.\n"
        else:
            return ''

    @staticmethod
    def male_to_female_transmission(a_infection_time, b_infection_time, time):
        if a_infection_time is None:
            return False
        elif b_infection_time is None:
            return a_infection_time < time
        else:
            return a_infection_time < b_infection_time and a_infection_time < time

    @staticmethod
    def male_to_female_transmission_2(a_id, b_id, csv_event_reporter):
        a_infection_time = csv_event_reporter[(csv_event_reporter['Individual_ID'] == a_id) &
                                                  (csv_event_reporter["Event_Name"] == "NewInfectionEvent")]
        if a_infection_time.empty:
            return False
        else:
            a_infection_time = int(a_infection_time["Time"])

        b_infection_time = csv_event_reporter[(csv_event_reporter['Individual_ID'] == b_id) &
                                                  (csv_event_reporter["Event_Name"] == "NewInfectionEvent")]
        if b_infection_time.empty:
            return True
        else:
            b_infection_time = int(b_infection_time["Time"])
        return a_infection_time > b_infection_time

    # overwrite the test method
    def test(self):
        sti_acquisition_multiplier = self.params[ConfigKeys.STI_Coinfection_Acquisition_Multiplier]
        sti_transmission_multiplier = self.params[ConfigKeys.STI_Coinfection_Transmission_Multiplier]
        base_infectivity = self.params[ConfigKeys.Base_Infectivity]
        acute_stage_infectivity_multiplier = self.params[ConfigKeys.Acute_Stage_Infectivity_Multiplier]
        aids_stage_infectivity_multiplier = self.params[ConfigKeys.AIDS_Stage_Infectivity_Multiplier]
        m_to_f_age = self.params[ConfigKeys.Male_To_Female_Relative_Infectivity_Ages]
        m_to_f_multiplier = self.params[ConfigKeys.Male_To_Female_Relative_Infectivity_Multipliers]
        condom_transmission_blocking_probability = self.params[ConfigKeys.Condom_Transmission_Blocking_Probability]
        base_year = self.params[ConfigKeys.Base_Year]

        if self.params[ConfigKeys.Report_Event_Recorder] != 1 or \
                self.params[ConfigKeys.Report_Coital_Acts] != 1:
            self.success = False
            self.msg.append(f'BAD: please set {ConfigKeys.Report_Event_Recorder} and '
                            f'{ConfigKeys.Report_Coital_Acts} to 1 in '
                            f'{self.config_filename}.\n')
        else:
            self.msg.append("parse report event recorder\n")
            self.parse_report_event_recorder()
            csv_event_reporter = self.csv.df
            csv_event_reporter = csv_event_reporter[["Individual_ID", "Time", "Event_Name"]]

            self.msg.append("parse RelationshipConsummated.csv\n")
            relationship_consummated_df = CsvOutput(file=os.path.join(self.output_folder, "RelationshipConsummated.csv")).df
            relationship_consummated_df_transmission = relationship_consummated_df[
                (relationship_consummated_df['A_Is_Infected'] == 1) |
                (relationship_consummated_df['B_Is_Infected'] == 1)]

            # relationship_consummated_df_transmission = relationship_consummated_df_transmission.drop_duplicates(
            #     subset=['A_ID', 'B_ID', 'Time'],
            #     keep=False).reset_index(drop=True)
            relationship_consummated_df_non_transmission = relationship_consummated_df[
                ~((relationship_consummated_df['A_Is_Infected'] == 1) |
                  (relationship_consummated_df['B_Is_Infected'] == 1))]

            self.msg.append("Testing RelationshipConsummated with: Risk_Multiplier:\n")
            result = True
            if any(relationship_consummated_df_non_transmission['Risk_Multiplier'] != 0):
                self.success = result = False
                self.msg.append(f"\tBAD: Risk_Multiplier in RelationshipConsummated should be "
                                f"0 if none of the partners is infected before the relationship.\n")
            for index, row in relationship_consummated_df_transmission.iterrows():
                risk_multiplier = row['Risk_Multiplier']
                a_id = row['A_ID']
                b_id = row['B_ID']
                time = row["Time"]
                r_id = row['Rel_ID']
                a_status, a_infection_time = self.check_hiv_status(a_id, csv_event_reporter, time)
                b_status, b_infection_time = self.check_hiv_status(b_id, csv_event_reporter, time)
                if (a_status in ['HIV', 'AIDS']) and \
                        (b_status in ['HIV', 'AIDS']):
                    expected_risk_multiplier = 0
                else:
                    if (a_status in ['NewInfection']) or \
                            (b_status in ['NewInfection']):
                        res = self.check_multiples_coital_acts_cause_infection(
                            relationship_consummated_df_transmission, a_id, b_id, time, row, r_id)
                        if res:
                            self.msg.append(res)
                            # skip this coital act
                            continue
                    expected_risk_multiplier = 0.3 * 0.85  # from CoitalActRiskFactors
                    # no co-infection (* max([sti_acquisition_multiplier, sti_transmission_multiplier]))
                    if row['Did_Use_Condom']:
                        expected_risk_multiplier *= 1 - condom_transmission_blocking_probability

                if not math.isclose(risk_multiplier, expected_risk_multiplier, rel_tol=0.01):
                    self.success = result = False
                    self.msg.append(f"\tBAD: Risk_Multiplier in RelationshipConsummated should be "
                                    f"{expected_risk_multiplier} at time {time} for Rel_ID {r_id}. got "
                                    f"{risk_multiplier}.\n")
            if result:
                self.msg.append(f"GOOD: Risk_Multiplier in RelationshipConsummated works fine.\n")

            self.msg.append("Testing RelationshipConsummated with: Transmission_Multiplier:\n")
            result = True
            if any(relationship_consummated_df_non_transmission['Transmission_Multiplier'] != 0):
                self.success = result = False
                self.msg.append(f"\tBAD: Transmission_Multiplier in RelationshipConsummated should be "
                                f"0 if none of the partners is infected before the relationship.\n")
            for index, row in relationship_consummated_df_transmission.iterrows():
                tran_multiplier = row['Transmission_Multiplier']
                a_id = row['A_ID']
                b_id = row['B_ID']
                time = row["Time"]
                r_id = row['Rel_ID']
                a_status, a_change_time = self.check_hiv_status(a_id, csv_event_reporter, time)
                b_status, b_change_time = self.check_hiv_status(b_id, csv_event_reporter, time)
                if (a_status in ['NewInfection']) or \
                        (b_status in ['NewInfection']):
                    res = self.check_multiples_coital_acts_cause_infection(
                        relationship_consummated_df_transmission, a_id, b_id, time, row, r_id)
                    if res:
                        self.msg.append(res)
                        # skip this coital act
                        continue

                if (a_status in ['HIV', 'AIDS']) and \
                    (b_status in ['HIV', 'AIDS']):
                    expected_tran_multiplier = 0
                else:
                    if self.male_to_female_transmission(a_change_time, b_change_time, time):
                        hiv_status, change_time = a_status, a_change_time
                        hiv_stage = row['A_HIV_Infection_Stage']
                    else:
                        hiv_status, change_time = b_status, b_change_time
                        hiv_stage = row['B_HIV_Infection_Stage']
                    expected_tran_multiplier = base_infectivity  # Latent
                    # hiv_stage: 1 = Acute, 2 = Latent, and 3 = AIDS
                    if hiv_status == "HIV":
                        if hiv_stage == 1:  # Acute
                            expected_tran_multiplier *= acute_stage_infectivity_multiplier
                    else:
                        if hiv_stage == 3:  # AIDS
                            expected_tran_multiplier *= aids_stage_infectivity_multiplier

                if not math.isclose(tran_multiplier, expected_tran_multiplier, rel_tol=0.01):
                    self.success = result = False
                    self.msg.append(f"\tBAD: Transmission_Multiplier in RelationshipConsummated should be "
                                    f"{expected_tran_multiplier} at time {time} for Rel_ID {r_id}. got "
                                    f"{tran_multiplier}.\n")
            if result:
                self.msg.append(f"GOOD: Transmission_Multiplier in RelationshipConsummated works fine.\n")

            self.msg.append("Testing RelationshipConsummated with: Acquisition_Multiplier:\n")
            result = True
            if any(relationship_consummated_df_non_transmission['Acquisition_Multiplier'] != 0):
                self.success = result = False
                self.msg.append(f"\tBAD: Acquisition_Multiplier in RelationshipConsummated should be "
                                f"0 if none of the partners is infected before the relationship.\n")
            for index, row in relationship_consummated_df_transmission.iterrows():
                acquisition_multiplier = row['Acquisition_Multiplier']
                a_id = row['A_ID']
                b_id = row['B_ID']
                time = row["Time"]
                r_id = row['Rel_ID']
                a_status, a_infection_time = self.check_hiv_status(a_id, csv_event_reporter, time)
                b_status, b_infection_time = self.check_hiv_status(b_id, csv_event_reporter, time)
                if (a_status in ['NewInfection']) or \
                            (b_status in ['NewInfection']):
                    res = self.check_multiples_coital_acts_cause_infection(
                        relationship_consummated_df_transmission, a_id, b_id, time, row, r_id)
                    if res:
                        self.msg.append(res)
                        # skip this coital act
                        continue
                if (a_status in ['HIV', 'AIDS']) and \
                        (b_status in ['HIV', 'AIDS']):
                    expected_acquisition_multiplier = 0
                elif self.male_to_female_transmission(a_infection_time, b_infection_time, time):
                    b_age = row['B_Age']
                    expected_acquisition_multiplier = self.find_infectivity_multiplier(m_to_f_age,
                                                                                       m_to_f_multiplier, b_age)
                else:
                    expected_acquisition_multiplier = 1 - 0.43  # Circumcision_Reduced_Acquire

                if not math.isclose(acquisition_multiplier, expected_acquisition_multiplier, rel_tol=0.01):
                    self.success = result = False
                    self.msg.append(f"\tBAD: Acquisition_Multiplier in RelationshipConsummated should be "
                                    f"{expected_acquisition_multiplier} at time {time} for Rel_ID {r_id}. got "
                                    f"{acquisition_multiplier}.\n")
            if result:
                self.msg.append(f"GOOD: Transmission_Multiplier in RelationshipConsummated works fine.\n")

            self.msg.append("Testing RelationshipConsummated with: Infection_Was_Transmitted:\n")
            result = True
            if any(relationship_consummated_df_non_transmission['Infection_Was_Transmitted'] != 0):
                self.success = result = False
                self.msg.append(f"\tBAD: Infection_Was_Transmitted in RelationshipConsummated should be "
                                f"0 if none of the partners is infected before the relationship.\n")
            for index, row in relationship_consummated_df_transmission.iterrows():
                infection_was_transmitted = row['Infection_Was_Transmitted']
                a_id = row['A_ID']
                b_id = row['B_ID']
                time = row["Time"]
                r_id = row['Rel_ID']
                a_status, a_infection_time = self.check_hiv_status(a_id, csv_event_reporter, time)
                b_status, b_infection_time = self.check_hiv_status(b_id, csv_event_reporter, time)
                if (a_status in ['NewInfection']) or \
                        (b_status in ['NewInfection']):
                    res = self.check_multiples_coital_acts_cause_infection(
                        relationship_consummated_df_transmission, a_id, b_id, time, row, r_id)
                    if res:
                        self.msg.append(res)
                        # skip this coital act
                        continue
                    if time == a_infection_time or time == b_infection_time:
                        expected_infection_was_transmitted = 1
                    else:
                        expected_infection_was_transmitted = 0
                else:
                    expected_infection_was_transmitted = 0

                if infection_was_transmitted != expected_infection_was_transmitted:
                    self.success = result = False
                    self.msg.append(f"\tBAD: Infection_Was_Transmitted in RelationshipConsummated should be "
                                    f"{expected_infection_was_transmitted} at time {time} for Rel_ID {r_id}. got "
                                    f"{infection_was_transmitted}.\n")
            if result:
                self.msg.append(f"GOOD: Infection_Was_Transmitted in RelationshipConsummated works fine.\n")

            pass


def application(output_folder="output", my_arg=None):
    if not my_arg:
        my_sft = ReportRelationshipStartTest()
    else:
        my_sft = ReportRelationshipStartTest(
            output=my_arg.output, stdout=my_arg.stdout, json_report=my_arg.json_report, event_csv=my_arg.event_csv,
            config=my_arg.config, campaign=my_arg.campaign, report_name=my_arg.report_name, debug=my_arg.debug)
    my_sft.run()


if __name__ == "__main__":
    # execute only if run as a script
    my_arg = arg_parser()
    application(my_arg=my_arg)
