#!/usr/bin/python

import math
import time
import re
import os
from hashlib import md5
from scipy import stats
import numpy as np
import matplotlib
from sys import platform
#if os.environ.get('DISPLAY','') == '':
if platform == "linux" or platform == "linux2":
    print('Linux OS. Using non-interactive Agg backend')
    #print('no display found. Using non-interactive Agg backend') 
    matplotlib.use('Agg')

import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import seaborn as sns # necessary for three_plots() method below
import warnings
from typing import Union
import json
from scipy.stats import gaussian_kde

"""
This module centralizes some small bits of functionality for SFT tests
to make sure we are using the same strings for messages and filenames.
"""

sft_output_filename = "scientific_feature_report.txt"
sft_no_test_data = "BAD: Relevant test data not found.\n"
sft_test_filename = "test.txt"
#SFT_EOF = "[Controller] Exiting"
SFT_EOF = "Finalizing 'InsetChart.json' reporter"
DAYS_IN_MONTH = 30
DAYS_IN_YEAR = 365
MONTHS_IN_YEAR = 12

def start_report_file(output_filename, config_name, already_started=False):
    if already_started:
        with open(output_filename, 'a') as outfile:
            outfile.write("Beginning validation for {0} at time {1}.\n".format(
                config_name,
                time.strftime("%Y-%m-%d %H:%M:%S", time.localtime())
            ))
    else:
        with open(output_filename, 'w') as outfile:
            outfile.write("Beginning validation for {0} at time {1}.\n".format(
                config_name,
                time.strftime("%Y-%m-%d %H:%M:%S", time.localtime())
            ))
    


def format_success_msg(success):
    report_msg = "SUMMARY: Success={0}\n".format(success)
    return report_msg


def add_time_stamp(time_stamp, line):
    temp_array = line.split()
    temp_array.append("time= " + str(time_stamp))
    new_line = " ".join(temp_array) + "\n"
    return new_line


def test_binomial_95ci(num_success, num_trials, prob, report_file, category):
    """
    -------------------------------------------------------
        This function test if a binomial distribution falls within the 95% confidence interval
            :param num_success: the number of successes
            :param num_trials: the number of trial
            :param prob: the probability of success for one trial
            :param report_file: for error reporting
            :param category: for error reporting
            :return: True, False
    -------------------------------------------------------
    """
    # calculate the mean and  standard deviation for binomial distribution
    mean = num_trials * prob
    standard_deviation = math.sqrt(prob * (1 - prob) * num_trials)
    # 95% confidence interval
    lower_bound = mean - 2 * standard_deviation
    upper_bound = mean + 2 * standard_deviation
    success = True
    result_message = f"For category {category}, the success cases is {num_success}," \
                     f"expected 95% confidence interval ( {lower_bound}, {upper_bound}).\n"
    if mean < 5 or num_trials * (1 - prob) < 5:
        # The general rule of thumb for normal approximation method is that
        # the sample size n is "sufficiently large" if np >= 5 and n(1-p) >= 5
        # for cases that binomial confidence interval will not work
        success = False
        report_file.write(f"There is not enough sample size in group {category}:" \
                          f"mean = {mean}, sample size - mean = {num_trials * (1 - prob)}.\n")
    elif num_success < lower_bound or num_success > upper_bound:
        success = False
        report_file.write(f"BAD:  {result_message}")
    else:
        report_file.write(f"GOOD: {result_message}")
    return success


def test_binomial_99ci(num_success, num_trials, prob, report_file, category):
    """
    -------------------------------------------------------
        This function test if a binomial distribution falls within the 99.73% confidence interval
            :param num_success: the number of successes
            :param num_trials: the number of trial
            :param prob: the probability of success for one trial
            :param report_file: for error reporting
            :param category: for error reporting
            :return: True, False
    -------------------------------------------------------
    """

    # calculate the mean and  standard deviation for binomial distribution
    mean = num_trials * prob
    standard_deviation = math.sqrt(prob * (1 - prob) * num_trials)
    # 99.73% confidence interval
    lower_bound = mean - 3 * standard_deviation
    upper_bound = mean + 3 * standard_deviation
    success = True
    result_message = f"BAD:  For category {category}, the success cases is {num_success}," \
                     f"expected 99.75% confidence interval ( {lower_bound}, {upper_bound}).\n"
    if mean < 5 or num_trials * (1 - prob) < 5:
        # The general rule of thumb for normal approximation method is that
        # the sample size n is "sufficiently large" if np >= 5 and n(1-p) >= 5
        success = False
        report_file.write(
            "There is not enough sample size in group {0}: mean = {1}, sample size - mean = {2}.\n".format(category,
                                                                                                           mean,
                                                                                                           num_trials * (
                                                                                                           1 - prob)))
    elif num_success < lower_bound or num_success > upper_bound:
        success = False
        report_file.write(f"BAD:  {result_message}")
    else:
        report_file.write(f"GOOD: {result_message}")
    return success


def calc_poisson_binomial(prob):
    """
    ------------------------------------------------------
        By definition, a Poisson binomial distribution is a sum of n independent Bernoulli distribution
        this function calculate the mean, standard deviation and variance based on probabilities from n independent Bernoulli distribution
            :param prob: list of probabilities from n independent Bernoulli distribution
            :return: mean, standard deviation and variance
    ------------------------------------------------------
    """
    mean = 0
    variance = 0
    standard_deviation = 0
    for p in prob:
        mean += p
        variance += (1 - p) * p
        standard_deviation = math.sqrt(variance)
    return {'mean': mean, 'standard_deviation': standard_deviation, 'variance': variance}


def calc_poisson_prob(mean, num_event):
    """
    ----------------------------------------------------
        This function calculates approximate probability of obeserving num_event events in an interval for a Poisson distribution
            :param mean: the average number of events per interval
            :param num_event: the number of event happened in an interval
            :return: probability of obeserving num_event events in an interval
    ----------------------------------------------------
    """
    a = math.exp(-1.0 * mean)
    b = math.pow(mean, float(num_event))
    c = math.factorial(int(num_event))
    prob = a * b / float(c)
    return prob


def plot_poisson_probability(rate, num, file, category='k vs. probability', xlabel='k', ylabel='probability',
                             label1="expected Poisson distribution", label2="test data",
                             title="Poisson Probability Mass Funciton",
                             show=True):
    """
    -----------------------------------------------------
    This function plot and save the actual and expected poisson probability and save them in a file when the error is
    larger than the tolerance
            :param rate:
            :param num:
            :param file:
            :param category:
            :param xlabel:
            :param ylabel:
            :return:
    -----------------------------------------------------
    """
    if not check_for_plotting():
        show = False

    d = {}
    x = []
    y = []
    z = []
    for n in num:
        if n in d:
            d[n] += 1
        else:
            d[n] = 1
    for n in sorted(num):
        if n in d:
            if rate < 10:
                p = calc_poisson_prob(rate, n)
                # p = stats.poisson.pmf(n,rate)
            else:
                loc = rate  # mean
                scale = math.sqrt(rate)  # standard deviation
                p = stats.norm.pdf(n, loc, scale)
            x.append(n)
            y.append(p)
            z.append(d[n] / float(len(num)))
            # file.write("Poisson Probability of {0} is {1}, expected {2}.
            # \n".format(n, p, d[n] / float(len(num_exposures_enviro))))
            if math.fabs(p - d[n] / float(len(num))) > 0.001:
                file.write("{3} : Probability for NumOfExposures {0} is {1}, expected {2}."
                           "\n".format(n, d[n] / float(len(num)), p, category))
                file.write("{3} : Count for NumOfExposures {0} is {1}, expected {2}."
                           "\n".format(n, d[n], p * float(len(num)), category))
            d.pop(n)
    fig = plt.figure()
    plt.plot(x, y, 'r--', x, z, 'bs')
    plt.title(title)
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    red_patch = mpatches.Patch(color='red', label=label1)
    blue_patch = mpatches.Patch(color='blue', label=label2)
    plt.legend(handles=[red_patch, blue_patch])
    if show:
        plt.show()
    fig.savefig(str(category) + "_rate" + str(rate) + ".png")
    plt.close(fig)
    return None


