import dtk_test.dtk_sft as dtk_sft
import json
import matplotlib
from sys import platform
if platform == "linux" or platform == "linux2":
    print('Linux OS. Using non-interactive Agg backend')
    matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

from scipy.stats import binom
from scipy.stats import binomtest
from scipy.stats import poisson
from scipy.stats import expon


def load_insetchart_to_df(insetchart_file):
    """
    Load insetchart.json to pandas dataframe
    :param insetchart_file:
    :return: insetchart pandas dataframe
    """
    with open(insetchart_file) as infile:
        icj = json.load(infile)["Channels"]
    data = {}
    for channel in icj.keys():
        channel_name = channel
        chars = "\\/`*_{}[]()>#+-.!$<= "
        for c in chars:
            channel_name = channel_name.replace(c, "")
        data[channel_name] = icj[channel]["Data"]
    insetchart_pd = pd.DataFrame.from_dict(data)
    return insetchart_pd


def read_inset_chart(insetchart_df):
    """
    Read channels needed from pandas dataframe of the insetchart.json
    :param insetchart_df:
    :return: an array of new pregnancies per time step, an array of mother able to get pregnant per time step
    """
    current_pregnant = insetchart_df['CurrentlyPregnant'].values
    possible_mothers = insetchart_df['PossibleMothers'].values
    # we don't need to consider women who just gave birth since it will be subtracted from the CurrentlyPregnant in the
    # next time step.
    # new_births = insetchart_df['NewBirths'].values

    new_pregnancies = insetchart_df['NewPregnancies'].values
    total_mother = possible_mothers - current_pregnant
    return new_pregnancies, total_mother


"""
----------------------------------------------
binomial distribution:
1. 95% confidence interval of the new pregnancies from a binomial distribution
2. exact (not normal approximation) p-value of binomial distribution
----------------------------------------------
"""


def cal_binom_ci_per_timestep(total_mother, prob_effect):
    ci_lower = []
    ci_upper = []
    if len(total_mother) == len(prob_effect):
        for i in range(len(total_mother)):
            if prob_effect[i] == 0 or total_mother[i] == 0:
                ci_lower.append(0)
                ci_upper.append(0)
            else:
                ci = binom.interval(confidence=0.95, n=total_mother[i], p=prob_effect[i])
                ci_lower.append(ci[0])
                ci_upper.append(ci[1]+1)
        return ci_lower, ci_upper
    else:
        raise ValueError(f'Number of time step in total_mother array and pregnancy_probability array not equal.')


def exact_binomial(count_success, count_trail, prob, alpha, output_report_file, category):
    """
    This method is not being used in the tests for now. Leave it here just in case we want to use it later.
    calculate exact alpha% confidence interval of Binomial(n = count_trail, p = prob)
    :param count_success: total number of observed success
    :param count_trail: total number of trails in the known binomial distribution
    :param prob: probability of success in the known binomial distribution
    :param alpha: confidence interval
    :return: boolean - whether count_success is within the exact alpha% CI of Bin(n = count_trail, p = prob)
    """
    success = True
    success_interval = binom.interval(confidence=alpha, n=count_trail, p=prob)
    lower_bound = success_interval[0]
    upper_bound = success_interval[1]
    p_value = binomtest(count_success, count_trail, prob)
    result_message = f"For category {category}, the success cases is {count_success}, " \
                     f"expected {alpha*100}% exact confidence interval ( {lower_bound}, {upper_bound}), " \
                     f"exact p-value of is {p_value}.\n"
    if count_success < lower_bound or count_success > upper_bound:
        success = False
        output_report_file.write(f"BAD:  {result_message}")
    else:
        output_report_file.write(f"GOOD: {result_message}")
    return success

"""
----------------------------------------------
poisson distribution:
1. 95% confidence interval of the new pregnancies from a poisson distribution
2. exact (not normal approximation) p-value of poisson distribution
----------------------------------------------
"""


