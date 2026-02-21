#!/usr/bin/python
import json
import numpy as np
import dtk_test.dtk_sft as dtk_sft
import matplotlib
from sys import platform
if platform == "linux" or platform == "linux2":
    print('Linux OS. Using non-interactive Agg backend')
    matplotlib.use('Agg')
from matplotlib import pyplot as plt
import seaborn as sns
from scipy import stats

# region Emod Constants
class ConfigParam:
    base_year = "Base_Year"
    simulation_duration = "Simulation_Duration"
    simulation_timestep = "Simulation_Timestep"
    campaign_filename = "Campaign_Filename"
    demographics_filenames = "Demographics_Filenames"


class EventReport:
    event_name = "Event_Name"
    year = "Year"
    ind_id = "Individual_ID"
    symptomatic = "NewlySymptomatic"
    new_infection = "NewInfectionEvent"
    death = "DiseaseDeaths"
    ip_name = "Adherence"
    ip_high = "HIGH"
    ip_low = "LOW"
    age_bin = "Age_Bin"
    age = "Age"
    started_art = "StartedART"
    stopped_art = "StoppedART"
    cd4 = "CD4"
    cd4_bin = "CD4_Bin"
    survival_duration = "Survival_Duration"
    count = "Count"
    gender = "Gender"
    f = "F"
    m = "M"

class Campaign:
    events = "Events"
    start_day = "Start_Day"
    property_restrictions = "Property_Restrictions"
    event_coordinator_config = "Event_Coordinator_Config"
    intervention_config = "Intervention_Config"
    class_name = "class"
    ARTMortalityTable = "ARTMortalityTable"
    art_duration_days_bins = "ART_Duration_Days_Bins"
    age_years_bins = "Age_Years_Bins"
    cd4_count_bins = "CD4_Count_Bins"
    mortality_table = "MortalityTable"
    ART = "AntiretroviralTherapy"
    target_gender = "Target_Gender"
# endregion

# region Config and Campaign
def load_config(config_filename, output_report_file):
    output_report_file.write(f"Config_Name = {dtk_sft.get_config_name(config_filename)}\n")
    base_year, simulation_duration, simulation_timestep, campaign_filename = \
        dtk_sft.get_config_parameter(config_filename,
                                     [ConfigParam.base_year,
                                      ConfigParam.simulation_duration,
                                      ConfigParam.simulation_timestep,
                                      ConfigParam.campaign_filename])
    simulation_duration_year = simulation_duration / dtk_sft.DAYS_IN_YEAR

    output_report_file.write(f"\t{ConfigParam.base_year} = {base_year}\n")
    output_report_file.write(f"\tSimulation_Duration is {simulation_duration_year} years({simulation_duration} "
                             f"days)\n")
    output_report_file.write(f"\t{ConfigParam.simulation_timestep} = {simulation_timestep} days\n")
    return base_year, simulation_duration, simulation_duration_year, simulation_timestep, campaign_filename


def load_campaign(campaign_filename):
    with open(campaign_filename, 'r') as campaign_file:
        cf = json.load(campaign_file)
        try:
            events = cf[Campaign.events]
            res = []
            for event in events:
                iv = event[Campaign.event_coordinator_config][Campaign.intervention_config]
                if iv[Campaign.class_name] == Campaign.ARTMortalityTable:
                    start_day = event[Campaign.start_day]
                    property_restrictions = event[Campaign.event_coordinator_config][Campaign.property_restrictions]
                    if property_restrictions:
                        ip_name, ip_value = property_restrictions[0].split(":")
                    else:
                        ip_name = ip_value = None
                    art_duration_days_bins = iv[Campaign.art_duration_days_bins]
                    age_years_bins = iv[Campaign.age_years_bins]
                    cd4_count_bins = iv[Campaign.cd4_count_bins]
                    mortality_table = iv[Campaign.mortality_table]
                    res.append([start_day, ip_name, ip_value,
                                {Campaign.art_duration_days_bins: art_duration_days_bins,
                                 Campaign.age_years_bins: age_years_bins,
                                 Campaign.cd4_count_bins: cd4_count_bins,
                                 Campaign.mortality_table: mortality_table}])
            return res

        except KeyError as ke:
            raise KeyError(f"{ke} is not found in {campaign_filename}.\n")


