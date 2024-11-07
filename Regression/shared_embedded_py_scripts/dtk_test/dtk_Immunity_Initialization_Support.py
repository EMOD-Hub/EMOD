
import math
import json
import os.path as path
import numpy as np
import pandas as pd
import dtk_test.dtk_sft as sft
from scipy import stats


class JsonReportKeys:
    CHANNELS = "Channels"
    DATA = "Data"


class InsetChannels:
    NEW_INFECTIONS = "New Infections"
    STATISTICAL_POPULATION = "Statistical Population"


class DemographicFileKeys:
    NODES = "Nodes"
    DEFAULTS = "Defaults"
    INDIVIDUAL_ATTRIBUTES = "IndividualAttributes"

    class SusceptibilityDistribution:
        KEY = "SusceptibilityDistribution"
        FLAG = KEY + 'Flag'
        DIST1 = KEY + '1'
        DIST2 = KEY + '2'
        class ComplexKeys:
            DistributionValues = 'DistributionValues'
            ResultValues = 'ResultValues'
    KEY_AVERAGE_IMMUNITY = "AverageImmunity"


class InitializationTypes:
    OFF = "DISTRIBUTION_OFF"
    SIMPLE = "DISTRIBUTION_SIMPLE"
    COMPLEX = "DISTRIBUTION_COMPLEX"


class ConfigKeys:
    PARAMETERS = "parameters"
    CONFIG_NAME = "Config_Name"
    DEMOGRAPHIC_FILENAMES = "Demographics_Filenames"
    CAMPAIGN_FILENAME = "Campaign_Filename"
    SUS_INIT_DIST_TYPE = "Susceptibility_Initialization_Distribution_Type"
    ENABLE_IMMUNITY = "Enable_Immunity"
    ENABLE_INITIAL_SUSCEPTIBILITY_DISTRIBUTION = "Enable_Initial_Susceptibility_Distribution"


class CampaignKeys:
    START_DAY = "Start_Day"
    DEMOGRAPHIC_COVERAGE_O = "Demographic_Coverage_Outbreak"
    DEMOGRAPHIC_COVERAGE = "Demographic_Coverage"


class ExpectedInfectionsKeys:
    MIN = "Expected_Infections_Min"
    MAX = "Expected_Infections_Max"
    MEAN = "Expected_Infections_Mean"
    TOLERANCE = "Tolerance"


class DataframeKeys:
    AGE = "age"
    MOD_ACQUIRE = "mod_acquire"
    ID = "id"

def parse_stdout_file(output_filename="test.txt",
                      debug=False):
    """
    Reads stdout file and creates an individual ID indexed
    DataFrame of ages and mod_aquire
    :param output_filename: stdout filename (test.txt)
    :param debug: generate
    :return: dataframe of all individuals from a single timestep
    """
    matches = ["{} = ".format(DataframeKeys.MOD_ACQUIRE),
               "{} = ".format(DataframeKeys.AGE),
               "{} = ".format(DataframeKeys.ID),
               "Update(): Time: "]
    filtered_lines = []
    with open(output_filename) as logfile:
        for line in logfile:
            if sft.has_match(line, matches):
                if matches[-1] in line:
                    break
                filtered_lines.append(line)
    if debug:
        with open("filtered_lines.txt","w") as outfile:
            outfile.writelines(filtered_lines)

    individuals_df = pd.DataFrame( columns=[DataframeKeys.AGE,
                                            DataframeKeys.MOD_ACQUIRE])
    individuals_df.index.name = 'index'
    for line in filtered_lines:
        age = float(sft.get_val(matches[1], line))
        acquire = float(sft.get_val(matches[0], line))
        id = int(sft.get_val(matches[2], line))
        individuals_df.loc[id] = [age, acquire]

    if debug:
        with open("DEBUG_individuals_dataframe.csv","w") as outfile:
            outfile.write(individuals_df.to_csv())
    return individuals_df