def cal_poisson_rate(total_with_contra_mother, prob_effect):
    list_rate_effect = []
    if len(total_with_contra_mother) == len(prob_effect):
        list_rate_effect = np.array(total_with_contra_mother) * np.array(prob_effect)
    return list_rate_effect


def cal_poisson_ci_per_timestep(total_with_contra_mother, prob_effect):
    """
    :param total_with_contra_mother: number of female >= 15 with contraception intervention
    :param prob_effect: list of probability of being pregnant with contraception in effect over usage duration
    :return:
    """
    ci_lower = []
    ci_upper = []
    list_rate_effect = cal_poisson_rate(total_with_contra_mother, prob_effect)
    if list_rate_effect is not []:
        ci = poisson.interval(0.95, list_rate_effect)
        ci_lower = ci[0]
        ci_upper = ci[1]+1
    return ci_lower, ci_upper


def exact_poisson(actual_rate, list_rate_effect, output_report_file, category, alpha = 0.95):
    # Alpha = 0.95 is a hard-coded value in all SFTs
    success = True
    theoretical_rate = sum(list_rate_effect)
    success_interval = poisson.interval(alpha, theoretical_rate)
    lower_bound = success_interval[0]
    upper_bound = success_interval[1]
    p_value = poisson.cdf(actual_rate, theoretical_rate)
    if actual_rate > theoretical_rate:
        p_value = 1 - poisson.cdf(actual_rate-1, theoretical_rate)
    result_message = f"For category {category}, the success cases is {actual_rate}, " \
                     f"expected {alpha*100}% exact confidence interval ( {lower_bound}, {upper_bound}), " \
                     f"exact p-value of is {p_value}.\n"
    if actual_rate < lower_bound or actual_rate > upper_bound:
        success = False
        output_report_file.write(f"BAD:  {result_message}")
    else:
        output_report_file.write(f"GOOD: {result_message}")
    return success


"""
----------------------------------------------
Plotting:
1. plot confidence interval of new pregnancies per time step
2. plot expected efficacy of contraception overtime
----------------------------------------------
"""


def plot_ci_per_time_step(ci_lower, ci_upper, new_pregnancies, file_name, show = True):
    matplotlib.rc('font', size=7)
    x = np.arange(len(ci_lower))
    fig = plt.figure()
    ax = fig.gca()
    ax.set_xticks(np.arange(0, len(new_pregnancies), int(len(new_pregnancies)/20)))
    ax.set_yticks(np.arange(0, max(new_pregnancies), 1))
    plt.plot(x, ci_lower)
    plt.plot(x, ci_upper)
    plt.plot(x, new_pregnancies, 'o', markersize=2)
    plt.grid()
    plt.title(f'CI of new pregnancies per time step ({file_name}) distribution')
    plt.xlabel('Time step')
    plt.ylabel('New pregnancies')
    plt.legend(['CI lower bound', 'CI upper bound', 'Observed'], loc='upper left')
    fig.savefig(file_name + '.png')
    if show and dtk_sft.check_for_plotting():
        plt.show()
    plt.close(fig)


def plot_efficacy(efficacy, show = True):
    fig = plt.figure()
    plt.plot(efficacy)
    plt.title('Efficacy of contraception during the simulation')
    plt.xlabel('Time step')
    plt.ylabel('Efficacy')
    if show and dtk_sft.check_for_plotting():
        plt.show()
    fig.savefig('Efficacy' + '.png')
    plt.close(fig)


def count_out_ci(ci_lower, ci_upper, new_pregnancies):
    """
    Count number of times the new pregnancies is out of its confidence interval lower and/or upper bounds
    :param ci_lower:
    :param ci_upper:
    :param new_pregnancies:
    :return:
    """
    count = 0
    list_tuple = list(zip(ci_lower, ci_upper, new_pregnancies))
    for item in list_tuple:
        if item[2] > item[1] or item[2] < item[0]:
            count += 1
    return count


