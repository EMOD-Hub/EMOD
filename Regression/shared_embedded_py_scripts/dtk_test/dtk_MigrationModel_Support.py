from __future__ import division    # for dividsion to be zero, required in threshold comparison in create_report_file()
import numpy as np
import json
import os.path as path
import pandas as pd
import os
from struct import unpack
from collections import namedtuple
import dtk_test.dtk_sft as sft


# DEVNOTE: add contact to point to input path, although we should probably interrogate the environment to get the
# input_path passed to dtk

KEY_TOTAL_TIMESTEPS = "Simulation_Duration"
KEY_SIMULATION_TIMESTEP = "Simulation_Timestep"
KEY_MIGRATION_MODEL = "Migration_Model"  # ("NO_MIGRATION" | "FIXED_RATE_MIGRATION" )
KEY_MIGRATION_PATTERN = "Migration_Pattern"  # {"RANDOM_WALK_DIFFUSION" | "SINGLE_ROUND_TRIPS" | "WAYPOINTS_HOME" }
KEY_ROUNDTRIP_WAYPOINTS = "Roundtrip_Waypoints"
KEY_DEMOGRAPHICS_FILENAMES  = "Demographics_Filenames"
KEY_INPUT_DIRECTORY = "input_directory"

# following is for generic Migration parameters
KEY_MIGRATION_ENABLE = "Enable_{}_Migration"
KEY_MIGRATION_FILENAME = "{}_Migration_Filename"
KEY_MIGRATION_ROUNDTRIP_DURATION = "{}_Migration_Roundtrip_Duration"
KEY_MIGRATION_ROUNDTRIP_PROBABILITY = "{}_Migration_Roundtrip_Probability"
KEY_MIGRATION_XSCALE = "x_{}_Migration"

# column header for individual susceptibility
KEY_INDIVIDUAL_ID = "id"
KEY_INDIVIDUAL_AGE = "age"


KEY_MIGRATION_PARAM = "{}_Migration_Param"


# MigrationBinJsonNodes = namedtuple("Id", "offset")


# struct we use to store migration parameters read from config
MigrationParameter = namedtuple("MigrationParameter", "enable filename roundtrip_duration roundtrip_probability xscale")

# region post_process support
def read_migration_params(param_obj, cdj, migration_mode):
    """
    reads migration config settings for a specific migration mode from the json file and populates params_obj from a
    deserialized json object
    See https://institutefordiseasemodeling.github.io/Documentation/general/parameter-configuration.html#migration
    :param param_obj:       the param_obj dictionary to be populated, new config entries for this specific migration type will be added to the dictionary
    :param cdj:             the deserialized json from the config json file
    :param migration_mode:  the migration mode, e.g. "Sea", "Local", "Regional", "Air"...
    :returns param_obj, cdj:     the param_obj dictionary dictionary with various migration parameters added in addition to the original params passed into this funciton
    """

    enable = cdj[KEY_MIGRATION_ENABLE.format(migration_mode)]
    filename = cdj[KEY_MIGRATION_FILENAME.format(migration_mode)]
    roundtrip_duration = cdj[KEY_MIGRATION_ROUNDTRIP_DURATION.format(migration_mode)]
    roundtrip_probability = cdj[KEY_MIGRATION_ROUNDTRIP_PROBABILITY.format(migration_mode)]
    xscale = cdj[KEY_MIGRATION_XSCALE.format(migration_mode)]

    migration_params = MigrationParameter(enable, filename, roundtrip_duration, roundtrip_probability, xscale)
    param_obj[KEY_MIGRATION_PARAM.format(migration_mode)] = migration_params
    return param_obj


def load_emod_parameters(config_filename="config.json"):
    """
    reads config file and populates params_obj
    :param config_filename: name of config file (config.json)
    :returns param_obj:     dictionary with KEY_TOTAL_TIMESTEPS, etc., keys (e.g.)
    """
    with open(config_filename) as infile:
        config = json.load(infile)
        cdj = config["parameters"]
    param_obj = {}
    param_obj[KEY_INPUT_DIRECTORY] = config[KEY_INPUT_DIRECTORY]
    param_obj[KEY_TOTAL_TIMESTEPS] = cdj[KEY_TOTAL_TIMESTEPS]
    param_obj[KEY_SIMULATION_TIMESTEP] = cdj[KEY_SIMULATION_TIMESTEP]
    param_obj[KEY_MIGRATION_MODEL] = cdj[KEY_MIGRATION_MODEL]
    param_obj[KEY_MIGRATION_PATTERN] = cdj[KEY_MIGRATION_PATTERN]
    param_obj[KEY_DEMOGRAPHICS_FILENAMES] = cdj[KEY_DEMOGRAPHICS_FILENAMES]
    param_obj[KEY_ROUNDTRIP_WAYPOINTS] = cdj.get(KEY_ROUNDTRIP_WAYPOINTS, 0)
    #print("*** param_obj ***")
    #print(param_obj)
    read_migration_params(param_obj, cdj, "Local")
    read_migration_params(param_obj, cdj, "Regional")
    read_migration_params(param_obj, cdj, "Air")
    read_migration_params(param_obj, cdj, "Sea")

    return param_obj, cdj