def plot_probability(dist1, dist2=None, precision=1, label1="test data", label2="scipy data",
                     title='probability density function', xlabel='k', ylabel='density', category='test', show=True,
                     line=False):
    """
    -----------------------------------------------------
    This function plot and the probability mass function of two distributions
            :param dist1:
            :param dist1:
            :param precision:
            :param title:
            :param xlabel:
            :param ylabel:
            :return:
    -----------------------------------------------------
    """

    if not check_for_plotting():
        show = False

    d = {}
    x = []
    y = []
    x2 = []
    y2 = []
    for n in sorted(dist1):
        # round n to x number of decimal value, x = precision
        i = round(n, precision)
        if i in d:
            d[i] += 1
        else:
            d[i] = 1
    for key in sorted(d):
        x.append(key)
        y.append(d[key] / float(len(dist1)))
        d.pop(key)
    fig = plt.figure()
    if line:
        color1 = 'r'
        color2 = 'b'
    else:
        color1 = 'rs'
        color2 = 'bs'
    if dist2 is not None:  # solve the future warning with comparison to 'None'.
        # if dist2: doesn't work for dataframe in Bamboo environment.
        for n in sorted(dist2):
            i = round(n, precision)
            if i in d:
                d[i] += 1
            else:
                d[i] = 1
        for key in sorted(d):
            x2.append(key)
            y2.append(d[key] / float(len(dist2)))
            d.pop(key)
        plt.plot(x, y, color1, x2, y2, color2)
    else:
        plt.plot(x, y, color1)
    # plt.plot(x, y, 'r--')
    plt.title("{0}".format(title))
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    red_patch = mpatches.Patch(color='red', label=label1)
    blue_patch = mpatches.Patch(color='blue', label=label2)
    plt.legend(handles=[red_patch, blue_patch])
    fig.savefig(str(category) + '.png')
    if show:
        plt.show()
    plt.close(fig)
    return None


def plot_hist(dist1, dist2=None, label1="test data 1", label2="test data 2", title=None, xlabel=None, ylabel=None,
              category='histogram', show=True):
    """
    -----------------------------------------------------
    This function plot and the histogram of one\two distributions
            :param dist1:
            :param dist1:
            :param title:
            :param xlabel:
            :param ylabel:
            :return:
    -----------------------------------------------------
    """

    if not check_for_plotting():
        show = False

    fig = plt.figure()
    if title:
        plt.title(title)
    if xlabel:
        plt.xlabel(xlabel)
    if ylabel:
        plt.ylabel(ylabel)
    if dist2 is not None:
        plt.hist([dist1, dist2], color=['r', 'b'], alpha=0.8)
    else:
        plt.hist(dist1, color='r', alpha=0.8)
    red_patch = mpatches.Patch(color='red', label=label1)
    blue_patch = mpatches.Patch(color='blue', label=label2)
    plt.legend(handles=[red_patch, blue_patch])
    fig.savefig(str(category) + '.png')
    if show:
        plt.show()
    plt.close(fig)
    return None


def calc_cdf(dist, num_bin=20):
    min_num = min(dist)
    max_num = max(dist)
    step = float(max_num - min_num) / num_bin
    bin_range = np.arange(min_num, max_num + step, step)
    # Use the histogram function to bin the data
    counts, bin_edges = np.histogram(dist, bins=bin_range, density=True)
    # Now find the cdf
    cdf = np.cumsum(counts)
    max_p = float(max(cdf))
    cdf = [x / max_p for x in cdf]
    return cdf, bin_edges[1:]


def plot_cdf(dist1, dist2=None, label1="test data", label2="scipy data", title='Cumulative distribution function',
             xlabel='k', ylabel='probability', category='Cumulative_distribution_function', show=True, line=False):
    """
    -----------------------------------------------------
    This function plot and the Cumulative distribution function of two distributions
            :param dist1:
            :param dist1:
            :param title:
            :param xlabel:
            :param ylabel:
            :return:
    -----------------------------------------------------
    """

    if not check_for_plotting():
        show = False

    fig = plt.figure()
    if line:
        color1 = 'r'
        color2 = 'b'
    else:
        color1 = 'ro'
        color2 = 'bo'

    num_bin = 20
    cdf, bin = calc_cdf(dist1, num_bin)

    if dist2 is None:
        plt.plot(bin, cdf, color1)
    else:
        cdf_2, bin_2 = calc_cdf(dist2, num_bin)
        plt.plot(bin, cdf, color1, bin_2, cdf_2, color2)

    plt.title("{0}".format(title))
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    red_patch = mpatches.Patch(color='red', label=label1)
    blue_patch = mpatches.Patch(color='blue', label=label2)
    plt.legend(handles=[red_patch, blue_patch])
    # plt.ylim((0.0, 1.01))
    fig.savefig(str(category) + '.png')
    if show:
        plt.show()
    plt.close(fig)
    return None


def check_for_plotting():
    homepath = None
    if os.environ.get("HOME"):
        homepath = os.getenv("HOME")
    elif os.environ.get("HOMEDRIVE"):
        homepath = os.path.join(os.getenv("HOMEDRIVE"), os.getenv("HOMEPATH"))
    else:
        # HPC case: if none of the env vars are present, we're on the HPC and won't plot anything.
        return False

    if (os.path.exists(os.path.join(homepath, ".rt_show.sft"))):
        return True
    else:
        return False

def plot_scatter_fit_line(dist1, dist2=None, label1="test data 1", label2=None, title=None,
                          xlabel=None, ylabel=None, xmin=None, xmax=None, ymin=None, ymax=None,
                          category='plot_scatter_fit_line', show=True, line=False, marker='s',
                          xticks=None, xtickslabel=None):
    """
    -----------------------------------------------------
    This function plot the dist1 data with color based on the spatial density of nearby points. If dist2 is provided,
    it plot dist2 data as fit data/line for dist1.
    -----------------------------------------------------
    """
    from mpl_toolkits.axes_grid1 import make_axes_locatable

    if not check_for_plotting():
        show = False

    fig, ax = plt.subplots()

    if title:
        ax.set_title(title)
    if xlabel:
        ax.set_xlabel(xlabel)
    if ylabel:
        ax.set_ylabel(ylabel)
    if xmin and xmax:
        ax.set_xlim(xmin, xmax)
    else:
        if xmax:
            ax.set_xlim(xmax=xmax)
        if xmin:
            ax.set_xlim(xmin=xmin)
    if ymin and ymax:
        ax.set_ylim(ymin, ymax)
    else:
        if ymax:
            ax.set_ylim(ymax=ymax)
        if xmin:
            ax.set_ylim(ymin=ymin)

    color = 'k'+marker
    if line:
        color += '-'

    # Generate data for scatter plot
    y = dist1
    x = np.arange(len(y))

    # Calculate the point density
    xy = np.vstack([x, y])
    z = gaussian_kde(xy)(xy)

    # Sort the points by density, so that the densest points are plotted last
    idx = z.argsort()
    x, y, z = x[idx], y[idx], z[idx]

    # scatter = ax.scatter(x, y, c=z, s=20000 * z, edgecolor='', label=label1, cmap="rainbow", alpha=0.5)
    scatter = ax.scatter(x, y, c=z, s=100, edgecolor=['none'], label=label1, cmap="rainbow", alpha=0.5)
    divider = make_axes_locatable(ax)
    cax = divider.append_axes('right', size='5%', pad=0.05)
    cax.set_ylabel("spatial density of nearby points")

    fig.colorbar(scatter, cax=cax, orientation='vertical')


    if dist2 is not None: # "if dist2:" will not work with numpy.ndarray
        # use plot instead of scatter for more efficiency here.
        ax.plot(dist2, color, label=label2, lw=0.5, markersize=3)
    ax.legend(loc=0)

    if xticks is not None:
        ax.set_xticks(xticks)
        if xtickslabel is not None:
            ax.set_xticklabels(xtickslabel, fontsize=8, rotation=20, rotation_mode="anchor", ha='right')

    fig.tight_layout()
    fig.savefig(str(category) + '.png')
    if show:
        plt.show()
    plt.close(fig)
    return None