def get_dist_param(inv_config):
    """
    Get usage duration distribution parameters of the contraception intervention
    :param inv_config:
    :return:
    """
    dist = inv_config['Usage_Duration_Distribution']

    """
    :param dist: CONSTANT_DISTRIBUTION
    :return: Usage_Duration_Constant

    :param dist: GAUSSIAN_DISTRIBUTION
    :return: Usage_Duration_Gaussian_Mean, Usage_Duration_Gaussian_Std_Dev

    :param dist: EXPONENTIAL_DISTRIBUTION
    :return: Usage_Duration_Exponential
    """

    if dist == 'CONSTANT_DISTRIBUTION':
        return 'CONSTANT_DISTRIBUTION', [inv_config['Usage_Duration_Constant']]
    elif dist == 'GAUSSIAN_DISTRIBUTION':
        return 'GAUSSIAN_DISTRIBUTION', [inv_config['Usage_Duration_Gaussian_Mean'], inv_config['Usage_Duration_Gaussian_Std_Dev']]
    elif dist == 'EXPONENTIAL_DISTRIBUTION':
        return 'EXPONENTIAL_DISTRIBUTION', [inv_config['Usage_Duration_Exponential']]


def parse_config_file(config_filename='config.json'):
    with open(config_filename) as infile:
        parameters = json.load(infile)['parameters']
    config_params = {}
    config_params['Config_Name'] = parameters['Config_Name']
    config_params['Campaign_Filename'] = parameters['Campaign_Filename']
    config_params['total_time_step'] = parameters['Simulation_Duration']
    # get initial demographics birth probability
    demographics_filename = parameters['Demographics_Filenames'][0]
    with open(demographics_filename) as infile:
        fertility_rate_params = json.load(infile)['Defaults']['IndividualAttributes']['FertilityDistribution']
    birth_rate_temp = (fertility_rate_params['ResultScaleFactor']) * (fertility_rate_params['ResultValues'][0][0])
    # Reason why modifying the birth_rate in the following way:
    # In the limit of low birth rate, the probability of becoming pregnant is equivalent to the birth rate.
    # However, at higher birth rates, some fraction of possible mothers will already be pregnant.
    # Roughly speaking, if we want women to give birth every other year, and they gestate for one year,
    # then the expected time between pregnancy has to be one year, not two.
    # Hence, the maximum possible birth rate is 1 child per woman per gestation period.
    # This is how Emod code is handling the birth rate, may need further tests using new_pregnancy or new_birth per year
    birth_rate = birth_rate_temp / (1 - birth_rate_temp * 280)
    config_params['birth_rate_without_contraception'] = birth_rate
    return config_params