# struct we use to store node attributes read from demographics file
NodeAttributes = namedtuple("NodeAttributes", "latitude longitude initial_population")

def read_node_attributes_from_demographics_file(filename="1x3_demographics_migration_heterogeneity.json"):
    """
    reads node attributes from the demographics json file
    See https://institutefordiseasemodeling.github.io/Documentation/general/parameter-demographics.html#nodeattributes
    :param filename: name of demographics json file
    :return node_attributes:  the node attributes named tuple object, contains latitude, longitude, initial_population
    """
    with open(filename) as infile:
        flat = json.load(infile)
    default_node_population =  flat["Defaults"]["NodeAttributes"].get("InitialPopulation",0)
    node_attributes = {}
    for i in range(len(flat["Nodes"])):
        vID = flat["Nodes"][i]["NodeID"]
        vlat = flat["Nodes"][i]["NodeAttributes"]["Latitude"]
        vlon = flat["Nodes"][i]["NodeAttributes"]["Longitude"]
        vinit_population = flat["Nodes"][i]["NodeAttributes"].get("InitialPopulation", default_node_population)  # use default node attributes if not existed
        node_attributes[vID] = NodeAttributes(vlat, vlon, vinit_population)

    return node_attributes


def read_migration_bin_json(filename="Dan_sea_migration.bin.json"):
    """
    reads the migration bin json file
    See https://institutefordiseasemodeling.github.io/Documentation/general/file-migration.html
    :param filename: name of migration bin metadata json file
    :return mdj:  the migration bin json object, with a added dictionary entry of 'DecodedNodeOffsets' which is a {Node_id}-> {To_Node_Migration_rate} mapping
    """
    with open(filename) as infile:
        mdj = json.load(infile)

    node_offset_length = 16
    # num_nodes = len(mdj["NodeOffsets"]) / (node_offset_length)
    # decode NodeOffsets into dictionary w/ d{Node_id}=>offset
    mdj["DecodedNodeOffsets"] = { int(mdj["NodeOffsets"][i:i + node_offset_length][0:8]): mdj["NodeOffsets"][i:i + node_offset_length][8:16] for i in range(0, len(mdj["NodeOffsets"]), node_offset_length)}
    return mdj


def read_migration_bin(migration_type, filename="Dan_sea_migration.bin", data_value_count=None):
    """
    reads the migration bin file
    See https://institutefordiseasemodeling.github.io/Documentation/general/file-migration.html
    :param migration_type:  type of migration, could be "Local" | "Air" | "Regional" | "Sea"
    :param filename: name of bin file
    :param data_value_count:  optional parameter, if missing, we decode the number of destinations per nodes by migration_type
    :return node_destination_rates:  the node destination rates object, a dict of dict object; to get rate from node i->j, use node_destination_rates[i][j]
    """
    number_destinations = data_value_count
    if data_value_count is None:
        if migration_type == 'Air':
            number_destinations = 60
        elif migration_type  == 'Regional':
            number_destinations = 30
        elif migration_type  == 'Local':
            number_destinations = 8
        elif migration_type == 'Sea':
            number_destinations = 5
    else:
        number_destinations = data_value_count

    print("\nread_migration_bin:{}, {}, {}\n".format(filename, migration_type, number_destinations))
    with open(filename, "rb") as handle:
        byte = "a"
        node_destination_rates = {}
        current_node_id = 1
        while len(byte) != 0:
            id = []
            rate = []
            node_rates = {}
            for i in range(number_destinations):
                byte = handle.read(4)
                if len(byte) != 0:
                    id.append(int(unpack('I', byte)[0]))
            for i in range(number_destinations):
                byte = handle.read(8)
                if len(byte) != 0:
                    rate.append(float(unpack('d', byte)[0]))
            for i in range(number_destinations):
                if len(byte) != 0 and id[i] != 0:
                    node_rates[id[i]] = rate[i]
            if len(byte) != 0:
                node_destination_rates[current_node_id] = node_rates
                current_node_id += 1

    print("node_destination_rates read:{}\n".format(str(node_destination_rates)))
    return node_destination_rates