def load_campaign_ART(campaign_filename):
    with open(campaign_filename, 'r') as campaign_file:
        cf = json.load(campaign_file)
        try:
            events = cf[Campaign.events]
            res = []
            for event in events:
                iv = event[Campaign.event_coordinator_config][Campaign.intervention_config]
                if iv[Campaign.class_name] == Campaign.ARTMortalityTable:
                    start_day = event[Campaign.start_day]
                    property_restrictions = event[Campaign.event_coordinator_config][
                        Campaign.property_restrictions]
                    if property_restrictions:
                        ip_name, ip_value = property_restrictions[0].split(":")
                    else:
                        ip_name = ip_value = None
                    art_duration_days_bins = iv[Campaign.art_duration_days_bins]
                    age_years_bins = iv[Campaign.age_years_bins]
                    cd4_count_bins = iv[Campaign.cd4_count_bins]
                    mortality_table = iv[Campaign.mortality_table]
                    res.append([start_day, ip_name, ip_value,
                                {Campaign.art_duration_days_bins: art_duration_days_bins,
                                 Campaign.age_years_bins: age_years_bins,
                                 Campaign.cd4_count_bins: cd4_count_bins,
                                 Campaign.mortality_table: mortality_table}])
                elif iv[Campaign.class_name] == Campaign.ART:
                    start_day = event[Campaign.start_day]
                    property_restrictions = event[Campaign.event_coordinator_config][
                        Campaign.property_restrictions]
                    if property_restrictions:
                        ip_name, ip_value = property_restrictions[0].split(":")
                    else:
                        ip_name = ip_value = None
                    res.append([start_day, ip_name, ip_value,
                                {}])

            return res

        except KeyError as ke:
            raise KeyError(f"{ke} is not found in {campaign_filename}.\n")


def check_campaign(campaign_object, campaign_filename, expected_one_bin, expected_intervention_count):
    res_msg = []
    if len(campaign_object) != expected_intervention_count:
        res_msg.append(f"BAD: {len(campaign_object)} {Campaign.ARTMortalityTable} intervention(s) "
                                 f"in the {campaign_filename}, expected {expected_intervention_count} "
                                 f"{Campaign.ARTMortalityTable} intervention, please check the test.\n")

    start_day, ip_name, ip_value, art_mortality_table = campaign_object[0]

    check_campaign_result = True
    for param_name in expected_one_bin:
        value = art_mortality_table[param_name]
        if len(value) != 1:
            check_campaign_result = False
            res_msg.append(f"BAD: test is expected 1 bin in {param_name}, got {len(value)}, "
                                     f"please check the test.\n")
    if check_campaign_result:
        res_msg.append(f"GOOD: {expected_one_bin} all have 1 bin. Mortality rate is the same within"
                                 f" the same {Campaign.cd4_count_bins}.\n")
    return res_msg, start_day, ip_name, ip_value, art_mortality_table
# endregion


def convert_value_to_bin(value, bins):
    """
    if inputs are related to age, convert age to age bins based on Age_Years_Bins array in ARTMortalityTable; else if
    if inputs are related to cd4 count, convert cd4 count to cd4 count bins based on CD4_Count_Bins array in
    ARTMortalityTable.
    Args:
        value: age or cd4 count
        bins: Age_Years_Bins or CD4_Count_Bins

    Returns: string of age bin or cd4 count bin

    """
    if not len(bins) or not (isinstance(bins, np.ndarray) or isinstance(bins, list)):
        return -1
    bins = sorted(bins)
    if value < bins[0]:
        return "lt-" + str(bins[0])
    for idx in range(1, len(bins)):
        bin = bins[idx]
        if value < bin:
            return str(bins[idx -1]) + "-" + str(bin)
    return "gt-" + str(bins[-1])


