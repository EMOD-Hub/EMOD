import json
import os


def load_config_parameters(config_filename="config.json", keys=[], debug=False):
    """
    Reads config file and populates params_obj
    Args:
        config_filename:
        keys: Config parameter names
        debug:

    Returns: param_obj: Dictionary with Config_Name, etc., keys (e.g.)

    """
    with open(config_filename) as infile:
        cdj = json.load(infile)[ConfigKeys.Parameters]
    param_obj = {}

    for key in keys:
        param_obj[key] = cdj[key]

    if debug:
        with open("DEBUG_param_object.json", 'w') as outfile:
            json.dump(param_obj, outfile, indent=4)
    return param_obj


def parse_inset_chart(output_folder="output", insetchart_name="InsetChart.json", insetkey_list=[], debug=False):
    """
    Load data from insetchart json file, creates inset_chart_obj structure with keys
    Args:
        output_folder:
        insetchart_name:
        insetkey_list:
        debug:

    Returns: inset_chart_obj structure, dictionary with keys in insetkey_list.

    """
    insetchart_path = os.path.join(output_folder, insetchart_name)
    with open(insetchart_path) as infile:
        icj = json.load(infile)[InsetKeys.Channels]

        inset_chart_obj = {}

    for insetkey in insetkey_list:
        if insetkey in icj:
            inset_chart_obj[insetkey] = icj[insetkey][InsetKeys.Data]
        else:
            print("WARNING: {0} is not in {1}.\n".format(insetkey, insetchart_name))

    if debug:
        with open("DEBUG_InsetChart.json", "w") as outfile:
            json.dump(inset_chart_obj, outfile, indent=4)

    return inset_chart_obj


class CampaignKeys:
    Events = 'Events'
    Event_Coordinator_Config = 'Event_Coordinator_Config'
    Nodeset_Config = 'Nodeset_Config'
    Start_Day = 'Start_Day'
    class_key = 'class'
    Demographic_Coverage = 'Demographic_Coverage'
    Intervention_Config = 'Intervention_Config'
    Number_Repetitions = 'Number_Repetitions'
    Timesteps_Between_Repetitions = 'Timesteps_Between_Repetitions'
    Property_Restrictions = 'Property_Restrictions'
    Target_Demographic = 'Target_Demographic'
    class InterventionClassKeys:
        OutbreakIndividual = 'OutbreakIndividual'
        SimpleVaccine = 'SimpleVaccine'
        EnvironmentalDiagnostic = 'EnvironmentalDiagnostic'
    class EnvironmentalDiagnosticKeys:
        Sample_Threshold = 'Sample_Threshold'
        Environment_IP_Key_Value = 'Environment_IP_Key_Value'
        Base_Specificity = 'Base_Specificity'
        Base_Sensitivity = 'Base_Sensitivity'
        Negative_Diagnostic_Event = 'Negative_Diagnostic_Event'
        Positive_Diagnostic_Event = 'Positive_Diagnostic_Event'