def parse_ReportHumanMigrationTracking_output(output_filename="otuput/ReportHumanMigrationTracking.csv", debug=False):
    """
    parse custom output file ReportHumanMigrationTracking.csv into data frame
    :param output_filename: file to parse
    :param debug: if True then print debug_info and write output_df to disk as './individual_susceptibility.csv'
    :return: output_df:  data frame of dataframe of [ Time IndividualID AgeYears Gender IsAdult Home_NodeID From_NodeID To_NodeID MigrationType Event] ordered by Time
    """
    if os.path.isfile(output_filename) == False:
        print('!!! File {} not found !!!'.format(output_filename))
        return
    human_mig = pd.read_csv(output_filename)
    human_emigrating = human_mig[human_mig[" Event"]=="Emigrating"]   # only interested in emigrating events
    print(human_emigrating)
    human_emigrating.columns = [x.strip() for x in human_emigrating.columns.values]    # remove leading space in column names
    if debug:
        print("current dir = " + os.getcwd())
        print("head of processed {} read...".format(output_filename))
        print(human_emigrating.head())
    return human_emigrating


def parse_ReportNodeDemographics_output(output_filename="output/ReportNodeDemographics.csv", debug=False):
    """
    parse custom output file ReportNodeDemographics.csv into data frame
    :param output_filename: file to parse
    :param debug: if True then print debug_info and write output_df to disk as './individual_susceptibility.csv'
    :return: output_df:  data frame of dataframe of [ Time NodeID Gender AgeYears NumIndividual NumInfected ] ordered by Time
    """
    if os.path.isfile(output_filename) == False:
        print('!!! File {} not found !!!'.format(output_filename))
        return
    node_demog = pd.read_csv(output_filename)
    node_demog.columns = [x.strip() for x in node_demog.columns.values]    # remove leading space in column names
    print(node_demog)
    if debug:
        print("head of {} read...".format(output_filename))
        print(node_demog.head())
    return node_demog


def parse_output_files(node_demog_csv="output\\ReportNodeDemographics.csv", human_migration_csv="otuput\\ReportHumanMigrationTracking.csv" ,debug=False):
    """
    creates a dataframe of time step and infected,  infectiouness and stat populations
    :param node_demog_csv:  dtk custom output file ReportNodeDemographics.csv to parse
    :param human_migration_csv:  dtk custom output file ReportHumanMigrationTracking.csv to parse
    :param debug: if True then print debug_info and write output_df to disk as './individual_susceptibility.csv'
    :return: node_demog_df:  node demogrpahics data frame, [Time NodeID Gender AgeYears NumIndividual NumInfected ] ordered by Time
    :return: human_migration_df:  human migration data frame, [Time IndividualID AgeYears Gender IsAdult Home_NodeID From_NodeID To_NodeID MigrationType Event] ordered by Time
    """

    node_demog_df = parse_ReportNodeDemographics_output(node_demog_csv)
    human_migration_df = parse_ReportHumanMigrationTracking_output(human_migration_csv)

    if debug:
        print("head of node demo dataframe:")
        print(node_demog_df.head())
        print("head of human migration tracking dataframe:")
        print(human_migration_df.head())

    return node_demog_df, human_migration_df


def pexp(t=1, rate = 0.1):
    """
    return the probability distribution function for the exponential distribution with rate rate (i.e., mean 1 / rate)
    this implement the same rexp function in R for convenience purpose
    :param t:  period
    :param rate: rate
    :return: the pdf value
    """
    return 1-np.exp(-rate * t)