# region functions for CD4Count tests
def prepare_df_to_test_by_cd4(event_df, start_day, base_year):
    # get time and individual id when death happened
    event_death_df = event_df[event_df[EventReport.event_name] == EventReport.death][
        [EventReport.year, EventReport.ind_id]]
    # get actual cd4 count, cd4 count bin and individual id when ART started
    event_startART_df = event_df[event_df[EventReport.event_name] == EventReport.started_art][[
        EventReport.ind_id,
        EventReport.cd4_bin,
        EventReport.cd4]]
    # merge event_startART_df into event_death_df with key = ind_id and inner join.
    df_to_test = event_death_df.merge(event_startART_df, on=EventReport.ind_id, how="inner")
    # list of cd4 bins in this test(could be different than what campaign defines.)
    cd4_bins = df_to_test[EventReport.cd4_bin].unique()

    # filter by: time > intervention start day
    df_to_test = df_to_test[(df_to_test[EventReport.year]) > start_day / dtk_sft.DAYS_IN_YEAR + base_year]
    # calculate actual survival duration
    df_to_test[EventReport.survival_duration] = df_to_test[EventReport.year] - base_year - \
                                                           start_day / dtk_sft.DAYS_IN_YEAR

    # plot pdf of Survival Duration per cd4 Group
    fig = plt.figure()
    ax = fig.add_axes([0.12, 0.15, 0.76, 0.76])
    for cd4_bin in cd4_bins:
        df = df_to_test[df_to_test[EventReport.cd4_bin] == cd4_bin]
        sns.histplot(df[EventReport.survival_duration], kde=True, ax=ax, label=cd4_bin)
    ax.set_title("PDF of Survival Duration per CD4 Bin")
    ax.set_ylabel("Probability")
    ax.set_xlabel("Survival Duration(Years)")
    plt.legend(loc=0)
    plt.savefig("PDF_of_Survival_Duration_per_CD4_Bin.png")
    if dtk_sft.check_for_plotting():
        plt.show()
    plt.close()

    # plot density plot for all cd4 count under test:
    df_to_test[EventReport.cd4].plot(kind="density")
    plt.legend(loc=0)
    plt.savefig("CD4_count_density.png")
    if dtk_sft.check_for_plotting():
        plt.show()
    plt.close()
    return df_to_test, cd4_bins