def test_all_immunity_binary(ind_df):
    '''
    Make sure that the lengths of the DataFrame of 0.0 immune
    and 1.0 immune individuals sum to the length of the whole
    DataFrame
    :param ind_df: dataframe of all individuals from a single timestep
    :return: Dataframe of non-binary immunity individuals
    '''
    immune = ind_df[DataframeKeys.MOD_ACQUIRE] == 0.0
    susceptible = ind_df[DataframeKeys.MOD_ACQUIRE] == 1.0
    immune_removed = \
        pd.concat([ind_df, ind_df[immune]]).drop_duplicates(keep=False)
    errors = \
        pd.concat([immune_removed, ind_df[susceptible]]).drop_duplicates(keep=False)
    if len(errors) > 0:
        return errors
    else:
        return None


def test_immunity_within_age_range(ind_df, max_age, expected_immunity,
                                   report_file_name=sft.sft_output_filename, min_age=0):
    '''
    NOTE: This only works for binary immunity with a constant (same
    at min as max) immunity probability in age range
    :param ind_df:
    :param max_age:
    :param report_file: s_f_t.txt file
    :param expected_immunity:
    :param min_age:
    :return:
    '''
    print("Min Age is: {0}".format(min_age))
    print("Max Age is: {0}".format(max_age))

    agerange_df = ind_df[(min_age < ind_df[DataframeKeys.AGE]) &
                         (max_age > ind_df[DataframeKeys.AGE])]
    immune = ind_df[DataframeKeys.MOD_ACQUIRE] == 0
    immune_count = len(agerange_df[immune])
    total_count = len(agerange_df)
    category_name = "Expected immunity in ages {0} to {1}".format(
        min_age, max_age
    )
    with open(report_file_name, 'a') as report_file:
        sft.test_binomial_95ci(immune_count, total_count, expected_immunity,
                               report_file, category_name)
    #raise NotImplementedError("I haven't figured out how to test this")


def parse_json_report(output_folder="output",
                      report_name="InsetChart.json",
                      channel_list = [InsetChannels.NEW_INFECTIONS,
                                      InsetChannels.STATISTICAL_POPULATION],
                      debug=False):
    """
    Creates report_data_object with keys that correspond to the channel_list
    :param output_folder: folder in which to read report (output)
    :param report_name: name of report file (InsetChart.json)
    :param channel_list: list of channels to read ("New Infections","Statistical Population")
    :param debug: generate json object on disk (False)
    :return: report_data_object
    """

    report_path = path.join(output_folder, report_name)
    chart_json = None
    with open(report_path) as infile:
        chart_json = json.load(infile)
        channels = chart_json[JsonReportKeys.CHANNELS]

    report_data_object = {}
    for key in channel_list:
        if key not in channels:
            raise KeyError("Couldn't find key {0} in {1}. Found these: {2}".format(
                key,
                report_path,
                list(channels.keys())
            ))
        else:
            channel_data = channels[key][JsonReportKeys.DATA]
            report_data_object[key] = channel_data

    if debug:
        with open('DEBUG_{0}_dataobj.json'.format(report_name), 'w') as outfile:
            json.dump(report_data_object, outfile, indent=4)

    return report_data_object


def load_config_file(config_filename="config.json",
                     config_location=".",
                     debug=False):
    config_json = None
    with open(path.join(config_location, config_filename)) as infile:
        config_json = json.load(infile)
        config_parameters = config_json[ConfigKeys.PARAMETERS]

    config_object = {}
    config_object[ConfigKeys.DEMOGRAPHIC_FILENAMES] = config_parameters[ConfigKeys.DEMOGRAPHIC_FILENAMES]
    config_object[ConfigKeys.CAMPAIGN_FILENAME] = config_parameters[ConfigKeys.CAMPAIGN_FILENAME]
    config_object[ConfigKeys.ENABLE_IMMUNITY] = config_parameters[ConfigKeys.ENABLE_IMMUNITY]
    config_object[ConfigKeys.ENABLE_INITIAL_SUSCEPTIBILITY_DISTRIBUTION] = config_parameters[ConfigKeys.ENABLE_INITIAL_SUSCEPTIBILITY_DISTRIBUTION]
    config_object[ConfigKeys.SUS_INIT_DIST_TYPE] = config_parameters[ConfigKeys.SUS_INIT_DIST_TYPE]
    config_object[ConfigKeys.CONFIG_NAME] = config_parameters[ConfigKeys.CONFIG_NAME]

    if debug:
        with open('DEBUG_config_object.json','w') as outfile:
            json.dump(config_object, outfile, indent=4)

    if not (config_parameters[ConfigKeys.ENABLE_IMMUNITY] and
            config_parameters[ConfigKeys.ENABLE_INITIAL_SUSCEPTIBILITY_DISTRIBUTION]):
        raise ValueError("Enable_Immunity and Enable_Initial_Susceptibility_Distribution must both be 1.")

    return config_object