def create_report_file_stationary_distribution(param_obj, node_demog_df, report_name, debug=False):
    """
    Do the validation for stationary distribution as follows and create the actual reports:
    For a inter-connected Node settings without any transient nodes (i.e. for any Node pair (i,j), there exist a way to
    get from i to j at any simulation time in a finite time steps), it's equivalent to a continuous time Markov Chain model,
    and therefore we should be able to pre-calculated a stationary distribution which it will eventually converged to;
    the validation then is to run the simulation with the said node topology settings and make sure it converge to the
    pre-calculated distribution;
    Also see: https://github.com/InstituteforDiseaseModeling/TestTeam/blob/master/MigrationTest/markov-%20chain-%20stationary-%20distribution.pdf
    base from: https://www.stat.berkeley.edu/~mgoldman/Section0220.pdf

    DEVNOTE:  here we will use a fixed distribution as specified in LocalMigration_3_Nodes.bin, which specify the migration rate for a 3 node scenario as follows:
              we could use a general scenario where are rates are arbitrary, but this will make the stationary matrix calcualtion more complicated.
    N1->N2:0.1*rate_factor   N1->N3:0.1*rate_factor
    N2->N1:0.2*rate_factor   N2->N3:0.1*rate_factor
    N3->N1:0.3*rate_factor   N3->N2:0.1*rate_factor

    # following in R code
    rate_factor = 0.6   # rate need to be small enough(<1 in total) to get accurate comparison
    m1 = pexp(1,rate_factor * 0.1)   # for migration rate = 0.1 for duration of 1 for 2 identical migration channel
    m2 = pexp(1,rate_factor * 0.2)   # for migration rate = 0.2 for duration of 1 for 2 identical migration channel
    m3 = pexp(1,rate_factor * 0.3)   # for migration rate = 0.3 for duration of 1 for 2 identical migration channel
    # calculate the reduction matrix, prep for solving the transition matrix
    #       [,1]      [,2]      [,3]
    # [1,]  1-m1-m1   m1        m1
    # [2,]  m2        1-m2-m1   m1
    # [3,]  m3        m1        1-m3-m1
    #    or
    #       [,1]            [,2]            [,3]
    # [1,]  1-p(0.1)-p(0.1) p(0.1)          p(0.1)
    # [2,]  p(0.2)          1-p(0.2)-p(0.1) p(0.1)
    # [3,]  p(0.3)          p(0.1)          1-p(0.3)-p(0.1)
    # where p(x) = probability of migration out of a node given migration rate = x
    #
    reduce_m = matrix(c(-2*m1, m2, m3, m1, -(m1+m2), m1, 1,1,1), nrow=3, byrow = T)
    # 0.5322030 0.2560926 0.2117044, for rate_factor=0.1
    solve(reduce_m, c(0,0,1))
    # end R code

    :param param_obj: parameter object(read from config.json)
    :param node_demog_df:  node demogrpahics data frame, [Time NodeID Gender AgeYears NumIndividual NumInfected ] ordered by Time
    :param human_migration_df:  human migration data frame, [Time IndividualID AgeYears Gender IsAdult Home_NodeID From_NodeID To_NodeID MigrationType Event] ordered by Time
    :param report_name: report file name to write to disk
    :param debug:
    :return: auccess
    """
    simulation_duration = int(param_obj[KEY_TOTAL_TIMESTEPS])
    migration_pattern = param_obj[KEY_MIGRATION_PATTERN]
    migration_model = param_obj[KEY_MIGRATION_MODEL]
    demog_filenames = param_obj[KEY_DEMOGRAPHICS_FILENAMES]
    input_path = param_obj[KEY_INPUT_DIRECTORY]
    success = True

    with open(report_name, "w") as outfile:
        outfile.write("=========================      validation for stationary_distribtuion      =========================\n")
        outfile.write("Simulation parameters: simulation_duration={0}, migration_pattern={1}, migration_model={2}, demog_filenames={3}\n".
                      format(simulation_duration, migration_pattern, migration_model, demog_filenames))
        outfile.write("Number of rows of node_demog_csv read: {}\n".format(node_demog_df.size))
        if param_obj[KEY_TOTAL_TIMESTEPS] < 500:  # give enough time for convergence
            outfile.write(
                "!!! simulation duration {} not long enough  \n".
                    format(param_obj[KEY_TOTAL_TIMESTEPS]))
            success = False
        rate_factor = 0
        for migration_mode in ("Local", "Regional", "Air", "Sea"):
            outfile.write("Migration parameter for {}\n".format(migration_mode))
            outfile.write(param_obj[KEY_MIGRATION_PARAM.format(migration_mode)].__str__() + "\n")
            if param_obj[KEY_MIGRATION_PARAM.format(migration_mode)].enable:
                rate_factor += param_obj[KEY_MIGRATION_PARAM.format(migration_mode)].xscale
        outfile.write("Total xscale rate = {}\n".format(rate_factor))

        if migration_model != "NO_MIGRATION":
            # Devnote: optionally should add detail file list passing if we need to (for more complicated scenario later on)
            node_attributes = read_node_attributes_from_demographics_file(demog_filenames[0])
            if debug:
                print("demographics file node attributes read:")
                print(node_attributes)

            if migration_pattern == "RANDOM_WALK_DIFFUSION":
                # read migration bin files for all migration mode, and we accumulate them together get the real rate
                for migration_mode in ("Local", "Regional", "Air", "Sea"):
                    if param_obj[KEY_MIGRATION_PARAM.format(migration_mode)].enable == 1:
                        outfile.write("Extracting data from {} migration...\n".format(migration_mode))
                        migration_bin_json = read_migration_bin_json(os.path.join(input_path, param_obj[KEY_MIGRATION_PARAM.format(migration_mode)].filename.__add__(".json")))
                        if debug:
                            print("\tMigration bin json read: {}\n".format(str(migration_bin_json)))
                        if migration_bin_json['Metadata']['NodeCount'] != len(node_attributes):
                            outfile.write("!!! Node count mismatched! node count in migration bin json {}={},  node account from demographics file {} = {} \n".
                                          format(param_obj[KEY_MIGRATION_PARAM.format(migration_mode)].filename.__add__(".json"),
                                                 migration_bin_json['Metadata']['NodeCount'],
                                                 demog_filenames[0],
                                                 len(node_attributes)))
                            success = False

                        node_destination_rates = read_migration_bin(migration_mode, os.path.join(input_path, param_obj[KEY_MIGRATION_PARAM.format(migration_mode)].filename))
                        # DEVNOTE: ensure that node migration rate is as expected, note that if we use a generic rate validation, we should remove this check,
                        #          and add migration rate aggregation across all migration mode to get the real migration rate from Node i-> Node j
                        if debug:
                            print("\tMigration bin read:")
                            print(node_destination_rates)
                        outfile.write("\tMigration bin read: {}\n".format(str(node_destination_rates)))
                        expected_destinate_rates = {1: {2: 0.1, 3: 0.1}, 2: {1: 0.2, 3: 0.1}, 3: {1: 0.3, 2: 0.1}}
                        if node_destination_rates  != expected_destinate_rates :
                            outfile.write("!!! Migration bin not as expected!\n")
                            outfile.write("expected migration rates: {}\n".format(str(expected_destinate_rates )))
                            success = False

                if rate_factor > 1: # rate multiplier need to be small enough(<1) to get accurate comparison
                    outfile.write("rate factor ({}) is too big!!!\n".format(rate_factor))
                    success = False

                # from the basic migration rates calculate the probability thro' a exponenetial distribution p.d.f.
                m1 = pexp(1, rate_factor * 0.1)
                m2 = pexp(1, rate_factor * 0.2)
                m3 = pexp(1, rate_factor * 0.3)
                reduce_m = np.array([[-2 * m1, m2, m3], [m1, -(m1 + m2), m1], [1, 1, 1]])
                stationary_dist = np.linalg.solve(reduce_m, np.array([0, 0, 1]))

                max_time = node_demog_df['Time'].max()
                last_node_demog = node_demog_df[node_demog_df['Time']==max_time]
                groupby_node = last_node_demog.groupby(['NodeID'])
                groupby_node.aggregate({'NumIndividuals': ['sum']})
                total_population = sum(last_node_demog['NumIndividuals'])

                for source_node_id in (migration_bin_json['DecodedNodeOffsets'].keys()):
                    outfile.write('\tProcessing node {}.....\n '.format(source_node_id))
                    actual_population = sum(groupby_node.get_group(source_node_id)['NumIndividuals'])
                    expected_population = total_population * stationary_dist[source_node_id-1]
                    if debug:
                        outfile.write('\tactual population:{}, expected population:{}\n'.format(actual_population,
                                                                                      expected_population))
                        print('\tactual population:{}, expected population:{}\n'.format(actual_population,
                                                                                      expected_population))
                    tolerance = 0.03
                    if not np.isclose(actual_population, expected_population, rtol=tolerance):
                        outfile.write('\t\t!!! Bad actual population {}, calculated at node {}, expected {}\n'.format(
                            actual_population, source_node_id, expected_population))
                        success = False
                    else:
                        outfile.write('\t\t GOOD: node {} has actual pop {} within tolerance of expected pop {}\n'.format(
                            source_node_id, actual_population, expected_population))

            else:
                print("!!! Stationary Distribution validation only work with Random walk, {} was given\n".format(migration_pattern))
                success = False

        outfile.write(sft.format_success_msg(success))

    return success


