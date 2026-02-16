#!/usr/bin/python
import os
if __name__ == '__main__':
    from pathlib import Path
    import sys
    os.chdir(str(Path(sys.argv[0]).parent))
    sys.path.append( str(Path('../../../../shared_embedded_py_scripts').resolve().absolute()) )

import dtk_test.dtk_sft as dtk_sft
import dtk_test.dtk_HIVDeath_Support as hiv_s
from dtk_test.dtk_StdOut import StdOut, SearchType

from scipy import stats
import math
import numpy as np


"""
This test is testing the HIV Infection to death duration. Test 1: testing the child slow progressors.

Without treatment,progression of HIV disease in infants is generally more rapid than in adults, and can be further 
sub-divided into rapid progressor sand slower progressors following a model published by Ferrand et al.
The proportion of children who are rapid progressors,α,is configured by the parameter 
HIV_Child_Survival_Rapid_Progressor_Fraction. Rapid progressors have an exponentially distributed survival time with 
decay rate β configured by the parameter HIV_Child_Survival_Rapid_Progressor_Rate. Slower progressors have a 
Weibull-distributed survival time with shape parameters configured by HIV_Child_Survival_Slow_Progressor_Shape and 
scale parameter μ configured by HIV_Child_Survival_Slow_Progressor_Scale. The Weibull distribution is a generalized 
form of the exponential distribution that is specified by two parameters, one for shape,the other for scale. 

Data to test: StdOut debug logging.

main test: test infection to death durations counter for child rapid progressors is drawn from the expected 
Ferrand weibull distribution.
The simulation runs with 15000+ population for only 2 days to collect the survival duration counter from debug logging. 
If survival scale is small, the theoretical data is generated from a custom weibull distribution(using Scipy 
rv_continuous) with cdf from Ferrand Weibull survival function. The theoretical data is continuous. If survival scale
is large, the theoretical data is generated based on how many number of death on average by the Ferrand Weibull '
survival function. The theoretical data is discrete. The test is using AD k sample method to test the survival duration 
counter with theoretical data.

Parameters to test:
            "HIV_Child_Survival_Slow_Progressor_Scale"
            "HIV_Child_Survival_Slow_Progressor_Shape"
            "HIV_Child_Survival_Rapid_Progressor_Fraction"
            "HIV_Age_Max_for_Child_Survival_Function"
            
Output: scientific_feature_report.txt
        cdf_with_theoretical_data.png
        cdf_with_FerrandWeibullCDF.png
        Theoretical_accumulated_proportion_of_death.png
        
"""


from scipy.stats import rv_continuous
class weibull_gen(rv_continuous):
    """
        define a continuous distribution with specific cdf.

        example of using this class:
            weibull = weibull_gen(name='weibull', a=0)
            dist = weibull.rvs(2, scale=16, size=1000)
    """
    def _cdf(self, x, c):
        # loc and scale are build-in parameters
        # cdf = 1 - sf
        return 1 - 2**(-pow(x, c))
    # def _sf(self, x, c):
    #     return 2 ** (-pow(x, c))


def calculate_proportion_surviving(t, shape, loc=0, scale=1):
    """
        Ferrand weibull survival function
    """
    return 2**(-pow((t-loc)/scale, shape))


def Ferrand_weibull_cdf(x, shape, loc=0, scale=1):
    """
       Cumulative distribution function based on Ferrand weibull survival function.
    """
    f = lambda a: 1 - calculate_proportion_surviving(a, shape, loc, scale)
    return np.array([f(a) for a in x])