def parse_campaign_file(campaign_filename="campaign.json"):
    """
    :param campaign_filename: Used for finding campaign name
    :return: campaign dictionary with 'broadcast_name, duration, efficacy' keys
    for contraception intervention
    """
    campaign_obj = {}
    # get usage duration and waning effect distribution from campaign.json and its corresponding parameters
    with open(campaign_filename) as infile:
        campaign_json = json.load(infile)

        if len(campaign_json['Events']) != 2:
            raise ValueError(f"There should be exactly 2 events in the campaign file, found"
                             f" {len(campaign_json['Events'])}, please check the campaign configuration.")
        event_broadcast = campaign_json['Events'][0] # the first event in the campaign file is broadcast event
        broadcast_config = event_broadcast['Event_Coordinator_Config']['Intervention_Config']
        campaign_obj['broadcast_name'] = broadcast_config['Broadcast_Event']
        event_intervention = campaign_json['Events'][1] # the second event in the campaign file is contraceptive intervention
        intervention_config = event_intervention['Event_Coordinator_Config']['Intervention_Config']
        # get usage duration distribution from campaign.json
        campaign_obj['duration'] = {}
        dist, list_param = get_dist_param(intervention_config)
        campaign_obj['duration']['distribution'] = dist
        campaign_obj['duration']['list_param'] = list_param
        # get waning effect distribution from campaign.json
        campaign_obj['efficacy'] = {}
        waning_config = intervention_config['Waning_Config']
        waning_effect_class = waning_config['class']
        waning_effect_initial = waning_config['Initial_Effect']
        campaign_obj['efficacy']['waning_class'] = waning_effect_class
        campaign_obj['efficacy']['initial_effect'] = waning_effect_initial
        campaign_obj['start_day'] = event_intervention['Start_Day']
        if 'WaningEffectBox' == waning_effect_class:
            campaign_obj['efficacy']['box_duration'] = waning_config['Box_Duration']
        elif 'WaningEffectBoxExponential' == waning_effect_class:
            campaign_obj['efficacy']['box_duration'] = waning_config['Box_Duration']
            campaign_obj['efficacy']['decay_time_constant'] = waning_config['Decay_Time_Constant']
        elif 'WaningEffectConstant' == waning_effect_class:
            pass
        elif 'WaningEffectExponential' == waning_effect_class:
            campaign_obj['efficacy']['decay_time_constant'] = waning_config['Decay_Time_Constant']
        elif 'WaningEffectMapLinear' == waning_effect_class:
            campaign_obj['efficacy']['reference_timer'] = waning_config['Reference_Timer']
            campaign_obj['efficacy']['expire_at_durability_map_end'] = waning_config['Expire_At_Durability_Map_End']
            campaign_obj['efficacy']['map_times'] = waning_config['Durability_Map']['Times']
            campaign_obj['efficacy']['map_values'] = waning_config['Durability_Map']['Values']
        elif 'WaningEffectMapLinearSeasonal' == waning_effect_class:
            campaign_obj['efficacy']['map_times'] = waning_config['Durability_Map']['Times']
            campaign_obj['efficacy']['map_values'] = waning_config['Durability_Map']['Values']
        elif 'WaningEffectMapPiecewise' == waning_effect_class:
            campaign_obj['efficacy']['reference_timer'] = waning_config['Reference_Timer']
            campaign_obj['efficacy']['expire_at_durability_map_end'] = waning_config['Expire_At_Durability_Map_End']
            campaign_obj['efficacy']['map_times'] = waning_config['Durability_Map']['Times']
            campaign_obj['efficacy']['map_values'] = waning_config['Durability_Map']['Values']
        elif 'WaningEffectRandomBox' == waning_effect_class:
            campaign_obj['efficacy']['expected_discard_time'] = waning_config['Expected_Discard_Time']
    return campaign_obj