def test_gamma(dist, shape_k, scale_theta, report_file=None):
    # generate a gamma distribution with numpy and compare to what we're getting
    size = len(dist)
    numpy_gamma = np.random.gamma(shape_k, scale_theta, size)

    result = stats.ks_2samp(numpy_gamma, dist)
    p = get_p_s_from_ksresult(result)['p']
    s = get_p_s_from_ksresult(result)['s']
    critical_value_s = calc_ks_critical_value(size)

    # return p >= 5e-2 or s <= critical_value_s
    if p >= 5e-2 or s <= critical_value_s:
        if report_file:
            report_file.write(
                f"GOOD: Distribution given probably is gamma with statistic={s}, pvalue={p},"
                " expected s less than {critical_value_s} and p larger than 0.05.\n")
        return True
    else:
        if report_file:
            report_file.write(
                f"BAD: Distribution given probably not gamma with statistic={s}, pvalue={p},"
                " expected s less than {critical_value_s} and p larger than 0.05.\n")
        return False


def plot_data(dist1, dist2=None, label1="test data 1", label2=None, title=None,
              xlabel=None, ylabel=None, xmin=None, xmax=None, ymin=None, ymax=None,
              category ='plot_data', show=True, line=False,alpha=1, overlap=False, marker1='s', marker2='o',
              sort=False, xticks=None, xtickslabel=None):
    """
    -----------------------------------------------------
    This function plot the data of one\two distributions
            :param dist1:
            :param dist1:
            :param title:
            :param xlabel:
            :param ylabel:
            :return:
    -----------------------------------------------------
    """
    if not check_for_plotting():
        show = False

    if sort: # use not in-place method
        dist1 = sorted(dist1)
        if dist2 is not None:
            dist2 = sorted(dist2)

    fig = plt.figure()
    ax= fig.add_axes([0.12,0.15,0.76,0.76])
    if title:
        ax.set_title(title)
    if xlabel:
        ax.set_xlabel(xlabel)
    if ylabel:
        ax.set_ylabel(ylabel)
    if xmin and xmax:
        ax.set_xlim(xmin, xmax)
    else:
        if xmax:
            ax.set_xlim(xmax=xmax)
        if xmin:
            ax.set_xlim(xmin=xmin)
    if ymin and ymax:
        ax.set_ylim(ymin, ymax)
    else:
        if ymax:
            ax.set_ylim(ymax=ymax)
        if xmin:
            ax.set_ylim(ymin=ymin)
    if overlap:
        color1 = 'r'+marker1
        color2 = 'b'+marker2
    else:
        color1 = 'y'+marker1
        color2 = 'g'+marker2
    if line:
        color1 += '-'
        color2 += '-'
    ax.plot(dist1, color1, alpha=alpha, label=label1, lw= 0.5, markersize=4)
    if dist2 is not None: # "if dist2:" will not work with numpy.ndarray
        plt.plot(dist2, color2, alpha=alpha, label=label2, lw=0.5, markersize=3)
    ax.legend(loc=0)

    if xticks is not None:
        ax.set_xticks(xticks)
        if xtickslabel is not None:
            ax.set_xticklabels(xtickslabel, fontsize=8, rotation=20, rotation_mode="anchor", ha='right')

    fig.savefig(str(category) + '.png')
    if show:
        plt.show()
    plt.close(fig)
    return None

def plot_pie(sizes, labels, category='plot_pie', show=True):
    """
    plot a pie chart based on sizes
    :param sizes:
    :param labels:
    :param show:
    :return:
    """
    plt.pie(sizes, labels=labels, autopct='%1.1f%%', shadow=True, startangle=140)
    plt.axis('equal')
    plt.savefig(str(category) + '.png')
    if show:
        plt.show()
    plt.close()
    return None


def plot_scatter_with_fit_lines(dataframe, xlabel, ylabel, fit_data_segments,
                                est_data_segments, fit_data_label="fits to data",
                                est_data_label="estimated fits", category="plot_data_fits",
                                show=True):
    """
    Plots a scatterplot of data, as well as fitted and estimated segments. This is written with
    Immunity Initialization in mind
    :param dataframe:
    :param xlabel: What is the x-axis? For example, 'age'
    :param ylabel: what is the y-axis? For example, 'mod_acquire'
    :param fit_data_segments: array of data-fit line segments in this format ([startx, endx],[starty, endy])
    :param est_data_segments: array of esimated (ideal) line segments as above
    :param fit_data_label: ("fits to data")
    :param est_data_label: ("esitmated fits")
    :param category: name of file without extension ("plot_data_fits")
    :param show:
    :return:
    """
    if not check_for_plotting():
        return
    fig = plt.figure()
    plt.scatter(dataframe[xlabel], dataframe[ylabel], s=20, alpha=0.02, lw=0)
    for segment in est_data_segments:
        plt.plot(segment[0], segment[1], 'r') # Red is reference, or expected data
    for segment in fit_data_segments:
        plt.plot(segment[0], segment[1], 'b') # Blue is data under test
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    fig.savefig(str(category) + '.png')
    if show:
        plt.show()
    plt.close(fig)


def calc_ks_critical_value(num_trials):
    """
    -----------------------------------------------------
    This function returns the critical values for kstest Statistic test based on KS table (reference: http://www.cas.usf.edu/~cconnor/colima/Kolmogorov_Smirnov.htm)
            :param num_trials: number of trials = length of distribution
            :return: critical values
    -----------------------------------------------------
    """
    # KS table
    ks_table = [0.975, 0.842, 0.708, 0.624, 0.565, 0.521, 0.486, 0.457, 0.432, 0.410, 0.391, 0.375, 0.361, 0.349, 0.338,
                0.328, 0.318, 0.309, 0.301, 0.294, 0.270, 0.240, 0.230]
    critical_value_s = 0
    if num_trials <= 20:
        critical_value_s = ks_table[num_trials - 1]
    elif num_trials <= 25:
        critical_value_s = ks_table[20]
    elif num_trials <= 30:
        critical_value_s = ks_table[21]
    elif num_trials <= 35:
        critical_value_s = ks_table[22]
    else:
        critical_value_s = 1.36 / math.sqrt(num_trials)
    return critical_value_s