def validate_round_trip(individualTrips, roundtrip_waypoints):
    """
    validation for WAYPOINTS_HOME model, all agent's travelling pattern should be in the pattern of A->X1->X2->...->Xk-> ..X2->X1->A,
    where k is config.Roundtrip_Waypoints;
    :param individitualTrips: data frame in the format of
     [Time IndividualID AgeYears Gender IsAdult Home_NodeID From_NodeID To_NodeID MigrationType Event] ordered by Time
    :param roundtrip_waypoints: if True then print debug_info and write output_df to disk as './individual_susceptibility.csv'
    :return validate:  True | False depends on validation on result
    """
    print("\n Validating IndividualTrips for individual {} with waypoints {}\n".format(individualTrips['IndividualID'].iloc[0], roundtrip_waypoints))
    print(individualTrips)
    validate = True
    stack = []
    trip = 0
    return_trip = False

    for x in individualTrips.itertuples():
        print('trip = {0}, return_trip = {1}, stack = {2}, validate = {3}\n'.format(trip, return_trip, stack, validate))
        print('validating tuple: {}\n'.format(str(x)))
        if trip == 0:
            if x.From_NodeID != x.Home_NodeID:
                validate = False
                print('individual {0}: Individual not at home node when starting trip!\n'.format(x.IndividualID))
                break
            else:
                print('home node detected, skipping....\n')
                trip += 1
                continue  # don't do anything for home node

        if trip < roundtrip_waypoints and return_trip==False:
            stack.append(x.From_NodeID)
            print('pushing {0} to stack\n'.format(x.From_NodeID))
            print(stack)

        elif trip == roundtrip_waypoints and return_trip==False:
            print('reaching waypoints limit...\n')
            if roundtrip_waypoints != 1:
                return_trip = True
                continue
            else:
                print('single roundtrip detected...\n')
                if x.Home_NodeID != x.To_NodeID:
                    validate = False
                    print('individual {0}: incorrect return trip detected!\n'.format(x.IndividualID))
                    print('trip details for individual {}\n'.format(x.IndividualID))
                    print(individualTrips)
                    break
                trip = 0
                continue

        elif trip > roundtrip_waypoints:
            validate = False
            print ('individual {0}: more waypoints than limit detected!\n'.format(x.IndividualID))
            break

        elif return_trip==True:
            n = stack.pop()
            trip -= 1
            print ('comparing pop node_id {0} with current node_id {1}\n'.format(n, x.From_NodeID))
            if n != x.From_NodeID:
                validate = False
                print ('individual {0}: incorrect return trip detected!\n'.format(x.IndividualID))
                print('trip details for individual {}\n'.format(x.IndividualID))
                print(individualTrips)
                break
            if stack == []:
                print('stack empty...\n')
                trip = 0
                return_trip = False
                continue

        else:
            print('shouldnt reach here\n')

        trip += 1

    return validate