def test_survival_duration_on_cd4(df_to_test, cd4_bins, mortality_table, cd4_count_bins,
                                  simulation_duration_year, min_cd4=None, max_cd4=None):
    res_msg = []
    for cd4_bin in cd4_bins:
        df = df_to_test[df_to_test[EventReport.cd4_bin] == cd4_bin]
        duration_to_test = df[EventReport.survival_duration].tolist()

        rate, exact_test = get_rate_by_cd4(df, cd4_bin, mortality_table, cd4_count_bins, min_cd4, max_cd4)

        res_msg.append(f"Test cd4 group: {cd4_bin}:\n")

        res_msg.append(f"\tTest if survival duration on ARTMortalityTable for cd4 group: {cd4_bin}"
                       f" follows exponential distribution with rate = {rate}:\n")

        res_msg.append(f"\tTest sample size = {len(duration_to_test)}.\n")

        dist_expon_scipy = stats.expon.rvs(loc=0, scale=1 / rate, size=max(100000, len(duration_to_test)))
        # make sure the simulation duration is long enough:
        if max(dist_expon_scipy) > simulation_duration_year * 1.1:
            res_msg.append(f"\tWARNING: simulation_duration = {simulation_duration_year} years may not "
                           f"be long enough here. Maximum value draw from scipy exponential distribution"
                           f"with rate = {rate} is {max(dist_expon_scipy)}.\n")
            res_msg.append(f"\tCap theoretical data at {simulation_duration_year * 1.1} years.\n")
            dist_expon_scipy = [x for x in dist_expon_scipy if x < simulation_duration_year * 1.1]

        # reporting precision for year in csv report is 2 decimal places.
        dist_expon_scipy = [dtk_sft.round_up(x, 2) for x in dist_expon_scipy]
        result = stats.ks_2samp(duration_to_test, dist_expon_scipy)  # use ks test instead of ad test.
        p_value = dtk_sft.get_p_s_from_ksresult(result)['p']
        s = dtk_sft.get_p_s_from_ksresult(result)['s']
        msg = f"ks_2samp() with rate = {rate}(per year) return p value = " \
            f"{p_value} and statistic = {s}.\n"

        if exact_test:
            if p_value < 5e-2:
                res_msg.append("\tBAD: statistical test on survival duration failed! " + msg)
            else:
                res_msg.append("\tGOOD: statistical test on survival duration passed! " + msg)
        if not exact_test:
            res_msg.append(f"\tSince we are testing on the average exponential rate from data drawn from"
                                     f" different distributions, we give it a bigger critical value:\n")
            critical_value_s = 1.94947 * dtk_sft.calc_ks_critical_value(len(duration_to_test)) / 1.36
            if s > critical_value_s and p_value < 1e-3:
                succeed = False
                res_msg.append("\tBAD: statistical test on survival duration failed! Used critical_value"
                                         f"={critical_value_s} (significance level = 0.1%)." + msg)
            else:
                res_msg.append("\tGOOD: statistical test on survival duration passed! Used critical_value"
                                         f"={critical_value_s} (significance level = 0.1%)." + msg)

        res_msg.append(f"\tPlease see survival_durations_onART_cd4_{cd4_bin}.png.\n")
        # replace by the three_plots method
        # # Plot test data with theoretical data
        # dtk_sft.plot_data(dist_expon_scipy[:len(duration_to_test)], duration_to_test,
        #                   sort=True, category=f"survival_durations_onART_cd4_{cd4_bin}", overlap=True,
        #                   label1="expon from scipy", label2="survival_durations_onART",
        #                   xlabel="Data Points", ylabel="Duration(Years)",
        #                   title=f"survival_durations_onART_cd4_{cd4_bin}")
        # # Plot with cdf function
        # dtk_sft.plot_cdf_w_fun(duration_to_test, name=f"expon_cdf_cd4_{cd4_bin}",
        #                        cdf_function=stats.expon.cdf,
        #                        args=(0, 1 / rate), show=True)

        # plot data, histogram and CDF with test data and theoretical data/cdf.
        dtk_sft.three_plots(duration_to_test,
                            cdf_function=stats.expon.cdf, args=(0, 1 / rate),
                            dist2=dist_expon_scipy[:len(duration_to_test)],
                            label1="Emod", label2="scipy",
                            title=f"survival_durations_onART_cd4_{cd4_bin}",
                            xlabel="Data Points", ylabel="Duration(Years)",
                            category=f"survival_durations_onART_cd4_{cd4_bin}",
                            show=True, sort=True)
    return res_msg


def get_rate_by_cd4(df, cd4_bin, mortality_table, cd4_count_bins, min_cd4=None, max_cd4=None):
    exact_test = True
    if cd4_bin.startswith("lt"):
        rate = mortality_table[0][0][0]
    elif cd4_bin.startswith("gt"):
        rate = mortality_table[0][0][-1]
    else:
        rates = []
        cd4_low = float(cd4_bin.split("-")[0]) if not min_cd4 else min_cd4
        cd4_high = float(cd4_bin.split("-")[1])if not max_cd4 else max_cd4
        i = cd4_count_bins.index(cd4_low)
        j = cd4_count_bins.index(cd4_high)
        rate_low = mortality_table[0][0][i]
        rate_high = mortality_table[0][0][j]
        for cd4 in df[EventReport.cd4]:
            rates.append(rate_low + (rate_high - rate_low) * (cd4 - cd4_low) / (cd4_high - cd4_low))
        # mean_rate = (rate_low + rate_high) / 2
        rate = sum(rates) / len(rates)  # using average of mortality rate
        exact_test = False

    return rate, exact_test
# endregion