def load_demographics_file(demographics_filename="demographics.json",
                           susceptibility_initialization_type=InitializationTypes.SIMPLE,
                           debug=False):
    demographic_json = None
    with open(demographics_filename) as infile:
        demographic_json = json.load(infile)
    demographics_object = {}

    # TODO: Handle reading from defaults
    # TODO: Handle reading overlay
    # first_node = demographic_json[DemographicFileKeys.NODES][0]
    defaults = demographic_json[DemographicFileKeys.DEFAULTS]
    ind_attributes = defaults[DemographicFileKeys.INDIVIDUAL_ATTRIBUTES]
    if DemographicFileKeys.SusceptibilityDistribution.KEY in ind_attributes:
        complex_distro = ind_attributes[DemographicFileKeys.SusceptibilityDistribution.KEY]
        demographics_object[DemographicFileKeys.SusceptibilityDistribution.KEY] = complex_distro
    demographics_object[DemographicFileKeys.SusceptibilityDistribution.FLAG] = \
        ind_attributes[DemographicFileKeys.SusceptibilityDistribution.FLAG]
    demographics_object[DemographicFileKeys.SusceptibilityDistribution.DIST1] = \
        ind_attributes[DemographicFileKeys.SusceptibilityDistribution.DIST1]
    demographics_object[DemographicFileKeys.SusceptibilityDistribution.DIST2] = \
        ind_attributes[DemographicFileKeys.SusceptibilityDistribution.DIST2]

    if susceptibility_initialization_type in [InitializationTypes.SIMPLE, InitializationTypes.OFF]:
        demographics_object[DemographicFileKeys.KEY_AVERAGE_IMMUNITY] = \
            1.0 - get_average_susceptibility(demographics_object, susceptibility_initialization_type)
    elif susceptibility_initialization_type == InitializationTypes.COMPLEX:
        demographics_object[DemographicFileKeys.SusceptibilityDistribution.KEY] = \
            ind_attributes[DemographicFileKeys.SusceptibilityDistribution.KEY]
    else:
        raise ValueError("Didn't recognize Initialization Type {0}".format(susceptibility_initialization_type))
    if debug:
        with open('DEBUG_demographics_object.json','w') as outfile:
            json.dump(demographics_object, outfile, indent=4)
    return demographics_object


def get_average_susceptibility(demographics_object, initialization_type=InitializationTypes.SIMPLE):
    if initialization_type == InitializationTypes.SIMPLE:
        average_susceptibility = get_simple_distribution_mean(
            flag=demographics_object[DemographicFileKeys.SusceptibilityDistribution.FLAG],
            dist1=demographics_object[DemographicFileKeys.SusceptibilityDistribution.DIST1],
            dist2=demographics_object[DemographicFileKeys.SusceptibilityDistribution.DIST2])
        return 1.0 - average_susceptibility
    elif initialization_type == InitializationTypes.OFF:
        return 0.0
    elif initialization_type in [InitializationTypes.COMPLEX]:
        raise NotImplementedError("Initialization type {0} doesn't have an average.".format(
            initialization_type
        ))
    else:
        raise ValueError("Yeah, I don't know what initialization type {0} is.".format(
            initialization_type
        ))


def get_simple_distribution_mean(flag, dist1, dist2):
    if flag == 0:
        # Just use first value
        return dist1
    elif flag == 1:
        # Uniform distribution, take mean of 1 and 2
        return (dist1 + dist2) / 2.0
    elif flag == 2:
        # Gaussian, dist1 is the mean by definition
        return dist1
    elif flag == 3:
        # Exponential, dist1 is the mean by definition
        return dist1
    elif flag == 4:
        # Poisson, dist1 is the mean by definition
        return dist1
    elif flag == 5:
        # LogNormal, dist1 is the mean by definition
        return dist1
    else:
        raise NotImplementedError("Haven't figured means for flag {0}".format(flag))