def test_discrete_duration_get_death_proportion(infection_to_death_durations, child_slow_scale_day, child_slow_shape,
                                                simulaton_timestep, output_report_file):
    """
        1. Test whether distribution follows Ferrand Weibull Survival Function. The theoretical data is discrete with
        step = simulation_timestep.
        2. Calculate the proportion of death per time step.
    """
    # Calculate the expected duration based on the survival function from this paper:
    # Ferrand RA, Corbett EL, WoodR, et al. AIDS among older children and adolescents in Southern Africa: projecting
    # the time course and magnitude of the epidemic. AIDS 2009; 23:2039–2046
    death_proportions = [0] * 7  # duration is at least one week
    expected_durations = []
    # Calculate proportion of death and expected survival duration, iterate for every time step until we reach the
    # max duration under test
    output_report_file.write("Calculating proportion of death and expected durations based on Ferrand Weilbull"
                             " survival function:\n")
    for t in np.arange(7, int(max(infection_to_death_durations)), simulaton_timestep):
        # death_proportion = 1 - survival_proportion
        death_proportion = 1 - calculate_proportion_surviving(t, child_slow_shape, 0, child_slow_scale_day)
        # Both proportion of death and proportion of survival are cumulated.
        # Assuming there are 1,000,000 population, calculate num of death per time step.
        # We need a very large population size to get a correct theoretical result since the death probability for
        # each time step is very small(1e-7). This doesn't need to be the same as the actual simulation population.
        num_of_death = int((death_proportion - death_proportions[-1]) * 1000000)
        expected_durations.extend([t] * num_of_death)
        death_proportions.append(death_proportion)

    output_report_file.write("Plotting the proportion of death, please see "
                             "'Theoretical_accumulated_proportion_of_death.png'.\n")
    # Plot accumulated proportion of death
    dtk_sft.plot_data(death_proportions, label1="accumulated proportion of death", label2=None,
                      title="Theoretical accumulated proportion of death", xlabel="Time Step", ylabel="Proportion",
                      category='Theoretical_accumulated_proportion_of_death', show=True, sort=False)

    output_report_file.write("Statistical testing:\n")
    # Using AD k sample test.
    result = stats.anderson_ksamp(samples=[infection_to_death_durations, expected_durations])
    if hasattr(result, "pvalue"): # SciPy > v1.17
        p_value = result.pvalue
    else:
        p_value = result.significance_level
    s = result.statistic
    msg = f"anderson_ksamp() with scale = {child_slow_scale_day} and shape = {child_slow_shape} return p value = " \
        f"{p_value} and statistic = {s}.\n"
    if p_value < 5e-2:
        result = False
        output_report_file.write("\tBAD: statistical test on infection to death durations failed! " + msg)
    else:
        result = True
        output_report_file.write("\tGOOD: statistical test on infection to death durations passed! " + msg)

    output_report_file.write("Plotting cdf of test data with theoretical data and Ferrand Weilbull cdf function:\n")
    # Plot cdf of test data and theoretical data
    dtk_sft.plot_cdf(infection_to_death_durations, expected_durations, label1="infection to death durations",
                     label2="theoretical data from Ferrand Weibull survival function",
                     title='Cumulative distribution function', xlabel='survival duration', ylabel='probability',
                     category='cdf_with_theoretical_data', show=True, line=False)
    output_report_file.write("\tcdf_with_theoretical_data.png\n")

    # Plot cdf of test data and Ferrand Weibull CDF
    dtk_sft.plot_cdf_w_fun(infection_to_death_durations, name="cdf_with_FerrandWeibullCDF",
                           cdf_function=Ferrand_weibull_cdf,
                           args=(child_slow_shape, 0, child_slow_scale_day), show=True)
    output_report_file.write("\tcdf_with_FerrandWeibullCDF.png\n")

    return result, death_proportions