def get_p_s_from_ksresult(result):
    # print( str( result ) )
    p = s = 0
    # NOTE: different versions of kstest seem to produce different output.
    if "pvalue" in result:
        p = float(get_val("pvalue=", str(result)))
        s = float(get_val("statistic=", str(result)))
    else:
        s = result[0]
        p = result[1]
        # report_file.write("s is {0}, p is : {1} for {2}.\n".format(s, p, category))
    return {'p': p, 's': s}


def create_geometric_dis(rate, scale, size, test_decay=True):
    curr_change = 0
    curr_count = scale
    series = []
    for _ in range(size):
        curr_count -= curr_change
        curr_change = math.floor(curr_count * rate)
        if test_decay:
            series.append(curr_count)
        else:
            series.append(scale - curr_count)
    return series

# Ye: I think this is not testing a real Geometric distribution, please see test_geometric().
def test_geometric_decay(distribution_under_test, rate, scale, test_decay=True, report_file=None, debug=False):
    """
    Tests if the given distribution is a geometric distribution
    with the given rate and scale
    :param distribution_under_test: array of integers to test against
    :param rate:  rate per timestep at which the value decays
    :param scale:  number of things to decay
    :param test_decay: True assumes that your distribution is being decayed, False assumes it is growing
    :return:
    """
    size = len(distribution_under_test)
    series = create_geometric_dis(rate, scale, size, test_decay)

    result = stats.ks_2samp(series, distribution_under_test)
    if debug and report_file:
        report_file.write(str(result) + "\n")

    p = get_p_s_from_ksresult(result)['p']
    s = get_p_s_from_ksresult(result)['s']

    critical_value_s = calc_ks_critical_value(len(distribution_under_test))
    report_file.write("distribution under test\n")
    report_file.write(str(distribution_under_test) + "\n")
    report_file.write("series I made\n")
    report_file.write(str(series) + "\n")
    if p >= 5e-2 or s <= critical_value_s:
        success = True
    else:
        if report_file is not None:
            report_file.write(
                "BAD: Two sample kstest result for {0} is: statistic={1}, pvalue={2}, expected s less than {3} and p larger than 0.05.\n".format(
                    "geometric decay", s, p, critical_value_s))
        success = False
    return success


def convert_remains_to_binomial_chain(remains):
    """
    Convert the remaining # of binomial trials at each time step into # of success cases at each time step. For example:
    with binomial prob = 0.1, initial trials = 100 and total time step = 10, we have:
    remains = <class 'list'>: [100, 90, 81, 73, 66, 60, 54, 49, 45, 41]
    binomial_chain = <class 'list'>: [10, 9, 8, 7, 6, 6, 5, 4, 4]
    Args:
        remains: the remaining # of binomial trials at each time step

    Returns: binomial_chain

    """
    binomial_chain = []
    for i in range(len(remains) - 1):
        binomial_chain.append(remains[i] - remains[i + 1])
    return binomial_chain


def convert_binomial_chain_to_geometric(binomial_chain):
    """
    Convert an array of binomial trials results into # of trials before first success, which is a Geometric distribution.
    Example:
        binomial_chain = <class 'list'>: [10, 9, 8, 7, 6, 6, 5, 4, 4]
        geom = <class 'list'>: [1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 8, 8, 8, 8, 9, 9, 9, 9]
    Args:
        binomial_chain: array of binomial trials results(# of success at each timestep)

    Returns: geom

    """
    geom = []
    for i in range(len(binomial_chain)):
        for _ in range(binomial_chain[i]):
            # index i starts at 0 but Geometric distribution are from set{1, 2, 3, ...}(shifted geometric distribution)
            geom.append(i+1)
    return geom


def test_geometric(distribution_under_test, prob, report_file=None, category=None):
    """
    kstest for Geometric distribution. (note this will have a lower type 1 error, please see "Type 1 Error" section
    for more details)

    In the egg hatching scenario, the distribution_under_test should be an array that contains the
    duration(timesteps) until hatch for each egg. If simulation starts at time 0, then distribution_under_test
    = [1] * # of eggs hatch at day 1 + [2] * # of eggs hatch at day 2 + ... + [i] * # of eggs hatch at day i.

    Type 1 Error:
        The kstest assumes the distribution under test is continuous. Since Geometric distribution is discrete, we will
        get a true type 1 error lower than the one we choose(5%). But depending on "how discrete" the distribution is,
        the type 1 error might be close to 5%. The following tests were performed to get an idea of
        the accuracy of this test:
            Test 1:
                distribution_under_test = stats.geom.rvs(p=0.1, size=100000)
                test_geometric(distribution_under_test, prob=0.1)
                Ran this test 1000 times and it failed 28 times.(Type 1 error is ~3% in this case.)
            Test 2:
                distribution_under_test = stats.geom.rvs(p=0.11, size=100000)
                test_geometric(distribution_under_test, prob=0.1)
                Ran this test 1000 times and it failed 1000 times.
            Test 3:
                distribution_under_test = stats.geom.rvs(p=0.101, size=100000)
                test_geometric(distribution_under_test, prob=0.1)
                Ran this test 1000 times and it failed 327 times.

    Args:
        distribution_under_test: the distribution to be tested
        prob: success probability for each trial
        report_file: file to write error reporting
        category: name of test for error reporting
    Returns: True, False
    """

    geom_dist = np.random.geometric(p=prob, size=len(distribution_under_test))

    result = stats.ks_2samp(distribution_under_test, geom_dist)

    p = get_p_s_from_ksresult(result)['p']
    s = get_p_s_from_ksresult(result)['s']

    critical_value_s = calc_ks_critical_value(size)

    msg = f"Geometric kstest result for {category} is: statistic={s}, p_value={p}, " \
          f"expected statistic less than {critical_value_s} and p_value larger than 0.05.\n"
    if p >= 5e-2 or s <= critical_value_s:
        if report_file is not None:
            report_file.write("GOOD: " + msg)
        return True
    else:
        if report_file is not None:
            report_file.write("BAD: " + msg)
        return False


def test_poisson(trials, rate, report_file=None, route=None, normal_approximation=True):
    """
    -----------------------------------------------------
        This function test if a distribution is a Poisson distribution with given rate
        For rate < 10, I am testing it using the two-sample kstest since one-sample kstest test for poisson distribution has a bug
            previously, I am test it based on probability of events for a Possion distribution, this test is too sensitive and may not be a good fit in our case
        For rate >= 10, I am using Normal Approximation with continuity correction
        If normal_approximation = False, I am testing it using the two-sample kstest
            :param trials: distribution to test, it contains values 0, 1, 2, ...
            :param rate: the average number of events per interval
            :param report_file:
            :param route:
            :param normal_approximation: if false, don't use normal approximation for rate >=10, only use poisson kstest for all rates.
            :return: True/False
    -----------------------------------------------------
    """
    success = True
    size = len(trials)
    if not normal_approximation or rate < 10:
        # rate less than 10 is calculated as a Poisson Draw
        # ps is Poisson distribution
        ps = np.random.poisson(rate, size)
        result = stats.ks_2samp(ps, trials)

        p = get_p_s_from_ksresult(result)['p']
        s = get_p_s_from_ksresult(result)['s']

        critical_value_s = calc_ks_critical_value(size)
        success = p >= 5e-2 or s <= critical_value_s
        result_string = "GOOD" if success else "BAD"
        if report_file is not None:
            report_file.write(
                "{0}: Poisson two sample kstest result for {1} is: statistic={2}, pvalue={3}, expected s less "
                "than {4} or p larger than 0.05.\n".format(result_string, route, s, p, critical_value_s)
            )
    else:
        # rate >= 10 is using Normal Approximation with continuity correction
        # stats.kstest(trials, 'norm', args=(loc, scale)) will fail since kstest is too sensitive for this approximation
        d = {}
        count = 0
        for n in sorted(trials):
            if d.get(n):
                d[n] += 1
            else:
                d[n] = 1
        for key in d:
            count += d[key]
            loc = rate  # mean
            scale = math.sqrt(rate)  # standard deviation
            p = stats.norm.cdf(key + 0.5, loc, scale)  # continuity correction
            actual_p = (count / float(size))  # calculate actual cumulative probability
            success = math.fabs(p - actual_p) <= 0.02
            result_string = "GOOD" if success else "BAD"
            if report_file is not None:
                report_file.write(
                    "{0}: Poisson cumulative probability for {1} and {2} is {3}, expected {4}.\n".format(result_string,
                                                                                                         route,
                                                                                                         key,
                                                                                                         actual_p,
                                                                                                         p)) 
    return success