# region function for AgeBins tests
def test_survival_time_on_age(df_to_test, age_bins, age_years_bins, mortality_table, art_mortality_table_start_day, base_year,
                              simulation_duration_year):
    res_msg = []
    for age_bin in age_bins:
        if age_bin.startswith("lt"):
            rate = mortality_table[0][0][0]
        elif age_bin.startswith("gt"):
            rate = mortality_table[0][-1][0]
        else:
            i = age_years_bins.index(float(age_bin.split("-")[1]))
            rate = mortality_table[0][i][0]
        res_msg.append(f"Test age bin group: {age_bin}:\n")

        death_year = df_to_test[df_to_test[EventReport.age_bin] == age_bin][EventReport.year]
        duration_to_test = [(x - base_year - art_mortality_table_start_day / dtk_sft.DAYS_IN_YEAR)
                            for x in death_year]

        res_msg.append(f"\tTest if survival duration on ARTMortalityTable for age bin group: {age_bin}"
                                 f" follows exponential distribution with rate = {rate}:\n")
        res_msg.append(f"\tTest sample size = {len(duration_to_test)}.\n")

        if rate == 0:
            result = len(duration_to_test) == 0  # expect no death
            if not result:
                res_msg.append(f"\tBAD: mortality rate = {rate} for age bin group: {age_bin}, expect no "
                               f"death, got {len(duration_to_test)}, test failed.\n")
                # Plot test data if it fails
                dtk_sft.plot_data(duration_to_test,
                                  sort=True, category=f"survival_durations_onART_{age_bin}", overlap=False,
                                  label1="survival_durations_onART",
                                  xlabel="Data Points", ylabel="Duration(Years)",
                                  title=f"survival_durations_onART_{age_bin}")
            else:
                res_msg.append(f"\tGOOD: mortality rate = {rate} for age bin group: {age_bin}, expect no "
                               f"death, got {len(duration_to_test)}, test passed.\n")
        else:
            dist_expon_scipy = stats.expon.rvs(loc=0, scale=1 / rate, size=max(100000, len(duration_to_test)))
            # make sure the simulation duration is long enough:
            if max(dist_expon_scipy) > simulation_duration_year * 1.1:
                res_msg.append(f"\tWARNING: simulation_duration = {simulation_duration_year} years may "
                                         f"not be long enough here. Maximum value draw from scipy exponential "
                                         f"distribution with rate = {rate} is {max(dist_expon_scipy)}.\n")
                res_msg.append(f"\tCap theoretical data at {simulation_duration_year * 1.1} years.\n")
                dist_expon_scipy = [x for x in dist_expon_scipy if x < simulation_duration_year * 1.1]

            result = stats.anderson_ksamp([duration_to_test, dist_expon_scipy])
            if hasattr(result, "pvalue"):  # SciPy > v1.17
                p_value = result.pvalue
            else:
                p_value = result.significance_level
            s = result.statistic
            msg = f"anderson_ksamp() with rate = {rate}(per year) return p value = " \
                f"{p_value} and statistic = {s}.\n"
            if p_value < 5e-2:
                res_msg.append("\tBAD: statistical test on survival duration failed! " + msg)
            else:
                res_msg.append("\tGOOD: statistical test on survival duration passed! " + msg)

            res_msg.append(f"\tPlease see survival_durations_onART_{age_bin}.png.\n")

            # plot data, histogram and CDF with test data and theoretical data/cdf.
            dtk_sft.three_plots(duration_to_test,
                                cdf_function=stats.expon.cdf, args=(0, 1 / rate),
                                dist2=dist_expon_scipy[:len(duration_to_test)],
                                label1="Emod", label2="scipy",
                                title=f"survival_durations_onART_{age_bin}",
                                xlabel="Data Points", ylabel="Duration(Years)",
                                category=f"survival_durations_onART_{age_bin}",
                                show=True, sort=True)

    return res_msg


# region functions for ARTDuration tests
def load_ip_group(ip_filename="IP_overlay.json"):
    with open(ip_filename, 'r') as ip_file:
        ip = json.load(ip_file)
        try:
            property = ip["Defaults"]["IndividualProperties"][0]["Property"]
            values = ip["Defaults"]["IndividualProperties"][0]["Values"]
            return property, values
        except KeyError as ke:
            raise KeyError(f"{ke} is not found in {ip_filename}.\n")