def test_ferrand_weibull_continuous(infection_to_death_durations, child_slow_scale_day, child_slow_shape,
                                    output_report_file):
    """
        Test whether distribution follows Ferrand Weibull Survival Function. The theoretical data is continuous.
        (This test is more accurate than test_discrete_duration_get_death_proportion() when the distribution under
        test is continuous but performance(mainly due to rv_continuous()) is slower.)
    """
    output_report_file.write("Generate expected duration using custom defined random function weibull_gen"
                             "(rv_continuous with Ferrand Weibull cdf):\n")
    weibull = weibull_gen(name='weibull', a=0)
    expected_duration = weibull.rvs(child_slow_shape,
                                    scale=child_slow_scale_day,
                                    size=max(len(infection_to_death_durations), 10000))

    # duration is at least one week
    output_report_file.write("Duration is at least one week(7 days), modify theoretical data.\n")
    if min(expected_duration) < 7:
        expected_duration = [x if x > 7 else 7 for x in expected_duration]

    output_report_file.write("Statistical testing:\n")
    # Using AD k sample test.
    result = stats.anderson_ksamp([infection_to_death_durations, expected_duration])
    if hasattr(result, "pvalue"): # SciPy > v1.17
        p_value = result.pvalue
    else:
        p_value = result.significance_level
    s = result.statistic
    msg = f"anderson_ksamp() with scale = {child_slow_scale_day} and shape = {child_slow_shape} return p value = " \
        f"{p_value} and statistic = {s}.\n"
    if p_value < 5e-2:
        result = False
        output_report_file.write("\tBAD: statistical test on infection to death durations failed! " + msg)
    else:
        result = True
        output_report_file.write("\tGOOD: statistical test on infection to death durations passed! " + msg)

    output_report_file.write("Plotting cdf of test data with theoretical data and Ferrand Weilbull cdf function:\n")
    # Plot cdf of test data and theoretical data
    dtk_sft.plot_cdf(infection_to_death_durations, expected_duration,
                     label1="infection to death durations",
                     label2="theoretical data from Ferrand Weibull survival function",
                     title='Cumulative distribution function', xlabel='survival duration', ylabel='probability',
                     category='cdf_with_theoretical_data2', show=True, line=False)
    output_report_file.write("\tcdf_with_theoretical_data2.png\n")
    # Plot cdf of test data and Ferrand Weibull CDF
    dtk_sft.plot_cdf_w_fun(infection_to_death_durations, name="cdf_with_weibull.cdf",
                           cdf_function=weibull.cdf,
                           args=(child_slow_shape, 0, child_slow_scale_day), show=True)
    output_report_file.write("\tcdf_with_weibull.cdf.png\n")

    return result


def test_standard_weilbull(durations, scale, shape, output_report_file, discrete=False,
                           name="infection_to_death_durations"):
    """
        testing a standard weibull distribution:
        If durations under test are not drawn from a standard Weibull distribution, this test will fail.
    """
    result = dtk_sft.test_weibull(durations, scale, shape,
                                  report_file=output_report_file, integer=False)

    # Plotting
    dist_weibull_scipy = stats.weibull_min.rvs(c=shape, loc=0, scale=scale,
                                               size=len(durations))
    if discrete:
        dist_weibull_scipy = [math.ceil(x) for x in dist_weibull_scipy]
    # Plot with standard scipy weibull distribution
    dtk_sft.plot_data(durations, dist_weibull_scipy,
                      sort=True, category=name, overlap=True,
                      label1=name,label2="weibull from scipy",
                      xlabel="Data Points", ylabel="Duration",
                      title=name)

    # Plot with cdf function
    dtk_sft.plot_cdf_w_fun(durations, name="weibull_cdf", cdf_function=stats.weibull_min.cdf,
                           args=(shape, 0, scale), show=True)
    return result


filter_string_list = ["Update(): Time:",
                      "days_until_death_by_HIV"]


load_df_param = [[hiv_s.Constant.ind_id, hiv_s.Constant.total_duration, hiv_s.Constant.age],
                 ["Individual id=", "days_until_death_by_HIV=", "age_years="],
                 [SearchType.VAL, SearchType.VAL, SearchType.VAL]]