def create_report_file(param_obj, node_demog_df, human_migration_df, report_name, debug=False):
    """
    Do the validation for normal scenario as follows and create the actual reports:

    For RANDOM_WALK_DIFFUSION:
    Check if the agent migration actually follow a random walk process, by validating the following:
    i) for each Node i, check that each agent got picked to migrate from there to any destination Node j, match the specified migration rate of Node i->j in migration input binary file  (i.e., no migration route bias);
    ii) Assuming a homogeneous node settings for all nodes, throughout the whole simulation, check that each agent got approximately got picked approximately the same number of times to migrate (i.e, no agent bias);
    iii) for each agent, the chance that it got pick should follow a Poisson distribution through out the whole simulation duration  (i.e. no time bias for agent picking), this can also be validated by getting the trip summary for each individual, and then make sure the time interval between each migrations follow a exponential distribution, parameterized by the migration rate parameter;

    pesudocode:
    - read from input/demog json to determine number of nodes, and set NodeAttrib = Node[i] => population;
    - read from input/migration bin json to
        - validate number of nodes matched in  Metadata.NodeCount
        - Set NodeAttrib[i] = rate[1...j]
    - read from output/ReportHumanMigrationTracking.csv to establish dataframe df_human_migration [Time IndividualID AgeYears Gender IsAdult Home_NodeID From_NodeID To_NodeID MigrationType Event]
    - read from output/ReportNodeDemographics.csv to establish dataframe df_node_demog [Time NodeID NumIndividual]
    RANDOM_WALK:
    - transform df_human_migration to [Time From_Node To_Node, MigrationType] as df_migration_type
        for each Migration type m
            for each time step t
                compare [ count of [df_migration_type] from each Node [i,j] pair (migration from node i->j at time t) / sum of [df_node_demog].NumIndividual(node i population at time t) ] ~= rate of migration for [Node i,j]
    - validate each individual's migration internval in df_human_migration match exponential distribution of lambda = migration rate, by:
    WAYPOINS_HOME / SINGLE_ROUND_TRIPS :
    - validate by validate_round_trip(individualTrips, roundtrip_waypoints) on df_human_migration;

    FOR WAYPOINTS_HOME model:
    Check that all agent's travelling pattern should be in the pattern of A->X1->X2->...->Xk-> ..X2->X1->A,, where k is config.Roundtrip_Waypoints;

    For SINGLE_ROUND_TRIPS model:
    it's just a special case of WAYPOINTS_HOME but with config.Roundtrip_Waypoints = 1 (except that there's no duration limit/return probably as set by *_Roundtrip_Duration/*_Roundtrip_Probability), so we could reuse the above simulation and set config.Roundtrip_Waypoints = 1 accordingly;

    :param param_obj: parameter object(read from config.json)
    :param node_demog_df:  node demogrpahics data frame, [Time NodeID Gender AgeYears NumIndividual NumInfected ] ordered by Time
    :param human_migration_df:  human migration data frame, [Time IndividualID AgeYears Gender IsAdult Home_NodeID From_NodeID To_NodeID MigrationType Event] ordered by Time
    :param report_name: report file name to write to disk
    :param debug:
    :return: auccess
    """
    simulation_duration = int(param_obj[KEY_TOTAL_TIMESTEPS])
    migration_pattern = param_obj[KEY_MIGRATION_PATTERN]
    migration_model = param_obj[KEY_MIGRATION_MODEL]
    demog_filenames = param_obj[KEY_DEMOGRAPHICS_FILENAMES]
    input_path = param_obj[KEY_INPUT_DIRECTORY]

    success = True
    with open(report_name, "w") as outfile:
        outfile.write("=========================      validation for normal scenario      =========================\n")
        outfile.write("Pandas version: {}\n".format(pd.__version__))
        outfile.write("numpy version: {}\n".format(np.__version__))
        outfile.write("Simulation parameters: simulation_duration={0}, migration_pattern={1}, migration_model={2}, demog_filenames={3}\n".
                      format(simulation_duration, migration_pattern, migration_model, demog_filenames))
        outfile.write("Number of rows of node_demog_csv read: {}\n".format(node_demog_df.size))
        outfile.write("Number of rows of human_migration_csv read: {}\n".format(human_migration_df.size))

        for migration_mode in ("Local", "Regional", "Air", "Sea"):
            outfile.write("Migration parameter for {}\n".format(migration_mode))
            outfile.write(param_obj[KEY_MIGRATION_PARAM.format(migration_mode)].__str__() + "\n")

        if migration_model != "NO_MIGRATION":
            # DEVNOTE: optionally should add detail file list passing if we need to (for more complicated scenario later on)
            node_attributes = read_node_attributes_from_demographics_file(demog_filenames[0])
            if debug:
                print("demographics file node attributes read:")
                print(node_attributes)

            if migration_pattern == "RANDOM_WALK_DIFFUSION":
                # validate for each migration type
                for migration_mode in ("Local", "Regional", "Air", "Sea"):
                    if param_obj[KEY_MIGRATION_PARAM.format(migration_mode)].enable == 1:
                        # do some random walk validate here
                        outfile.write("Working on {} migration...\n".format(migration_mode))
                        """
                        # Devnote: we can't validate individual in general case here as the out-going rate for all nodes could be different and there's no fixed exponential distribution as the individual moved from node to node
                        for individual_id in (human_migration_df.IndividualID.unique()):
                            # get interval of time (for exponential dist test) for a individual
                            human_migration_df[human_migration_df.IndividualID == individual_id].iloc[:, 0].diff()
                        """

                        migration_bin_json = read_migration_bin_json(os.path.join(input_path, param_obj[KEY_MIGRATION_PARAM.format(migration_mode)].filename.__add__(".json")))
                        if debug:
                            print("\tMigration bin json read:")
                            print(str(migration_bin_json))
                            outfile.write("\tMigration bin json read:")
                            outfile.write(str(migration_bin_json))
                        if migration_bin_json['Metadata']['NodeCount'] != len(node_attributes):
                            outfile.write("!!! Node count mismatched! node count in migration bin json {}={},  node account from demographics file {} = {} \n".
                                          format(param_obj[KEY_MIGRATION_PARAM.format(migration_mode)].filename.__add__(".json"),
                                                 migration_bin_json['Metadata']['NodeCount'],
                                                 demog_filenames[0],
                                                 len(node_attributes)))
                            success = False

                        outfile.write("about to read_migration_bin({}, {}) \n".format(migration_mode, os.path.join(input_path, param_obj[KEY_MIGRATION_PARAM.format(migration_mode)].filename)))
                        node_destination_rates = read_migration_bin(migration_mode, os.path.join(input_path, param_obj[KEY_MIGRATION_PARAM.format(migration_mode)].filename))
                        if debug:
                            print("\tMigration bin read:")
                            print(str(node_destination_rates))
                            outfile.write("\tMigration bin read:")
                            outfile.write(str(node_destination_rates))

                        rate_factor = param_obj[KEY_MIGRATION_PARAM.format(migration_mode)].xscale
                        for source_node_id in (migration_bin_json['DecodedNodeOffsets'].keys()):
                            for target_node_id in (migration_bin_json['DecodedNodeOffsets'].keys()):
                                if source_node_id == target_node_id:
                                    continue
                                outfile.write("\tProcessing Source node {0} to Target node {1} .....\n".format(source_node_id, target_node_id))
                                expected_migration_rate = node_destination_rates[source_node_id][target_node_id]
                                outfile.write("\tMigration rate read = {}\n".format(expected_migration_rate))
                                outfile.write("\trate factor= {}\n".format(rate_factor))
                                expected_migration_probability = pexp(1, expected_migration_rate * rate_factor)
                                outfile.write("\tMigration probability calculated = {}\n".format(expected_migration_probability))
                                num_ppl_migrated_out_from_node_at_time = human_migration_df[(human_migration_df.From_NodeID==source_node_id) & (human_migration_df.To_NodeID==target_node_id) ].groupby('Time')

                                print("size of num_ppl_migrated_out_from_node_at_time  = {}\n".format(num_ppl_migrated_out_from_node_at_time.size()))

                                for t in range(1, simulation_duration):
                                    outfile.write("\t\tvalidating for time {}\n".format(t))
                                    node_population_time_t = sum(node_demog_df.groupby(['Time', 'NodeID']).get_group((t, source_node_id)).NumIndividuals)
                                    if debug:
                                        print("node_population_time_t ={}".format(node_population_time_t))
                                        print(node_demog_df.groupby(['Time', 'NodeID']).get_group((t, source_node_id)))
                                        # DEVNOTE: when tested on my environment "num_ppl_migrated_out_from_node_at_time.groups[{}].size works for both 2.7/3.6",
                                        #          but keep hitting error {'AttributeError: 'list' object has no attribute 'size'} when running in bamboo, workaround by len(...)
                                        print("num_ppl_migrated_out_from_node_at_time.groups[{}] = {} ".format(t,num_ppl_migrated_out_from_node_at_time.groups[t]))
                                        print("num_ppl_migrated_out_from_node_at_time.groups[{}].size = {} \n".format(t, len(num_ppl_migrated_out_from_node_at_time.groups[t])))

                                    outfile.write(
                                        "\t\tnode population at time {}: {}, num individuals migration out of node({}) at time {}: {},  their division = {} \n".format(
                                            t, node_population_time_t, source_node_id, t,
                                            len(num_ppl_migrated_out_from_node_at_time.groups[t]),
                                            len(num_ppl_migrated_out_from_node_at_time.groups[t]) / node_population_time_t))
                                    if (not np.isclose(len(num_ppl_migrated_out_from_node_at_time.groups[t]) / node_population_time_t , expected_migration_probability, rtol = 0.2)):
                                        outfile.write(
                                            "\t\t!!! Bad migration probability calculated: {},  expected probability = {}, difference = {}\n".format(
                                                len(num_ppl_migrated_out_from_node_at_time.groups[t]) / node_population_time_t,
                                                expected_migration_probability,
                                                expected_migration_probability - len(num_ppl_migrated_out_from_node_at_time.groups[t]) / node_population_time_t))
                                        success = False

            elif migration_pattern in ["SINGLE_ROUND_TRIPS", "WAYPOINTS_HOME"]:
                if migration_pattern == "WAYPOINTS_HOME":
                    roundtrip_waypoints = param_obj[KEY_ROUNDTRIP_WAYPOINTS]
                else:
                    roundtrip_waypoints = 1

                # validate for each migration type
                for migration_mode in ("Local", "Regional", "Air", "Sea"):
                    if param_obj[KEY_MIGRATION_PARAM.format(migration_mode)].enable == 1:
                        human_migration_modespecific_df = human_migration_df[human_migration_df.MigrationType == migration_mode.lower()]
                        for IndividualID in (human_migration_modespecific_df.IndividualID.unique()):
                            single_individual_trips = human_migration_df[human_migration_df.IndividualID==IndividualID]
                            success = success and validate_round_trip(single_individual_trips, roundtrip_waypoints)
                            if success == False:
                                outfile.write("!!! Failed validation for migration mode {}\n".format(migration_mode))
                                print("!!! Failed validation for migration mode {}\n".format(migration_mode))
                    if success:
                        outfile.write("GOOD: migration mode {} passed validation\n".format(migration_mode))

            else:
                if debug:
                    print("!!! Bad migration pattern () detected!\n", migration_pattern)
                    success = False

        outfile.write(sft.format_success_msg(success))

    return success

# endregion