def parse_campaign(campaign_object, msg):
    art_start_days = {}
    for campaign in campaign_object:
        start_day, ip_name, ip_value, mortality_table = campaign
        if mortality_table:
            art_mortality_table_start_day = start_day
            if ip_value or ip_name:
                msg.append(f"WARNING: {Campaign.property_restrictions} for {Campaign.ARTMortalityTable} is "
                           f"{ip_name}: {ip_value}, expect empty string, please fix the test.\n")
            art_mortality_table = mortality_table
            expected_one_bin = [Campaign.age_years_bins,
                                Campaign.cd4_count_bins]
            check_campaign_result = True
            for param_name in expected_one_bin:
                value = art_mortality_table[param_name]
                if len(value) != 1:
                    check_campaign_result = False
                    msg.append(f"BAD: test is expected 1 bin in {param_name}, got {len(value)}, "
                                             f"please check the test.\n")
            if check_campaign_result:
                msg.append(
                    f"GOOD: {expected_one_bin} all have 1 bin. Mortality rate is the same within"
                    f" the same {Campaign.art_duration_days_bins}.\n")
            art_duration_days_bins = art_mortality_table[Campaign.art_duration_days_bins]
            if len(art_duration_days_bins) == 1:
                msg.append(
                    f"WARNING: {Campaign.art_duration_days_bins} only has one element, please test more than"
                    f"one art duration days bin.\n")
        else:
            art_start_days[ip_name + "-" + ip_value] = start_day

    return art_start_days, art_duration_days_bins, art_mortality_table, art_mortality_table_start_day


def check_ART_start_event(event_df, ip_name, art_start_days, base_year, event_report_name, msg):
    startedART_df = event_df[event_df[EventReport.event_name] == EventReport.started_art
                             ].groupby([ip_name, EventReport.year]).count().reset_index()
    check_startedART_resut = True
    for ip_group, start_day in art_start_days.items():
        start_year = round(start_day / dtk_sft.DAYS_IN_YEAR + base_year, 2)
        ip_name, ip_value = ip_group.split("-")
        years = startedART_df[startedART_df[ip_name] == ip_value][EventReport.year].unique()
        if any(year != start_year for year in years):
            check_startedART_resut = False
            msg.append(f"BAD: {Campaign.ARTMortalityTable} should not restart the ART "
                       f"intervention. there are {EventReport.started_art} events broadcast "
                       f"for {ip_group} in {event_report_name} at {years}, expect only at "
                       f"{start_year}.\n")
        else:
            msg.append(f"\tGOOD: There are {EventReport.started_art} events broadcast "
                       f"for {ip_group} in {event_report_name} at {years}, expect only at "
                       f"{start_year}.\n")
    if check_startedART_resut:
        msg.append(f"GOOD: {Campaign.ARTMortalityTable} does not restart the ART intervention.\n")


def get_rate_by_ARTduration(art_duration, art_duration_days_bins, art_mortality_table):
    rate = max_duration_current_bin = None
    if art_duration <= art_duration_days_bins[0]:
        rate = art_mortality_table[Campaign.mortality_table][0][0][0]
        max_duration_current_bin = art_duration_days_bins[0]
    else:
        for idx in range(1, len(art_duration_days_bins)):
            art_duration_days_bin = art_duration_days_bins[idx]
            if art_duration < art_duration_days_bin:
                rate = art_mortality_table[Campaign.mortality_table][idx][0][0]
                max_duration_current_bin = art_duration_days_bin
                break
        if not rate:
            rate = art_mortality_table[Campaign.mortality_table][-1][0][0]
    return rate, max_duration_current_bin