def test_lognorm(timers, mu, sigma, report_file=None, category=None, round=False):
    """
    -----------------------------------------------------
        kstest for lognormal distribution
            :param timers: the distribution to test
            :param mu: mean, which is equal to sqrt(scale)
            :param sigma: the standard deviation
            :param report_file: for error reporting
            :param category: for error reporting
            :return: True, False
    -----------------------------------------------------
    """
    # print( "Running test_lognorm for " + category )
    scale = math.exp(mu) # median
    size = len(timers)
    dist_lognormal = stats.lognorm.rvs(sigma, 0, scale, size)
    # dist_lognormal = np.random.lognormal(mu, sigma, size)

    if round:
        dist_lognormal_2 = []
        for n in dist_lognormal:
            dist_lognormal_2.append(round_to_n_digit(n, 7))
        result = stats.ks_2samp(dist_lognormal_2,
                                timers)  # switch to 2 sample kstest so I can round the data from scipy
    else:
        result = stats.kstest(timers, 'lognorm', args=(sigma, 0, scale))

    p = get_p_s_from_ksresult(result)['p']
    s = get_p_s_from_ksresult(result)['s']

    critical_value_s = calc_ks_critical_value(size)

    if p >= 5e-2 or s <= critical_value_s:
        if report_file is not None:
            report_file.write(
                "GOOD: log normal kstest result for {0} is: statistic={1}, pvalue={2}, expected s less than {3} and p larger than 0.05.\n".format(
                    category, s, p, critical_value_s))
        return True
    else:
        if report_file is not None:
            report_file.write(
                "BAD: log normal kstest result for {0} is: statistic={1}, pvalue={2}, expected s less than {3} and p larger than 0.05.\n".format(
                    category, s, p, critical_value_s))
        # print("BAD: log normal kstest result for {0} is: statistic={1}, pvalue={2}, expected s less than {3} and p larger than 0.05.\n".format(category, s, p, critical_value_s))
        return False


def test_uniform(dist, p1, p2, report_file=None, round=False, significant_digits=7):
    """
     -----------------------------------------------------
        kstest for uniform distribution
            :param p1: loc
            :param p2: loc + scale
            :param dist: The distribution to be tested
            :return: True, False
    -----------------------------------------------------
    """
    loc = p1
    scale = p2 - p1
    size = len(dist)
    dist_uniform_scipy = stats.uniform.rvs(loc, scale, size)
    # dist_uniform_np = np.random.uniform(p1, p2, size)
    if round:
        dist_uniform_scipy_r = []
        for s in dist_uniform_scipy:
            dist_uniform_scipy_r.append(round_to_n_digit(s, significant_digits))
        result = stats.ks_2samp(dist_uniform_scipy_r, dist)
    else:
        result = stats.ks_2samp(dist_uniform_scipy,dist)
        # result = stats.kstest(dist, 'uniform', args=(loc, scale)) this is not compatible for Python 3, only Python 2 works
    p = get_p_s_from_ksresult(result)['p']
    s = get_p_s_from_ksresult(result)['s']
    critical_value_s = calc_ks_critical_value(size)

    # return p >= 5e-2 or s <= critical_value_s
    if p >= 5e-2 or s <= critical_value_s:
        return True
    else:
        if report_file is not None:
            report_file.write(
                "BAD: ({0},{1})failed with statistic={2}, pvalue={3}, expected s less than {4} and p larger than 0.05.\n".format(
                    p1, p2, s, p, critical_value_s))
        return False


def test_gaussian_chisquare(dist, p1, p2, report_file=None):
    """
    Note: Currently used to test usage duration with Gaussian distribution.
    Bins (categories):
        1. gaps between two consecutive integers (e.g. 23 to 24) if the expected value in the bin > 5
        2. if the expected value in the bin < 5, the bin is combined with the bin next to it iteratively until the total
           expected value of the combined bins is greater than 5
    This way of splitting the bins is currently not working.
    But need more caution on how to split the bins. e.g. Should we just ignore the tails and group the tails to a bin?
    """
    import collections
    counter = sorted(collections.Counter(dist).items(), key = lambda kv: (kv[0]))
    observed = [item[1] for item in counter]
    expected = []
    for key in [item[0] for item in counter]:
        expected.append((stats.norm.cdf(key, p1, p2) - stats.norm.cdf(key - 1, p1, p2))*len(dist))
    # want each expected value to be at least 5
    observed_at_least_min, expected_at_least_min = collapse_to_at_least_min(observed, expected, 5)

    # scipy release wants things "normalized" https://github.com/scipy/scipy/issues/12282
    obs_sum = sum(observed_at_least_min)
    obs_norm = [obs / obs_sum for obs in observed_at_least_min]
    exp_sum = sum(expected_at_least_min)
    exp_norm = [exp / exp_sum for exp in expected_at_least_min]

    result = stats.chisquare(obs_norm, exp_norm, ddof=0)
    s = tuple(result)[0]
    p = tuple(result)[1]

    result_string = f"with statistic={s}, pvalue={p}, expected p larger than 0.05.\n"
    if p >= 5e-2:
        if report_file is not None:
            report_file.write(f"GOOD: ({p1}, {p2}) passed {result_string}")
        return True
    else:
        if report_file is not None:
            report_file.write(f"BAD: ({p1}, {p2}) failed {result_string}")
        return False


def test_gaussian(dist, p1, p2, allow_negative=True, report_file=None, round=False):
    """
     -----------------------------------------------------
        kstest for gaussian distribution
            :param dist: The distribution to be tested
            :param p1: mean, loc
            :param p2: width(standard deviation), scale
            :param allow_negative: allow negative value in normal distribution, if False, turn all negative value to 0.0
            :param round: True to round the theoretical distribution to 7 significant digits.
            :return: True, False
    -----------------------------------------------------
    """
    size = len(dist)
    if round or not allow_negative:
        # dist_gaussian_np = np.random.normal(p1, p2, size)
        dist_gaussian_scipy = stats.norm.rvs(p1, p2, size)
        dist_gaussian_scipy2 = []
        for n in dist_gaussian_scipy:
            if (not allow_negative) and n < 0:
                n = 0
            if round:
                dist_gaussian_scipy2.append(round_to_n_digit(n, 7))
            else:
                dist_gaussian_scipy2.append(n)
        result = stats.ks_2samp(dist_gaussian_scipy2, dist)
    else:
        result = stats.kstest(dist, "norm", args=(p1, p2))

    p = get_p_s_from_ksresult(result)['p']
    s = get_p_s_from_ksresult(result)['s']
    critical_value_s = calc_ks_critical_value(size)

    # return p >= 5e-2 or s <= critical_value_s
    if p >= 5e-2 or s <= critical_value_s:
        return True
    else:
        if report_file is not None:
            report_file.write(
                "BAD: ({0},{1})failed with statistic={2}, pvalue={3}, expected s less than {4} and p larger than 0.05.\n".format(
                    p1, p2, s, p, critical_value_s))
        return False