def get_efficacy(campaign_obj, waning_effect_class, waning_effect_initial, simulation_total_time, start_time):
    """
    Calculate the expected efficacy of the contraceptive intervention per time step
    :param campaign_obj:
    :param waning_effect_class: "class" in "Waning_Config"
    :param waning_effect_initial: "Initial_Effect" in "Waning_Config"
    :param simulation_total_time: "Simulation_Duration" from config file
    :return: a numpy array of expected efficacy per time step of the corresponding waning effect distribution
    """
    efficacy = []
    # birth_rate = []
    if 'WaningEffectBox' == waning_effect_class:
        waning_effect_box_duration = campaign_obj['efficacy']['box_duration']
        efficacy = [waning_effect_initial] * waning_effect_box_duration
        efficacy.extend([0] * (simulation_total_time - waning_effect_box_duration))
        # assumption: simulation_total_time - waning_effect_box_duration > 0
    elif 'WaningEffectBoxExponential' == waning_effect_class:
        waning_effect_box_duration = campaign_obj['efficacy']['box_duration']
        waning_effect_lambda = 1 / campaign_obj['efficacy']['decay_time_constant']
        # assumption: simulation_total_time - waning_effect_box_duration > 0
        decay_total_time = simulation_total_time - waning_effect_box_duration
        t = np.arange(decay_total_time)
        lambda_t = t * waning_effect_lambda
        efficacy = [waning_effect_initial] * waning_effect_box_duration
        efficacy.extend(np.exp(-lambda_t) * waning_effect_initial)
    elif 'WaningEffectConstant' == waning_effect_class:
        efficacy = [waning_effect_initial] * simulation_total_time
    elif 'WaningEffectExponential' == waning_effect_class:
        waning_effect_lambda = 1 / campaign_obj['efficacy']['decay_time_constant']
        t = np.arange(simulation_total_time)
        lambda_t = t * waning_effect_lambda
        efficacy = np.exp(-lambda_t) * waning_effect_initial
    elif 'WaningEffectMapLinear' == waning_effect_class:
        waning_reference_timer = campaign_obj['efficacy']['reference_timer']
        waning_map_time = campaign_obj['efficacy']['map_times']
        waning_map_val = campaign_obj['efficacy']['map_values']
        is_expire_at_end = campaign_obj['efficacy']['expire_at_durability_map_end']
        # check if intervention expires when the end of the map is reached.
        # case of expires: 'expire_at_durability_map_end' == 1, then effect = 0;
        effect = 0
        # case of not expires: 'expire_at_durability_map_end' == 0, then effect = last value in waning_map_val*waning_effect_initial
        if is_expire_at_end == 0:
            effect = waning_map_val[-1]
        # intervention starts from reference_timer
        time_step_mapped = np.arange(waning_reference_timer, waning_reference_timer + waning_map_time[-1])
        # assumption: time_step_remain >= 0, and usage duration > simulation_total_time
        time_step_remain = simulation_total_time - (start_time + waning_reference_timer + waning_map_time[-1])
        # efficacy of contraception of each day using linear interpolation between every consecutive mapped times
        prob_tmp = np.interp(time_step_mapped, waning_map_time, waning_map_val)
        efficacy = [0] * waning_reference_timer
        efficacy.extend(prob_tmp * waning_effect_initial)
        efficacy.extend([effect * waning_effect_initial] * time_step_remain)
        efficacy = [0] * start_time + efficacy
    elif 'WaningEffectMapLinearSeasonal' == waning_effect_class:
        waning_map_time = campaign_obj['efficacy']['map_times']
        waning_map_val = campaign_obj['efficacy']['map_values']
        time_step_mapped = np.arange(waning_map_time[-1])
        time_step_repeat = int(simulation_total_time / 365)
        time_step_repeat_remain = simulation_total_time % 365
        # efficacy of contraception of each day using linear interpolation between every consecutive mapped times
        prob_tmp = np.interp(time_step_mapped, waning_map_time, waning_map_val)
        # repeat for time_step_repeat number of years
        prob_tmp = np.tile(prob_tmp, time_step_repeat)
        # append efficacy of the remaining days left in the last year
        prob_tmp = np.append(prob_tmp, prob_tmp[0:time_step_repeat_remain])
        # prob_map_effect is probability of being pregnant with contraception in effect
        efficacy = prob_tmp * waning_effect_initial
    elif 'WaningEffectMapPiecewise' == waning_effect_class:
        waning_reference_timer = campaign_obj['efficacy']['reference_timer']
        waning_map_time = campaign_obj['efficacy']['map_times']
        waning_map_val = campaign_obj['efficacy']['map_values']
        is_expire_at_end = campaign_obj['efficacy']['expire_at_durability_map_end']
        effect = 0
        if is_expire_at_end == 0:
            effect = waning_map_val[-1]
        prob_tmp = [0] * waning_reference_timer
        waning_map_time[0] += waning_reference_timer
        i = 0
        while i < len(waning_map_val) - 1:
            prob_tmp.extend([waning_map_val[i]] * (waning_map_time[i+1] - waning_map_time[i]))
            i += 1

        time_step_remain = simulation_total_time - waning_map_time[-1]
        prob_tmp.extend([effect] * time_step_remain)
        efficacy = np.array(prob_tmp) * waning_effect_initial
    elif 'WaningEffectRandomBox' == waning_effect_class:
        # want to set waning initial effect to 1:
        # We can use cdf of exponential distribution to approximate the number of female whose usage duration has
        # expired at each time step. By setting waning initial effect to 1, no female in her usage duration can get
        # pregnant, so the number of possible mother is the number of female with usage duration expired.

        # if waning_effect_initial != 1:
        #     raise ValueError(f'Expect Initial_Effect to be 1 to test WaningEffectRandomBox.')
        # else:
        #     efficacy = [0] * simulation_total_time
        efficacy = [waning_effect_initial] * simulation_total_time
    return np.array(efficacy)