def create_report_file(duration_df, total_pop, config_filename, report_name):
    succeed = True
    with open(report_name, 'w') as output_report_file:
        # Load parameters to test from config
        output_report_file.write(f"Config_Name = {dtk_sft.get_config_name(config_filename)}\n")
        simulaton_duration = dtk_sft.get_simulation_duration(config_filename)
        simulaton_timestep = dtk_sft.get_simulation_timestep(config_filename)

        child_slow_shape, child_slow_scale, max_child_age, rapid_fraction = \
            dtk_sft.get_config_parameter(config_filename,
                                         [hiv_s.ConfigParam.child_slow_shape,
                                          hiv_s.ConfigParam.child_slow_scale,
                                          hiv_s.ConfigParam.max_child_age,
                                          hiv_s.ConfigParam.rapid_fraction])

        # Convert scale to days
        child_slow_scale_day = dtk_sft.DAYS_IN_YEAR * child_slow_scale

        output_report_file.write(f"\t{hiv_s.ConfigParam.child_slow_shape} = {child_slow_shape}.\n")
        output_report_file.write(f"\t{hiv_s.ConfigParam.child_slow_scale} = {child_slow_scale} years({child_slow_scale_day} "
                                 f"days).\n")
        output_report_file.write(f"\t{hiv_s.ConfigParam.max_child_age} = {max_child_age} years.\n")

        if rapid_fraction > 0:
            succeed = False
            output_report_file.write(f"\tBAD: {hiv_s.ConfigParam.rapid_fraction} must be 0 in this test, please check the "
                                     f"test.\n")
        else:
            output_report_file.write(f"\t{hiv_s.ConfigParam.rapid_fraction} = {rapid_fraction}, so all child will be slow "
                                     f"progressors in this test.\n")

        # Get infection to death duration counters for all children.
        duration_df.sort_values(by=[hiv_s.Constant.total_duration], inplace=True)
        infection_to_death_durations = duration_df[
            duration_df[hiv_s.Constant.age] < max_child_age][
            hiv_s.Constant.total_duration].values.tolist()
        output_report_file.write(f"{len(infection_to_death_durations)} children out of {total_pop} individuals.\n")
        output_report_file.write(f"Test data point size = {len(infection_to_death_durations)}.\n")

        # Main test
        if child_slow_scale_day > 1000:
            output_report_file.write(f"{hiv_s.ConfigParam.child_slow_scale} = {child_slow_scale_day} (day) is larger than "
                                     f"1000 days, testing with discrete theoretical data. It's less accurate but "
                                     f"faster test. use it when scale under test is large.\n")
            result, death_proportions = test_discrete_duration_get_death_proportion(infection_to_death_durations,
                                                                                    child_slow_scale_day,
                                                                                    child_slow_shape,
                                                                                    simulaton_timestep,
                                                                                    output_report_file)
        else:
            output_report_file.write(f"{hiv_s.ConfigParam.child_slow_scale} = {child_slow_scale_day} (day) is less than "
                                     f"1000 days, testing with with continuous theoretical data. It's more accurate but"
                                     f" slower test. use it when scale under test is small.\n")
            result = test_ferrand_weibull_continuous(infection_to_death_durations,
                                                     child_slow_scale_day,
                                                     child_slow_shape,
                                                     output_report_file)

        # test whether the distribution is standard weilbull distribution. result is False in this test.
        # result = test_standard_weilbull(infection_to_death_durations, child_slow_scale_day, child_slow_shape,
        #                                 output_report_file, discrete=False,
        #                                 name="infection_to_death_durations")

        if not result:
            succeed = False

        output_report_file.write(dtk_sft.format_success_msg(succeed))

        return succeed


def application(output_folder="output", stdout_filename="test.txt",
                config_filename="config.json",
                report_name=dtk_sft.sft_output_filename,
                debug=False):
    if debug:
        print("output_folder: " + output_folder+ "\n")
        print("stdout_filename: " + stdout_filename + "\n")
        print("config_filename: " + config_filename + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    dtk_sft.wait_for_done(stdout_filename)
    stdout = StdOut(stdout_filename, filter_string_list, load_df_param)
    duration_df = stdout.df

    total_pop = 0
    for line in stdout.filtered_lines:
        if "Time: 1" in line:
            total_pop = int(dtk_sft.get_val("StatPop: ", line))
            break
    if total_pop <= 0:
        raise ValueError(f"total population is {total_pop}, please check the test.")

    create_report_file(duration_df, total_pop, config_filename, report_name)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt)")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-r', '--reportname', default=dtk_sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', help="debug flag", action='store_true')
    args = parser.parse_args()

    application(output_folder=args.output, stdout_filename=args.stdout,
                config_filename=args.config,
                report_name=args.reportname, debug=args.debug)