def get_expected_infections(stat_pop_channel,
                            average_susceptibilty,
                            campaign_object,
                            debug=False):
    outbreak_coverage = campaign_object[CampaignKeys.DEMOGRAPHIC_COVERAGE_O]
    outbreak_day = campaign_object[CampaignKeys.START_DAY]
    statistical_population = stat_pop_channel[outbreak_day]
    expected_infections_mean = statistical_population * average_susceptibilty * outbreak_coverage
    tolerance = 0.0 if average_susceptibilty == 0 else 2e-2 * statistical_population
    expected_min = expected_infections_mean - tolerance
    expected_max = expected_infections_mean + tolerance
    expected_infections = {}
    expected_infections[ExpectedInfectionsKeys.MEAN] = expected_infections_mean
    expected_infections[ExpectedInfectionsKeys.MAX] = expected_max
    expected_infections[ExpectedInfectionsKeys.MIN] = expected_min
    expected_infections[ExpectedInfectionsKeys.TOLERANCE] = tolerance
    if debug:
        with open("DEBUG_Expected_Infections_Obj.json",'w') as outfile:
            json.dump(expected_infections, outfile, indent=4)
    return expected_infections


def get_actual_infections(new_infections_channel,
                          outbreak_day):
    new_infections = new_infections_channel[outbreak_day]
    return new_infections


def create_report_file(expected_infections_obj, actual_new_infections,
                       outbreak_day, report_name, debug):
    mean_expected = expected_infections_obj[ExpectedInfectionsKeys.MEAN]
    min_expected = expected_infections_obj[ExpectedInfectionsKeys.MIN]
    max_expected = expected_infections_obj[ExpectedInfectionsKeys.MAX]
    tolerance = expected_infections_obj[ExpectedInfectionsKeys.TOLERANCE]
    with open(report_name, 'a') as outfile:
        success = True
        if math.fabs(actual_new_infections - mean_expected) > tolerance:
            success = False
            outfile.write(("BAD: At time step {0}, new infections are {1} as reported, " +
                           "expected between {2} and {3}.\n").format(
                              outbreak_day, actual_new_infections,
                              min_expected, max_expected))
        else:
            outfile.write(("GOOD: At time step {0}, new infections are {1} as reported, " +
                           "expected between {2} and {3}.\n").format(
                              outbreak_day, actual_new_infections,
                              min_expected, max_expected))
        outfile.write(sft.format_success_msg(success))
    sft.plot_data_unsorted(actual_new_infections, mean_expected,
                           label1= "Actual", label2 = "Expected",
                           ylabel="new infection", xlabel="red: actual data, blue: expected data",
                           title = "Actual new infection vs. expected new infection",
                           category = 'New_infection',show = True )
    if debug:
        print( "SUMMARY: Success={0}\n".format(success) )
    return success


def load_campaign_file(campaign_filename="campaign.json", debug = False):
    """reads campaign file and populates campaign_obj

    :param campaign_filename: campaign.json file
    :returns: campaign_obj structure, dictionary with KEY_INITIAL_EFFECT, etc., keys (e.g.)
    """
    with open(campaign_filename) as infile:
        cf = json.load(infile)
    campaign_obj = {}
    outbreak_event = cf["Events"][0]
    demographic_coverage_o = outbreak_event["Event_Coordinator_Config"][CampaignKeys.DEMOGRAPHIC_COVERAGE]
    start_day_0 = outbreak_event[CampaignKeys.START_DAY]
    campaign_obj[CampaignKeys.DEMOGRAPHIC_COVERAGE_O] = demographic_coverage_o
    campaign_obj[CampaignKeys.START_DAY] = start_day_0
    if debug:
        with open ('DEBUG_campaign_obj.json','w') as outfile:
            json.dump(campaign_obj, outfile, indent=4)

    return campaign_obj


def drop_dataframe_as_csv(temp_df: object, filename: object) -> object:
    with open(filename, 'w') as outfile:
        temp_df.to_csv(outfile)


