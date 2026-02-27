#!/usr/bin/python

import json
import dtk_test.dtk_sft as sft
import math
import scipy.stats as stats
import numpy as np

class Param_keys:
    LOGNORMAL_SCALE = "Heterogeneous_Infectiousness_LogNormal_Scale"
    BASE_INFECTIVITY = "Base_Infectivity"
    SIMULATION_TIMESTEPS = "Simulation_Duration"

matches = ["heterogeneity multiplier = ", "infectiousness from HIV = "]

"""
Scale parameter of a LogNormal distribution that governs an infectiousness multiplier.
The multiplier represents heterogeneity in infectivity, and adjusts Base_Infectivity.

Based on information from Dan K, this Heterogeneous_Infectiousness_LogNormal_Scale is the
sigma(standard deviation) of the original normal distribution, when it's given, we determine
the mu of normal distribution so that mu_l of log_normal distribution is 1: mu = -sigma^2 / 2.
In dtk MathFunctions.cpp. param1 is median_l of log_normal distribution which is exp(mu).
"""
def load_emod_parameters(config_filename="config.json", debug = False):
    """
    reads config file and populates params_obj
    :param config_filename: name of config file (config.json)
    :returns param_obj:     dictionary with KEY_LOGNORMAL_SCALE, etc., keys (e.g.)
    """
    cdj = None
    with open(config_filename) as infile:
        cdj = json.load(infile)["parameters"]
    param_obj = {}
    param_obj[Param_keys.LOGNORMAL_SCALE] = cdj[Param_keys.LOGNORMAL_SCALE]
    param_obj[Param_keys.BASE_INFECTIVITY] = cdj[Param_keys.BASE_INFECTIVITY]
    param_obj[Param_keys.SIMULATION_TIMESTEPS] = cdj[Param_keys.SIMULATION_TIMESTEPS]
    if debug:
        print( param_obj )
    return param_obj

def parse_output_file(output_filename="test.txt", sim_timesteps=2, debug=False):
    """
    creates an object which contains the heterogeneity multiplier and infectiousness
    :param output_filename: file to parse (test.txt)
    :return:                output_obj:  heterogeneity multiplier and infectiousness for each infection
    """

    update_match = "Update():"
    found_last_timestep = False
    while not found_last_timestep:
        with open(output_filename) as timefile:
            update_lines = []
            for line in timefile:
                if update_match in line:
                    update_lines.append(line)
            if len(update_lines) >= sim_timesteps:
                found_last_timestep = True


    filtered_lines = []
    output_obj= {}
    for match in matches:
        output_obj[match] = []
    with open(output_filename) as logfile:
        for line in logfile:
            for match in matches:
                if match in line:
                    output_obj[match].append(float(sft.get_val(match, line)))
                    filtered_lines.append(line)
                    break

    if debug:
        with open("filtered_lines.txt", "w") as outfile:
            outfile.writelines(filtered_lines)
    return output_obj

def create_report_file(param_obj, multipliers, infectiousness, report_name, debug):
    with open(report_name, "w") as outfile:
        success = True
        if not multipliers:
            outfile.write(sft.sft_no_test_data)
        sigma = param_obj[Param_keys.LOGNORMAL_SCALE]
        base_infectivity = param_obj[Param_keys.BASE_INFECTIVITY]
        if sigma > 0:
            mu = - sigma**2 / 2.0
            # test log_normal distribution
            success = sft.test_lognorm(multipliers,mu=mu, sigma=sigma,report_file=outfile,round = False)

            # test mean_l = 1
            mean_l = np.mean(multipliers)
            mean_infectiousness = np.mean(infectiousness)
            outfile.write("mean of the multipliers is {}, expected 1.0.\n".format(mean_l))
            outfile.write("mean of the Infectiousness is {0}, while base infectivity is {1}.\n".format(mean_infectiousness,
                                                                                           base_infectivity))

            tolerance  = 3e-2
            if math.fabs(mean_l - 1.0) > tolerance:
                outfile.write("BAD: mean of the multipliers is {}, expected 1.0.\n".format(mean_l))
                success = False
            # plotting
            size = len(multipliers)
            outfile.write("size is {}\n".format(size))
            scale = math.exp(mu)
            dist_lognormal = stats.lognorm.rvs(sigma, 0, scale, size)
            sft.plot_data(multipliers, dist_lognormal,
                          label1="Emod", label2="Scipy",
                          ylabel="Multiplier", xlabel="data point",
                          category="Emod_vs_Scipy",
                          title="Emod_vs_Scipy, sigma = {}".format(sigma),
                          show=True, sort=True)
            sft.plot_probability(multipliers, dist_lognormal,
                                 precision=1, label1="Emod", label2="Scipy",
                                 category="Probability_mass_function_Emod_vs_Scipy",
                                 title="Emod_vs_Scipy, sigma = {}".format(sigma),
                                 show=True)
            sft.plot_cdf(multipliers,dist_lognormal,label1="Emod", label2="Scipy",
                                 category="cdf",
                                 title="cdf, sigma = {}".format(sigma),
                                 show=True, line = False)
            if debug:
                with open("scipy_data.txt", "w") as file:
                    for n in sorted(dist_lognormal):
                        file.write(str(n) + "\n")
                with open("emod_data.txt", "w") as file:
                    for n in sorted(multipliers):
                        file.write(str(n) + "\n")
        else:
            # sigma = 0, this feature is disabled
            for multiplier in multipliers:
                if multiplier != 1.0:
                    success = False
                    outfile.write("BAD: multiplier is {0} when {1} set to {2}, expected 1.0.\n".format(multiplier, Param_keys.LOGNORMAL_SCALE, sigma))
            # plotting
            sft.plot_data(multipliers, label1="Multiplier", label2="NA",
                          category="Multiplier", title="Multiplier_Sigma={}".format(sigma),
                          ylabel="Multiplier", xlabel="data point",
                          show=True, sort=True)
            sft.plot_data(infectiousness, label1="Infectiousness",
                          label2="NA",category="Infectiousness",
                          title="Infectiousness_Sigma={0}_BaseInfectivity={1}".format(sigma,base_infectivity),
                          ylabel="Infectiousness",xlabel="data point",
                          show=True, sort=True)
        outfile.write(sft.format_success_msg(success))

    if debug:
        print( "SUMMARY: Success={0}\n".format(success) )
    return success

def application( output_folder="output", stdout_filename="test.txt",
                 config_filename="config.json",
                 report_name=sft.sft_output_filename,
                 debug=False):
    if debug:
        print( "output_folder: " + output_folder )
        print( "stdout_filename: " + stdout_filename+ "\n" )
        print( "config_filename: " + config_filename + "\n" )
        print( "report_name: " + report_name + "\n" )
        print( "debug: " + str(debug) + "\n" )

    # sft.wait_for_done()
    param_obj = load_emod_parameters(config_filename,debug)
    output_obj = parse_output_file(output_filename=stdout_filename,
                                   sim_timesteps=param_obj[Param_keys.SIMULATION_TIMESTEPS],
                                   debug=debug)
    multipliers = output_obj[matches[0]]
    infectiousness = output_obj[matches[1]]

    create_report_file(param_obj, multipliers, infectiousness, report_name, debug)

if __name__ == "__main__":
    # execute only if run as a script
    application( "output" )