def round_down(num, precision):
    multiplier = math.pow(10.0, precision)
    return math.floor(num * multiplier) / multiplier


def round_up(num, precision):
    multiplier = math.pow(10.0, precision)
    return math.ceil(num * multiplier) / multiplier


def test_exponential(dist, p1, report_file=None, integers=False, roundup=False, round_nearest=False):
    """
     -----------------------------------------------------
        kstest for exponential distribution
            :param p1: decay rate = 1 / decay length , lambda, >0,
            :param dist: The distribution to be tested
            :param report_file: report file to which write the error if such exists
            :param integers: Indicates whether the distribution is rounded up or down to integers or not
            :return: True, False
    -----------------------------------------------------
    """
    size = max(len(dist), 10000)
    scale = 1.0 / p1
    if integers:
        dist_exponential_np = np.random.exponential(scale, size)
        if round_nearest:
            dist_exponential_np = [round(x) for x in dist_exponential_np]
        elif roundup:
            dist_exponential_np = [round_up(x, 0) for x in dist_exponential_np]
        else:
            dist_exponential_np = [round_down(x, 0) for x in dist_exponential_np]

        result = stats.anderson_ksamp([dist, dist_exponential_np])
        if hasattr(result, "pvalue"):  # SciPy > v1.17
            p= result.pvalue
        else:
            p = result.significance_level
        s = result.statistic
    else:
        # loc = 0
        # dist_exponential_scipy = stats.expon.rvs(loc, scale, size)
        # result = stats.ks_2samp(dist_exponential_np, list(dist))
        result = stats.kstest(dist, "expon", args=(0, scale))

        p = get_p_s_from_ksresult(result)['p']
        s = get_p_s_from_ksresult(result)['s']
        # critical_value_s = calc_ks_critical_value(size)
        # if p >= 5e-2 or s <= critical_value_s:
    if p >= 5e-2:
        if report_file is not None:
            report_file.write(
                "GOOD: ({0})succeed with statistic={1}, pvalue={2}, expected p larger "
                "than 0.05.\n".format(p1, s, p))
        return True
    else:
        if report_file is not None:
            report_file.write("BAD: ({0})failed with statistic={1}, pvalue={2}, expected p larger "
                              "than 0.05.\n".format(p1, s, p))
        return False


def collapse_to_at_least_min(observed, expected, min_expected=5):
    """
    Iterates through expected and observed results, combine bins in the expected array that are less than min_expected
      to their neighbours until their sum is 5 or more. Combine the corresponding bins in the observed array as well.
    :param observed: an array of observed occurrence of each unique usage duration expiration time step in the
     simulation, with the time steps sorted in increasing ordered
    :param expected: an array of expected occurrence of each unique usage duration expiration time step in the
     simulation, with the time steps sorted in increasing ordered
    :return:
    """
    expected_at_least_min = []
    observed_at_least_min = []
    i = 0
    while i < len(expected):
        tmp_e = expected[i]
        tmp_o = observed[i]
        while tmp_e < min_expected and i < len(expected) - 1:
            i = i + 1
            tmp_e = tmp_e + expected[i]
            tmp_o = tmp_o + observed[i]
        expected_at_least_min.append(tmp_e)
        observed_at_least_min.append(tmp_o)
        i = i + 1
    return observed_at_least_min, expected_at_least_min


def test_bimodal(dist, p1, p2, report_file=None):
    """
     -----------------------------------------------------
        Test for bimodal distribution. This bimodal distribution is not a true bimodal distribution, which is defined as the overlap of two Gaussians.
        The definition of bimodal in DTK mathfunctions.cpp is a function that gives an output of either 1 or the value of param2. The param1 controls the fraction.
            :param p1: faction of param 2
            :param p2: multiplier
            :param dist: The distribution to be tested
            :return: True, False
    -----------------------------------------------------
    """
    size = len(dist)
    count1 = 0
    count2 = 0
    for n in dist:
        if n == 1.0:
            count1 += 1
        elif n == p2:
            count2 += 1
        else:
            if report_file is not None:
                report_file.write(
                    "BAD: Binomal distribution contains value = {0}, expected 1.0 or {1}.\n".format(n, p2))
            return False
    actual_faction = count2 / float(size)
    if math.fabs(p1 - actual_faction) <= 5e-2:
        return True
    else:
        if report_file is not None:
            report_file.write(
                "BAD: test Binomal failed with actual fraction = {0}, expected {1}.\n".format(actual_faction, p1))
        return False


def test_weibull(dist, p1, p2, report_file=None, round=False, integer=False):
    """
     -----------------------------------------------------
        kstest for weibull distribution
            :param p1: scale, lambda > 0
            :param p2: shape, kappa > 0
            :param dist: The distribution to be tested
            :return: True, False
    -----------------------------------------------------
    """
    size = max(len(dist), 10000)
    # s = np.random.weibull(p2, size)
    # dist_weibull_np = map(lambda x : x * p1, s)

    if round or integer:
        dist_weibull_scipy = stats.weibull_min.rvs(c=p2, loc=0, scale=p1, size=size)
        dist_weibull_scipy2 = []
        for n in dist_weibull_scipy:
            if integer:
                dist_weibull_scipy2.append(math.ceil(n))
            else:
                dist_weibull_scipy2.append(round_to_n_digit(n, 7))
        result = stats.anderson_ksamp([dist, dist_weibull_scipy2])
        if hasattr(result, "pvalue"):  # SciPy > v1.17
            p = result.pvalue
        else:
            p = result.significance_level
        s = result.statistic
    else:
        # update to use one sample ks test with weibull cdf function.
        result = stats.kstest(dist, "weibull_min", (p2, 0, p1))
        p = get_p_s_from_ksresult(result)['p']
        s = get_p_s_from_ksresult(result)['s']

    # return p >= 5e-2 or s <= critical_value_s
    msg = "({0},{1}) test returns statistic={2} and p value={3}, expected p larger than 0.05.\n".format(
        p1, p2, s, p)
    if p >= 5e-2:
        if report_file is not None:
            report_file.write("GOOD: " + msg)
        return True
    else:
        if report_file is not None:
            report_file.write("BAD: " + msg)
        return False