def data_prep(rate, duration_to_test, art_mortality_table, simulation_duration_year, art_mortality_table_start_day,
              max_duration_current_bin, art_duration, ip_group, messages, art_duration_days_bins=None, idx=None,
              move_to_next_bin=None):
    size = len(duration_to_test)
    dist_expon_scipy = stats.expon.rvs(loc=0, scale=1 / rate, size=max(100000, size))
    if rate == art_mortality_table[Campaign.mortality_table][-1][0][0]:  # last duration bin
        # make sure the simulation duration is long enough:
        max_duration = (simulation_duration_year - art_mortality_table_start_day / dtk_sft.DAYS_IN_YEAR) * 1.1
        if max(dist_expon_scipy) > max_duration:
            messages.append(f"\tWARNING: simulation_duration = {simulation_duration_year} years may "
                            f"not be long enough here. Maximum value draw from scipy exponential"
                            f" distribution is {max(dist_expon_scipy)}.\n")
            messages.append(f"\tCap theoretical data at {max_duration} years.\n")
            dist_expon_scipy = [x for x in dist_expon_scipy if x < max_duration]
    else:  # other duration bins
        # check if need to calculate next ART Duration bin
        max_duration = (max_duration_current_bin - art_duration) / dtk_sft.DAYS_IN_YEAR
        if max(dist_expon_scipy) > max_duration or max(duration_to_test) > max_duration:
            messages.append(f"\tWARNING: days until death may be greater than max_duration_current_bin"
                            f" ({max_duration_current_bin}) - duration_on_art({art_duration}). Maximum"
                            f" value draw from scipy exponential distribution"
                            f"is {max(dist_expon_scipy)}, while maximum duration "
                            f"of ip {ip_group} is {max(duration_to_test)}.\n")
            if move_to_next_bin is not None:
                temp = [x for x in duration_to_test if max_duration <= x]
                for i in range(idx, len(art_duration_days_bins) - 1):
                    next_max_duration = (max_duration +
                                     (art_duration_days_bins[i+1] - art_duration_days_bins[i])/dtk_sft.DAYS_IN_YEAR)
                    move_to_next_bin[i + 1].extend([x for x in temp if x < next_max_duration])
                    temp = [x for x in temp if x >= next_max_duration]
                move_to_next_bin[-1].extend(temp)

            messages.append(f"\tCap theoretical data and test data at {max_duration} years.\n")
            dist_expon_scipy = [x for x in dist_expon_scipy if x < max_duration]
            duration_to_test = [x for x in duration_to_test if x < max_duration]
    if move_to_next_bin is None:
        num_of_not_tested_data = size - len(duration_to_test)
        if num_of_not_tested_data > 0.1 * size:
            messages.append("BAD: too many data points are not tested, please fix the test.\n")
        messages.append(f"\tTest sample size = {len(duration_to_test)}, {num_of_not_tested_data} data points are not"
                        f" tested.\n")
    else:
        messages.append(f"\tTest sample size = {len(duration_to_test)}, {size - len(duration_to_test)} data points are "
                        f"moved to later duration bins.\n")
    return dist_expon_scipy, duration_to_test


def test_survival_time_on_duration(duration_to_test, dist_expon_scipy, rate, messages, art_duration,
                                   p=5e-2, cdf_function=stats.expon.cdf):
    result = stats.ks_2samp(duration_to_test, dist_expon_scipy)
    p_value = dtk_sft.get_p_s_from_ksresult(result)['p']
    s = dtk_sft.get_p_s_from_ksresult(result)['s']
    # result = stats.anderson_ksamp([duration_to_test, dist_expon_scipy])
    # p_value = result.pvalue
    # s = result.statistic
    msg = f"ks_2samp() with rate = {rate}(per year) return p value = " \
        f"{p_value} and statistic = {s}.\n"

    if p_value < p:
        messages.append("\tBAD: statistical test on survival duration failed! " + msg)
    else:
        messages.append("\tGOOD: statistical test on survival duration passed! " + msg)

    messages.append(f"\tPlease see survival_durations_onART_ARTDuraiton-{art_duration}.png.\n")

    # plot data, histogram and CDF with test data and theoretical data/cdf.
    dtk_sft.three_plots(duration_to_test,
                        cdf_function=cdf_function, args=(0, 1 / rate),
                        dist2=dist_expon_scipy[:len(duration_to_test)],
                        label1="Emod", label2="scipy",
                        title=f"survival_durations_onART_ARTDuraiton-{art_duration}",
                        xlabel="Data Points", ylabel="Duration(Years)",
                        category=f"survival_durations_onART_ARTDuraiton-{art_duration}",
                        show=True, sort=True)
# endregion