def validate_complex_initialization_numpy(individual_dataframe, susceptibility_distribution, outfile, debug=False):
    distribution_ages = susceptibility_distribution['DistributionValues']
    distribution_probabilities = susceptibility_distribution['ResultValues']

    if debug:
        print("distribution_ages: {0}\n".format(distribution_ages))
        print("distribution_probabilities: {0}\n".format(distribution_probabilities))

    # calculate expected slopes from the distributions
    eps = np.finfo(float).eps  # Value very, very close to zero so we don't divide by zero
    slopes_expected = (np.diff(distribution_probabilities) / np.diff(distribution_ages) + eps).tolist()
    if debug:
        print("Slopes expected: {0}\n".format(slopes_expected))

    # loop through the distribution and slice the dataframe
    slopes_calculated = []
    slopes_std_err = []
    result_messages = []

    slope_template = "Slope expected: {0}\tSlope calculated: {1}\tError calculated: {2}\tIs within two error: {3}"
    success_template = "GOOD: {0}\n"
    fail_template = "BAD: {0}\n"
    neutral_template = "NEUTRAL: {0}\n"

    fit_data_segments = []
    est_data_segments = []

    success = False # we assume failure until we find at least one fit
    for x in range(0, len(distribution_ages) - 1):
        min_age = distribution_ages[x] + 1 #the first day is spotty
        max_age = distribution_ages[x + 1]
        df_agesliced = individual_dataframe[(min_age < individual_dataframe['age'])]
        df_agesliced = df_agesliced[(max_age > df_agesliced['age'])]
        df_agesliced = df_agesliced.sort_values(by='age', ascending=True)

        # temp_df = individual_dataframe[(min_age < individual_dataframe['age']) & (max_age > individual_dataframe['age'])].sort_values(by='age', ascending=True)

        if debug:
            # print("df_agesliced has {0} many, temp_df has {1} many\n".format(len(df_agesliced), len(temp_df)))
            print("df_agesliced has {0}".format(len(df_agesliced)))

        # # data_age
        data_age = df_agesliced['age']

        # # data_mod_acquire
        data_probability = df_agesliced['mod_acquire']

        if len(data_age) > 10:
            # # linfit the data for this frame
            linfit = np.polyfit(data_age, data_probability, 1)  # First (1) order fit
            slopes_calculated.append(linfit[0])

            # # Get start points
            min_age_idx = df_agesliced['age'].idxmin()
            if debug:
                print("min_age_idx: {0}".format(min_age_idx))
            start_data_x = df_agesliced.iloc[0,:]['age']
            start_data_y = df_agesliced.iloc[0,:]['mod_acquire']
            est_start_y = distribution_probabilities[x]

            # # Get end points
            est_end_y = distribution_probabilities[x + 1]
            end_data_x = max_age
            end_data_y = linfit[0] * (end_data_x - start_data_x) + start_data_y

            fit_segment = [start_data_x, end_data_x],[start_data_y, end_data_y]
            fit_data_segments.append(fit_segment)

            est_segment = [start_data_x, end_data_x],[est_start_y, est_end_y]
            est_data_segments.append(est_segment)

            # # calculate error estimation
            estimated_probability = linfit[0] * data_age + linfit[1]
            SumSquaresXX = np.sum((data_age - np.mean(data_age)) * (data_age - np.mean(data_age)))
            SumSquaresXY = np.sum((data_age - np.mean(data_age)) * (data_probability - np.mean(data_probability)))
            SumSquaresError = np.sum(
                (data_probability - estimated_probability) * (data_probability - estimated_probability)) / len(data_age)
            std_err = np.sqrt(SumSquaresError / SumSquaresXX)
            slopes_std_err.append(std_err)

            is_within_two_errors = ((linfit[0] - slopes_expected[x]) < 2 * std_err)
            slope_message = \
                slope_template.format(slopes_expected[x], linfit[0], std_err, is_within_two_errors)

            if debug:
                print("linfit: {0}".format(linfit))
                print(slope_message)

            if is_within_two_errors:
                success = True
                result_messages.append(success_template.format(slope_message))
            else:
                result_messages.append(fail_template.format(slope_message))

        else:
            # Because of histogram-like initializations, ignore tiny datasets
            if debug:
                print("Not enough data, then.\n")
            slopes_calculated.append(np.nan)
            slopes_std_err.append(np.nan)
            result_messages.append(neutral_template.format("only {0} datapoints, need 200".format(len(data_age))))

    sft.plot_scatter_with_fit_lines(individual_dataframe, 'age', 'mod_acquire',
                                    fit_data_segments, est_data_segments)
    for message in result_messages:
        if message.startswith("BAD"):
            success = False # One failure fails the test
        outfile.write(message)
    outfile.write(sft.format_success_msg(success))