def test_multinomial(dist, proportions, report_file=None, prob_flag=True):
    """
    Chi-squared test for multinomial data
    :param dist: array_like, number in each categories
    :param p: array_like, proportions in each categories
    :param report_file:
    :param report_file: flag that indicates whether p are proportions or the expected values
    :return: True or False for test result
    """
    if prob_flag:
        n = sum(dist)
        prob = sum(proportions)
        total = int(n/prob)
        result = stats.chisquare(dist, np.array(proportions) * total, ddof=0 ) #returns chi-square statistic and p value
    else:
        # 6/2021 scipy release wants things "normalized" https://github.com/scipy/scipy/issues/12282
        dist_sum = sum(dist)
        dist_norm = [d / dist_sum for d in dist]
        prop_sum = sum(proportions)
        prop_norm = [p / prop_sum for p in proportions]
        result = stats.chisquare(dist_norm, prop_norm, ddof=0)
        
    p = get_p_s_from_ksresult(result)['p']
    s = get_p_s_from_ksresult(result)['s']
    if p >= 5e-2:
        if report_file is not None:
            report_file.write(
                "GOOD: Chi-squared test for multinomial data passed with statistic={0}, pvalue={1}, expected p larger"
                " than 0.05.\ndata for test is {2} and proportion is {3}.\n".format(s, p, dist, proportions))
        return True
    else:
        if report_file is not None:
            report_file.write(
                "BAD: Chi-squared test for multinomial data failed with statistic={0}, pvalue={1}, expected p larger"
                " than 0.05.\ndata for test is {2} and proportion is {3}.\n".format(s, p, dist, proportions))
        return False

def is_stats_test_pass(fail_count, pass_count, report_file=None):
    """
    -------------------------------------------------------
        This function determine whether a set of statistic tests(kstest basically) pass
        The threshold for p value to determine whether the ks test pass is hardcoded as 0.05
            :param fail_count: total number of failing statistic tests
            :param pass_count: total number of passing statistic tests
            :param report_file: optional
            :return: True, False
    -------------------------------------------------------
    """
    count = fail_count + pass_count
    # hardcoded threshold for p value
    threshold_pvalue = 5e-2
    # probability for small probability event
    small_prob_event = 1e-3
    # this is the theoretical average failing count
    mean_pvalue = count * threshold_pvalue

    if fail_count < mean_pvalue:
        if report_file is not None:
            report_file.write("mean_pvalue = {0}, fail_count = {1}.\n".format(mean_pvalue, fail_count))
        return True
    else:
        # could use Normal Approximation with continuity correction (mean > 25) and 6 sigma rule.
        # calculate the cummulative density function
        prob = stats.poisson.cdf(fail_count - 1, mean_pvalue)
        if report_file is not None:
            report_file.write(
                "prob = {0}, mean_pvalue = {1}, fail_count = {2}.\n".format(prob, mean_pvalue, fail_count))
        # test passes when the survival function (1 - cdf) >= probability for samll probability event(0.001, 0.01 or 0.05)
        # higher value is stricter than lower value
        # if <=, which means the small probability event happens in real life so we determine the test fails
        return (1.0 - prob) >= small_prob_event


def round_to_1_digit(x):
    """
    -----------------------------------------------------
        Round number x to 1 significant digit
            :param x: number to be rounded
            :return:
    -----------------------------------------------------
        """
    if x == float("inf") or x == float("-inf") or x == 0.0:
        return x
    # elif math.fabs(x - 0.0) < 1e-1:
    #     return 0.0
    else:
        return round(x, -int(math.floor(math.log10(abs(x)))))


def round_to_n_digit(x, n):
    """
    -----------------------------------------------------
        Round number x to n significant digits
            :param x: number to be rounded
            :param n: # of significant digits
            :return:
    -----------------------------------------------------
    """
    if x == float("inf") or x == float("-inf") or x == 0.0:
        return x
    else:
        return round(x, -int(math.floor(math.log10(abs(x)))) + (n - 1))


def get_val(key, line):
    regex = key + r"(\d*\.*\d*)"
    match = re.search(regex, line)
    if match is not None:
        return match.group(1)
    else:
        raise LookupError


def wait_for_done(filename=sft_test_filename):
    with open(filename, "r") as test_file:
        while 1:
            where = test_file.tell()
            line = test_file.readline()
            if not line:
                time.sleep(1)
                test_file.seek(where)
            else:
                if SFT_EOF in line:
                    output_file_md5 = md5_hash_of_file(filename)
                    with open("touchfile", "w") as touchfile:
                        touchfile.write("{} file completely written. Move on to read.\n".format(filename))
                        touchfile.write(line)
                        touchfile.write(str(output_file_md5))
                        # print( "Last line read = " + line )
                    return


def md5_hash(handle):
    handle.seek(0)
    md5calc = md5()
    while True:
        data = handle.read(10240)  # reasonable value from examples
        if len(data) == 0:
            break
        md5calc.update(data.encode("UTF-8"))
    hash = md5calc.hexdigest()
    return hash


def md5_hash_of_file(filename):
    # print( "Getting md5 for " + filename )
    file_handle = open(filename)
    hash = md5_hash(file_handle)
    # md5calc = md5()
    # while True:
    # file_bytes = file_handle.read( 10240 ) # value picked from example!
    # if len(file_bytes) == 0:
    # break
    # md5calc.update( file_bytes )
    file_handle.close()
    # hash = md5calc.hexdigest()
    return hash


def has_match(target, matches):
    for match in matches:
        if match in target:
            return True
    return False


def get_char(key, line):
    regex = key + r"(\w*\d*\w*\d*)"
    match = re.search(regex, line)
    if match != None:
        return match.group(1)
    else:
        raise LookupError


def convert_barchart_to_interpolation(population_groups, result_values):
    for i in range(len(population_groups)):
        for j in range(0, 2 * len(population_groups[i]), 2):
            age_or_year = population_groups[i][j]
            population_groups[i].insert(j + 1, age_or_year + 0.9999999)
    for i in range(0, 2 * len(result_values), 2):
        age_value = result_values[i]
        age_value_copy = [p for p in age_value]
        result_values.insert(i + 1, age_value_copy)
    for age_value in result_values:
        for i in range(0, 2 * len(age_value), 2):
            age_value.insert(i + 1, age_value[i])


def cal_tolerance_poisson(expected_value, prob=0.05):
    """
    This method calculates the tolerance for expected mean of N draws from a Poisson or Binomial distributions.
    The probability of test value will exceed the tolerance is prob(default value is 5%).

    The equation is based on the cumulative distribution function and error function of a normal
    distribution.

    Applications: The sum of N draws from N independent Poisson distributions is Poisson distributed. When a Poisson has
    rate lambda greater than 10, then normal distribution with mean = lambda and variance = lambda is a good
    approximation to the Poisson distribution. In this case we can use this method for N draws from Poisson distributions.

    :param expected_value: expected mean
    :param prob: the probability of test value will exceed the tolerance.
    :return tolerance:
    """
    if expected_value < 10:
        raise ValueError("This method only valid with an expected_value >= 10.")
    else:
        from scipy.special import erfinv
        tolerance = -math.sqrt(2) * math.sqrt(expected_value) * erfinv(prob - 1) / expected_value

        return tolerance


def cal_tolerance_binomial(expected_value, binomial_p, prob=0.05):
    """
    This method calculates the tolerance for expected mean of a Binomial distributions. The probability of test value
    will exceed the tolerance is prob(default value is 5%).

    The equation is based on the cumulative distribution function and error function of a normal
    distribution.

    Applications: When a Binomial distribution has a trial n greater than 20 and probability binomial_p which is not
    near 0 or 1, then normal distribution with mean = n*p and variance = n*p*(1-p) is a reasonable approximation for
    the Binomial distribution. In this case, we can also use this method for Binomial distribution.

    :param expected_value: expected mean from a Binomial distribution.
    :param binomial_p: p from a Binomial distribution.
    :param prob: the probability of test value will exceed the tolerance.
    :return tolerance:
    """
    if expected_value < 10 or expected_value * (1 - binomial_p) < 10:
        raise ValueError("This method only valid with an expected_value >= 10 and expected_value * (1 - binomial_p) >= 10.")
    else:
        from scipy.special import erfinv
        tolerance = -math.sqrt(2) * math.sqrt(expected_value * (1 - binomial_p)) * erfinv(prob - 1) / expected_value

        return tolerance