def ci_per_timestep(total_mother, birth_rate, new_pregnancies, dist = 'binomial', show = True):
    if dist == 'binomial':
        print("ci_per_timestep: binomial")
        ci_lower, ci_upper = cal_binom_ci_per_timestep(total_mother, birth_rate)
        plot_ci_per_time_step(ci_lower, ci_upper, new_pregnancies, 'Binomial', show)
    elif dist == 'poisson':
        print("ci_per_timestep: poisson")
        ci_lower, ci_upper = cal_poisson_ci_per_timestep(total_mother, birth_rate)
        plot_ci_per_time_step(ci_lower, ci_upper, new_pregnancies, 'Poisson', show)
    else:
        return
    count_pregnancy_out_of_ci = count_out_ci(ci_lower, ci_upper, new_pregnancies)
    return count_pregnancy_out_of_ci


def create_report_file(insetchart_df, config_params, campaign_obj, report_name=dtk_sft.sft_output_filename, show_plots=True):
    succeed = True
    with open(report_name, 'w') as output_report_file:
        output_report_file.write(f"Config_Name = {config_params['Config_Name']}\n")
        # get simulation time step
        simulation_total_time = config_params['total_time_step']
        # get waning effect distribution config
        waning_effect_class = campaign_obj['efficacy']['waning_class']
        waning_effect_initial = campaign_obj['efficacy']['initial_effect']
        start_time = int(campaign_obj['start_day'])
        # get birth rate per time step without contraception
        birth_rate_without_contraception = config_params['birth_rate_without_contraception']
        # check waning effect distribution and get corresponding configurations of the contraception efficacy
        efficacy = get_efficacy(campaign_obj, waning_effect_class, waning_effect_initial, simulation_total_time, start_time)
        # plot efficacy of contraception
        plot_efficacy(efficacy, show = show_plots)
        # calculate birth rate with contraception efficacy
        birth_rate = (1-efficacy) * birth_rate_without_contraception
        # calculate observed mother pool and observed new pregnant mother
        new_pregnancies, total_mother = read_inset_chart(insetchart_df)
        if 'WaningEffectRandomBox' == waning_effect_class:
            waning_discard_time = campaign_obj['efficacy']['expected_discard_time']
            prob_expire = expon.cdf(list(range(simulation_total_time)), loc=0, scale=waning_discard_time)
            total_mother = prob_expire* total_mother + (1-prob_expire)*total_mother*(1-efficacy)
            birth_rate = [birth_rate_without_contraception] * simulation_total_time
        # calculate CI bounds for new pregnancies per time step using binomial and/or poisson distribution
        count_pregnancy_out_of_ci = ci_per_timestep(total_mother, birth_rate, new_pregnancies, dist='binomial', show=show_plots)
        percent_pregnancy_out_of_ci = count_pregnancy_out_of_ci / simulation_total_time
        if percent_pregnancy_out_of_ci > 0.1:
            succeed = False
        output_report_file.write(f"Number of times pregnancy out of CI: {count_pregnancy_out_of_ci} out of"
                                 f" {simulation_total_time} time step. "
                                 f"Number of times pregnancy out of CI: {percent_pregnancy_out_of_ci}%. \n")

        list_rate_effect = cal_poisson_rate(total_mother, birth_rate)

        # Alpha = 0.95 is a hard-coded value in all SFTs
        result_in_effect_exact_poisson = exact_poisson(sum(new_pregnancies), list_rate_effect,
                                                          output_report_file, 'contraception in effect Poisson', 0.95)
        if result_in_effect_exact_poisson:
            output_report_file.write(f"GOOD: InsetChart.json looks good.\n")
        else:
            output_report_file.write(f"BAD:  InsetChart.json has some problem.\n")
            succeed = False
        output_report_file.write(dtk_sft.format_success_msg(succeed))
        return succeed