class ConfigKeys:
    Parameters = 'parameters'
    Acute_Stage_Infectivity_Multiplier = 'Acute_Stage_Infectivity_Multiplier'
    Acquisition_Blocking_Immunity_Decay_Rate = 'Acquisition_Blocking_Immunity_Decay_Rate'
    Acquisition_Blocking_Immunity_Duration_Before_Decay = 'Acquisition_Blocking_Immunity_Duration_Before_Decay'
    Age_Initialization_Distribution_Type = 'Age_Initialization_Distribution_Type'
    AIDS_Stage_Infectivity_Multiplier = 'AIDS_Stage_Infectivity_Multiplier'
    Air_Migration_Filename = 'Air_Migration_Filename'
    Air_Migration_Roundtrip_Duration = 'Air_Migration_Roundtrip_Duration'
    Air_Migration_Roundtrip_Probability = 'Air_Migration_Roundtrip_Probability'
    Air_Temperature_Filename = 'Air_Temperature_Filename'
    Air_Temperature_Offset = 'Air_Temperature_Offset'
    Air_Temperature_Variance = 'Air_Temperature_Variance'
    Animal_Reservoir_Type = 'Animal_Reservoir_Type'
    Base_Air_Temperature = 'Base_Air_Temperature'
    Base_Incubation_Period = 'Base_Incubation_Period'
    Base_Individual_Sample_Rate = 'Base_Individual_Sample_Rate'
    Base_Infectious_Period = 'Base_Infectious_Period'
    Base_Infectivity = 'Base_Infectivity'
    Base_Land_Temperature = 'Base_Land_Temperature'
    Base_Mortality = 'Base_Mortality'
    Base_Population_Scale_Factor = 'Base_Population_Scale_Factor'
    Base_Rainfall = 'Base_Rainfall'
    Base_Relative_Humidity = 'Base_Relative_Humidity'
    Base_Year = 'Base_Year'
    Birth_Rate_Dependence = 'Birth_Rate_Dependence'
    Birth_Rate_Time_Dependence = 'Birth_Rate_Time_Dependence'
    Burnin_Cache_Mode = 'Burnin_Cache_Mode'
    Burnin_Cache_Period = 'Burnin_Cache_Period'
    Burnin_Name = 'Burnin_Name'
    Campaign_Filename = 'Campaign_Filename'
    Climate_Model = 'Climate_Model'
    Climate_Update_Resolution = 'Climate_Update_Resolution'
    Config_Name = 'Config_Name'
    Condom_Transmission_Blocking_Probability = 'Condom_Transmission_Blocking_Probability'
    Custom_Reports_Filename = 'Custom_Reports_Filename'
    Custom_Individual_Events = 'Custom_Individual_Events'
    Custom_Node_Events = 'Custom_Node_Events'
    Custom_Coordinator_Events = 'Custom_Coordinator_Events'
    Death_Rate_Dependence = 'Death_Rate_Dependence'
    Default_Geography_Initial_Node_Population = 'Default_Geography_Initial_Node_Population'
    Default_Geography_Torus_Size = 'Default_Geography_Torus_Size'
    Demographics_Filenames = 'Demographics_Filenames'
    Enable_Termination_On_Zero_Total_Infectivity = 'Enable_Termination_On_Zero_Total_Infectivity'
    Enable_Aging = 'Enable_Aging'
    Enable_Air_Migration = 'Enable_Air_Migration'
    Enable_Birth = 'Enable_Birth'
    Enable_Climate_Stochasticity = 'Enable_Climate_Stochasticity'
    Enable_Default_Reporting = 'Enable_Default_Reporting'
    Enable_Default_Shedding_Function = 'Enable_Default_Shedding_Function'
    Enable_Demographics_Birth = 'Enable_Demographics_Birth'
    Enable_Demographics_Builtin = 'Enable_Demographics_Builtin'
    Enable_Demographics_Gender = 'Enable_Demographics_Gender'
    Enable_Demographics_Initial = 'Enable_Demographics_Initial'
    Enable_Demographics_Other = 'Enable_Demographics_Other'
    Enable_Demographics_Reporting = 'Enable_Demographics_Reporting'
    Enable_Demographics_Risk = 'Enable_Demographics_Risk'
    Enable_Disease_Mortality = 'Enable_Disease_Mortality'
    Enable_Family_Migration = 'Enable_Family_Migration'
    Enable_Heterogeneous_Intranode_Transmission = 'Enable_Heterogeneous_Intranode_Transmission'
    Enable_Immune_Decay = 'Enable_Immune_Decay'
    Enable_Immunity = 'Enable_Immunity'
    Enable_Immunity_Distribution = 'Enable_Immunity_Distribution'
    Enable_Immunity_Initialization_Distribution = 'Enable_Immunity_Initialization_Distribution'
    Enable_Infectivity_Reservoir = 'Enable_Infectivity_Reservoir'
    Enable_Infectivity_Scaling = 'Enable_Infectivity_Scaling'
    Enable_Initial_Prevalence = 'Enable_Initial_Prevalence'
    Enable_Interventions = 'Enable_Interventions'
    Enable_Local_Migration = 'Enable_Local_Migration'
    Enable_Maternal_Infection_Transmission = 'Enable_Maternal_Infection_Transmission'
    Enable_Maternal_Protection = 'Enable_Maternal_Protection'
    Enable_Maternal_Transmission = 'Enable_Maternal_Transmission'
    Enable_Migration_Heterogeneity = 'Enable_Migration_Heterogeneity'
    Enable_Natural_Mortality = 'Enable_Natural_Mortality'
    Enable_Property_Output = 'Enable_Property_Output'
    Enable_Rainfall_Stochasticity = 'Enable_Rainfall_Stochasticity'
    Enable_Regional_Migration = 'Enable_Regional_Migration'
    Enable_Sea_Demographics_Modifiers = 'Enable_Sea_Demographics_Modifiers'
    Enable_Sea_Family_Migration = 'Enable_Sea_Family_Migration'
    Enable_Sea_Migration = 'Enable_Sea_Migration'
    Enable_Skipping = 'Enable_Skipping'
    Enable_Spatial_Output = 'Enable_Spatial_Output'
    Enable_Superinfection = 'Enable_Superinfection'
    Enable_Timestep_Channel_In_Report = 'Enable_Timestep_Channel_In_Report'
    Enable_Vital_Dynamics = 'Enable_Vital_Dynamics'
    Environmental_Cutoff_Days = "Environmental_Cutoff_Days"
    Environmental_Peak_Start = "Environmental_Peak_Start"
    Environmental_Ramp_Down_Duration = "Environmental_Ramp_Down_Duration"
    Environmental_Ramp_Up_Duration = "Environmental_Ramp_Up_Duration"
    Family_Migration_Filename = 'Family_Migration_Filename'
    Family_Migration_Roundtrip_Duration = 'Family_Migration_Roundtrip_Duration'
    Geography = 'Geography'
    Immunity_Acquisition_Factor = 'Immunity_Acquisition_Factor'
    Immunity_Initialization_Distribution_Type = 'Immunity_Initialization_Distribution_Type'
    Immunity_Mortality_Factor = 'Immunity_Mortality_Factor'
    Immunity_Transmission_Factor = 'Immunity_Transmission_Factor'
    Incubation_Period_Distribution = 'Incubation_Period_Distribution'
    Incubation_Period_Constant = 'Incubation_Period_Constant'
    Individual_Sampling_Type = 'Individual_Sampling_Type'
    Infection_Updates_Per_Timestep = 'Infection_Updates_Per_Timestep'
    Infectious_Period_Constant = 'Infectious_Period_Constant'
    Infectious_Period_Distribution = 'Infectious_Period_Distribution'
    Infectivity_Boxcar_Forcing_Amplitude = "Infectivity_Boxcar_Forcing_Amplitude"
    Infectivity_Boxcar_Forcing_End_Time = "Infectivity_Boxcar_Forcing_End_Time"
    Infectivity_Boxcar_Forcing_Start_Time = "Infectivity_Boxcar_Forcing_Start_Time"
    Infectivity_Exponential_Baseline = "Infectivity_Exponential_Baseline"
    Infectivity_Exponential_Delay = "Infectivity_Exponential_Delay"
    Infectivity_Exponential_Rate = "Infectivity_Exponential_Rate"
    Infectivity_Sinusoidal_Forcing_Amplitude = "Infectivity_Sinusoidal_Forcing_Amplitude"
    Infectivity_Sinusoidal_Forcing_Phase = "Infectivity_Sinusoidal_Forcing_Phase"
    Infectivity_Scale_Type = 'Infectivity_Scale_Type'
    Job_Node_Groups = 'Job_Node_Groups'
    Job_Priority = 'Job_Priority'
    Land_Temperature_Filename = 'Land_Temperature_Filename'
    Land_Temperature_Offset = 'Land_Temperature_Offset'
    Land_Temperature_Variance = 'Land_Temperature_Variance'
    Listed_Events = 'Listed_Events'
    Load_Balance_Filename = 'Load_Balance_Filename'
    Load_Balance_Scheme = 'Load_Balance_Scheme'
    Local_Migration_Filename = 'Local_Migration_Filename'
    Local_Migration_Roundtrip_Duration = 'Local_Migration_Roundtrip_Duration'
    Local_Migration_Roundtrip_Probability = 'Local_Migration_Roundtrip_Probability'
    Local_Simulation = 'Local_Simulation'
    Male_To_Female_Relative_Infectivity_Ages = 'Male_To_Female_Relative_Infectivity_Ages'
    Male_To_Female_Relative_Infectivity_Multipliers = 'Male_To_Female_Relative_Infectivity_Multipliers'
    Maternal_Transmission_Probability = 'Maternal_Transmission_Probability'
    Max_Individual_Infections = 'Max_Individual_Infections'
    Max_Node_Population_Samples = 'Max_Node_Population_Samples'
    Migration_Model = 'Migration_Model'
    Migration_Pattern = 'Migration_Pattern'
    Minimum_Adult_Age_Years = 'Minimum_Adult_Age_Years'
    Mortality_Blocking_Immunity_Decay_Rate = 'Mortality_Blocking_Immunity_Decay_Rate'
    Mortality_Blocking_Immunity_Duration_Before_Decay = 'Mortality_Blocking_Immunity_Duration_Before_Decay'
    Mortality_Time_Course = 'Mortality_Time_Course'
    Node_Grid_Size = 'Node_Grid_Size'
    Node_Contagion_Decay_Rate = 'Node_Contagion_Decay_Rate'
    Num_Cores = 'Num_Cores'
    Number_Basestrains = 'Number_Basestrains'
    Number_Substrains = 'Number_Substrains'
    PKPD_Model = 'PKPD_Model'
    Population_Density_C50 = 'Population_Density_C50'
    Population_Density_Infectivity_Correction = 'Population_Density_Infectivity_Correction'
    Population_Scale_Type = 'Population_Scale_Type'
    Post_Infection_Acquisition_Multiplier = 'Post_Infection_Acquisition_Multiplier'
    Post_Infection_Mortality_Multiplier = 'Post_Infection_Mortality_Multiplier'
    Post_Infection_Transmission_Multiplier = 'Post_Infection_Transmission_Multiplier'
    Python_Script_Path = 'Python_Script_Path'
    Rainfall_Filename = 'Rainfall_Filename'
    Rainfall_Scale_Factor = 'Rainfall_Scale_Factor'
    Random_Type = 'Random_Type'
    Regional_Migration_Filename = 'Regional_Migration_Filename'
    Regional_Migration_Roundtrip_Duration = 'Regional_Migration_Roundtrip_Duration'
    Regional_Migration_Roundtrip_Probability = 'Regional_Migration_Roundtrip_Probability'
    Relative_Humidity_Filename = 'Relative_Humidity_Filename'
    Relative_Humidity_Scale_Factor = 'Relative_Humidity_Scale_Factor'
    Relative_Humidity_Variance = 'Relative_Humidity_Variance'
    Report_Coital_Acts = 'Report_Coital_Acts'
    Report_Coital_Acts_Start_Year = 'Report_Coital_Acts_Start_Year'
    Report_Coital_Acts_End_Year = 'Report_Coital_Acts_End_Year'
    Report_Coital_Acts_Min_Age_Years = 'Report_Coital_Acts_Min_Age_Years'
    Report_Coital_Acts_Max_Age_Years = 'Report_Coital_Acts_Max_Age_Years'
    Report_Coital_Acts_Must_Have_IP_Key_Value = 'Report_Coital_Acts_Must_Have_IP_Key_Value'
    Report_Coital_Acts_Must_Have_Intervention = 'Report_Coital_Acts_Must_Have_Intervention'
    Report_Coital_Acts_Node_IDs_Of_Interest = 'Report_Coital_Acts_Node_IDs_Of_Interest'
    Report_Coital_Acts_Individual_Properties = 'Report_Coital_Acts_Individual_Properties'
    Report_Coital_Acts_Include_Other_Relationship_Statistics = 'Report_Coital_Acts_Include_Other_Relationship_Statistics'
    Report_Coital_Acts_Include_HIV_Disease_Statistics = 'Report_Coital_Acts_Include_HIV_Disease_Statistics'
    Report_Coital_Acts_Has_Intervention_With_Name = 'Report_Coital_Acts_Has_Intervention_With_Name'
    Report_Event_Recorder = 'Report_Event_Recorder'
    Report_Event_Recorder_Events = 'Report_Event_Recorder_Events'
    Report_Event_Recorder_Start_Year = 'Report_Event_Recorder_Start_Year'
    Report_Event_Recorder_End_Year = 'Report_Event_Recorder_End_Year'
    Report_Event_Recorder_Min_Age_Years = 'Report_Event_Recorder_Min_Age_Years'
    Report_Event_Recorder_Max_Age_Years = 'Report_Event_Recorder_Max_Age_Years'
    Report_Event_Recorder_Must_Have_IP_Key_Value = 'Report_Event_Recorder_Must_Have_IP_Key_Value'
    Report_Event_Recorder_Must_Have_Intervention = 'Report_Event_Recorder_Must_Have_Intervention'
    Report_Event_Recorder_Node_IDs_Of_Interest = 'Report_Event_Recorder_Node_IDs_Of_Interest'
    Report_Event_Recorder_Ignore_Events_In_List = 'Report_Event_Recorder_Ignore_Events_In_List'
    Report_Event_Recorder_Individual_Properties = 'Report_Event_Recorder_Individual_Properties'
    Report_Event_Recorder_PropertyChange_IP_Key_Of_Interest = 'Report_Event_Recorder_PropertyChange_IP_Key_Of_Interest'
    Report_Relationship_Start = 'Report_Relationship_Start'
    Report_Relationship_Start_Start_Year = 'Report_Relationship_Start_Start_Year'
    Report_Relationship_Start_End_Year = 'Report_Relationship_Start_End_Year'
    Report_Relationship_Start_Min_Age_Years = 'Report_Relationship_Start_Min_Age_Years'
    Report_Relationship_Start_Max_Age_Years = 'Report_Relationship_Start_Max_Age_Years'
    Report_Relationship_Start_Must_Have_IP_Key_Value = 'Report_Relationship_Start_Must_Have_IP_Key_Value'
    Report_Relationship_Start_Must_Have_Intervention = 'Report_Relationship_Start_Must_Have_Intervention'
    Report_Relationship_Start_Individual_Properties = 'Report_Relationship_Start_Individual_Properties'
    Report_Relationship_Start_Node_IDs_Of_Interest = 'Report_Relationship_Start_Node_IDs_Of_Interest'
    Report_Relationship_Start_Include_Other_Relationship_Statistics = 'Report_Relationship_Start_Include_Other_Relationship_Statistics'
    Report_Relationship_Start_Include_HIV_Disease_Statistics = 'Report_Relationship_Start_Include_HIV_Disease_Statistics'
    Report_Node_Event_Recorder = 'Report_Node_Event_Recorder'
    Report_Coordinator_Event_Recorder = 'Report_Coordinator_Event_Recorder'
    Report_Surveillance_Event_Recorder = 'Report_Surveillance_Event_Recorder'
    Roundtrip_Waypoints = 'Roundtrip_Waypoints'
    Run_Number = 'Run_Number'
    Sample_Rate_0_18mo = 'Sample_Rate_0_18mo'
    Sample_Rate_10_14 = 'Sample_Rate_10_14'
    Sample_Rate_15_19 = 'Sample_Rate_15_19'
    Sample_Rate_18mo_4yr = 'Sample_Rate_18mo_4yr'
    Sample_Rate_20_Plus = 'Sample_Rate_20_Plus'
    Sample_Rate_5_9 = 'Sample_Rate_5_9'
    Sample_Rate_Birth = 'Sample_Rate_Birth'
    Sea_Demographics_Modifier_Adult_Females = 'Sea_Demographics_Modifier_Adult_Females'
    Sea_Demographics_Modifier_Adult_Males = 'Sea_Demographics_Modifier_Adult_Males'
    Sea_Demographics_Modifier_Child_Females = 'Sea_Demographics_Modifier_Child_Females'
    Sea_Demographics_Modifier_Child_Males = 'Sea_Demographics_Modifier_Child_Males'
    Sea_Family_Migration_Probability = 'Sea_Family_Migration_Probability'
    Sea_Migration_Filename = 'Sea_Migration_Filename'
    Sea_Migration_Roundtrip_Duration = 'Sea_Migration_Roundtrip_Duration'
    Sea_Migration_Roundtrip_Probability = 'Sea_Migration_Roundtrip_Probability'
    Simulation_Duration = 'Simulation_Duration'
    Simulation_Timestep = 'Simulation_Timestep'
    Simulation_Type = 'Simulation_Type'
    Start_Time = 'Start_Time'
    STI_Coinfection_Acquisition_Multiplier ='STI_Coinfection_Acquisition_Multiplier'
    STI_Coinfection_Transmission_Multiplier = 'STI_Coinfection_Transmission_Multiplier'
    Susceptibility_Scale_Type = 'Susceptibility_Scale_Type'
    Symptomatic_Infectious_Offset = 'Symptomatic_Infectious_Offset'
    Transmission_Blocking_Immunity_Decay_Rate = 'Transmission_Blocking_Immunity_Decay_Rate'
    Transmission_Blocking_Immunity_Duration_Before_Decay = 'Transmission_Blocking_Immunity_Duration_Before_Decay'
    Valid_Intervention_States = 'Valid_Intervention_States'
    Vector_Migration_Base_Rate = 'Vector_Migration_Base_Rate'
    logLevel_Individual = 'logLevel_Individual'
    x_Air_Migration = 'x_Air_Migration'
    x_Base_Population = 'x_Base_Population'
    x_Birth = 'x_Birth'
    x_Family_Migration = 'x_Family_Migration'
    x_Local_Migration = 'x_Local_Migration'
    x_Other_Mortality = 'x_Other_Mortality'
    x_Population_Immunity = 'x_Population_Immunity'
    x_Regional_Migration = 'x_Regional_Migration'
    x_Sea_Migration = 'x_Sea_Migration'
    x_Temporary_Larval_Habitat = 'x_Temporary_Larval_Habitat'