def get_config_parameter(config_filename="config.json", parameters: Union[list, str]="Config_Name"):
    with open(config_filename, 'r') as config_file:
        cf = json.load(config_file)["parameters"]
        try:
            if isinstance(parameters, str):
                return [cf[parameters]]
            elif isinstance(parameters, list):
                res = []
                for parameter in parameters:
                    res.append(cf[parameter])
                return res
            else:
                raise ValueError(f"parameters is {type(parameters)}, expected list or str.\n")
        except KeyError as ke:
            raise KeyError(f"{ke} is not found in {config_filename}.\n")


def get_config_name(config_filename="config.json"):
    return get_config_parameter(config_filename, "Config_Name")[0]


def get_simulation_duration(config_filename="config.json"):
    return get_config_parameter(config_filename, "Simulation_Duration")[0]


def get_simulation_timestep(config_filename="config.json"):
    return get_config_parameter(config_filename, "Simulation_Timestep")[0]


def plot_bar_graph(data, xticklabels, x_label, y_label, legends, plot_name, show=True, num_decimal=1):
    """
    plot bar chart for two lists of data.
    :param data: two lists of data to plot
    :param xticklabels: list of string for xticks, length = length of data to plot
    :param x_label:  string for x label
    :param y_label: string for y label
    :param legends: list of string for legends, length = 2
    :param plot_name: string for plot title and name
    :return:
    """

    if not check_for_plotting():
        show = False

    fig = plt.figure()
    ax = fig.add_axes([0.12, 0.15, 0.76, 0.76])
    x_ind = np.arange(len(xticklabels))
    width = 0.35
    rectangles1 = ax.bar(x_ind, data[0], width, color="orange")
    rectangles2 = ax.bar(x_ind + width, data[1], width, color="seagreen")
    ax.set_xticks(x_ind + width / 2)
    ax.set_xticklabels(xticklabels)
    ax.set_xlabel(x_label)
    ax.set_ylabel(y_label)
    ax.set_title(f'{plot_name}')
    ax.legend((rectangles1[0], rectangles2[0]), (legends[0], legends[1]))
    autolabel(ax, rectangles1, num_decimal)
    autolabel(ax, rectangles2, num_decimal)

    fig.savefig(f'{plot_name}.png')
    if show:
        plt.show()
    plt.close(fig)


def autolabel(ax, rects, num_decimal=1):
    """
    Attach a text label above each bar displaying its height
    """
    for rect in rects:
        height = rect.get_height()
        ax.text(rect.get_x() + rect.get_width()/2., 1.005*height,
                f'%.{num_decimal}f' % float(height),
                ha='center', va='center',rotation=45)


def plot_cdf_w_fun(data, name="cdf", cdf_function=None, args=(), show=False):

    if not check_for_plotting():
        show = False

    cdf, bin_edges = calculate_cdf(data)
    data_set = sorted(set(data))

    # Plot the cdf
    fig = plt.figure()
    ax = fig.add_axes([0.12, 0.12, 0.76, 0.76])
    plt.plot(bin_edges[:-1], cdf, linestyle='--', marker="o", color='b', alpha=0.3, label="calculated with data bin",
             markersize=3)
    ax.set_ylim((-0.01, 1.05))
    ax.set_ylabel("Probability")
    ax.set_xlabel('X')
    plt.grid(True)

    if cdf_function is not None:
        cdf_theoretical = cdf_function(data_set, *args)
        plt.scatter(data_set, cdf_theoretical, color='r', alpha=0.3,
                    label=f"calculated used {cdf_function.__name__} function", s=20)
    ax.set_title("cumulative distribution function")
    ax.legend(loc=0)

    plt.savefig(f"{name}.png")
    if show:
        plt.show()
    plt.close()


def calculate_cdf(data):
    data_size = len(data)

    # Set bins edges
    data_set = sorted(set(data))
    bins = np.append(data_set, data_set[-1] + 1)

    # Use the histogram function to bin the data
    counts, bin_edges = np.histogram(data, bins=bins, density=False)

    counts = counts.astype(float) / data_size

    # Find the cdf
    cdf = np.cumsum(counts)

    return cdf, bin_edges


def get_cmap(n, name='hsv'):
    """
    Returns a function that maps each index in 0, 1, ..., n-1 to a distinct
    RGB color; the keyword argument name must be a standard mpl colormap name.
    :param n:
    :param name:
    :return:
    """
    return plt.cm.get_cmap(name, n)


def three_plots(dist1, cdf_function=None, args=(), dist2=None,
                 label1="data 1", label2=None, title=None, xlabel=None, ylabel=None,
                 category='three_plots', show=True, line=False, alpha=1, color1='b', color2='r', sort=False):
    """
    Compares two distributions with a distribution plot, a density plot, and a cdf plot.
    Mainly used to compare an expected distribution with one from model data.
    """
    if not check_for_plotting():
        show = False

    if sort:
        dist1 = sorted(dist1)
        if dist2 is not None:
            dist2 = sorted(dist2)
    fig, axarr = plt.subplots(1, 3)

    # 1st plot: simple plot with all data point
    if line:
        color1 += '-'
        color2 += '-'
    axarr[0].plot(dist1, color1, marker='s', alpha=alpha, label=label1, lw= 0.5, markersize=4)
    if dist2 is not None: # "if dist2:" will not work with numpy.ndarray
        axarr[0].plot(dist2, color2, marker='o', alpha=alpha, label=label2, lw=0.5, markersize=3)
    axarr[0].set_title("plot data")
    if xlabel:
        axarr[0].set_xlabel(xlabel)
    if ylabel:
        axarr[0].set_ylabel(ylabel)

    # 2nd plot: density plot
    sns.histplot(dist1, kde=True, ax=axarr[1], color=color1, label=label1)
    if dist2 is not None:  # "if dist2:" will not work with numpy.ndarray
        sns.histplot(dist2, kde=True, ax=axarr[1], color=color2, label=label2)
    axarr[1].set_xlabel("Probability")
    axarr[1].set_title("histplot")
    axarr[1].set_ylim(axarr[0].get_ylim())

    # 3rd plot: cdf plot
    cdf, bin_edges = calculate_cdf(dist1)
    data_set = sorted(set(dist1))
    axarr[2].plot(bin_edges[:-1], cdf, linestyle='--', color=color1, alpha=alpha, label=label1,
             markersize=3)
    if cdf_function is not None:
        cdf_theoretical = cdf_function(data_set, *args)
        axarr[2].scatter(data_set, cdf_theoretical, color='r', alpha=0.3,
                        label=f"{cdf_function.__name__}", s=20)
    elif dist2 is not None:
        cdf2, bin_edges2 = calculate_cdf(dist2)
        axarr[2].plot(bin_edges2[:-1], cdf2, linestyle='--', color=color2, alpha=alpha,
                      label=label2, markersize=3)
    axarr[2].set_ylim((-0.01, 1.05))
    axarr[2].set_ylabel("Probability")
    if ylabel:
        axarr[2].set_xlabel(ylabel)
    axarr[2].set_title("CDF")

    # formatting
    if title:
        fig.suptitle(title)
    for ax in axarr:
        ax.legend(loc=0)
        ax.grid(True)
    fig.tight_layout()
    fig.subplots_adjust(top=0.88)
    fig.savefig(str(category) + '.png')
    if show:
        plt.show()
    plt.close(fig)
    return None