class DemographicsKeys:
    Defaults = "Defaults"
    Nodes = 'Nodes'
    IndividualProperties = "IndividualProperties"
    class PropertyKeys:
        Property = "Property"
        Values = "Values"
        Initial_Distribution = "Initial_Distribution"
        TransmissionMatrix = "TransmissionMatrix"
        Matrix = 'Matrix'
        Route = 'Route'
        Contact = 'Contact'
        Environmental = 'Environmental'
    class NodesKeys:
        NodeID = 'NodeID'
        NodeAttributes = 'NodeAttributes'
        InitialPopulation = 'InitialPopulation'


class InsetKeys:
    Channels = "Channels"
    Data = "Data"
    Units = "Units"
    class ChannelsKeys:
        Births = 'Births'
        Campaign_Cost = 'Campaign Cost'
        Contact_Contagion_Population = "Contact Contagion Population"
        Cumulative_Infections = 'Cumulative Infections'
        Cumulative_Reported_Infections = 'Cumulative Reported Infections'
        Daily_Human_Infection_Rate = 'Daily (Human) Infection Rate'
        Disease_Deaths = 'Disease Deaths'
        Environmental_Contagion_Population = "Environmental Contagion Population"
        Exposed_Population = 'Exposed Population'
        Human_Infectious_Reservoir = 'Human Infectious Reservoir'
        Infected = 'Infected'
        Infectious_Population = 'Infectious Population'
        Log_Prevalence = 'Log Prevalence'
        New_Infections = 'New Infections'
        New_Infections_By_Route_CONTACT = "New Infections By Route (CONTACT)"
        New_Infections_By_Route_ENVIRONMENT = "New Infections By Route (ENVIRONMENT)"
        New_Reported_Infections = 'New Reported Infections'
        Recovered_Population = 'Recovered Population'
        Statistical_Population = 'Statistical Population'
        Susceptible_Population = 'Susceptible Population'
        Waning_Population = 'Waning Population'

