# inset_chart_diff.py
# -----------------------------------------------------------------------------
# -----------------------------------------------------------------------------

import sys, os, json, collections, struct
import matplotlib.pyplot as plt
#import pylab
from math import sqrt, ceil


def ReadJson(json_fn):
    json_file = open(json_fn, 'r')
    json_data = json.load(json_file)
    json_file.close()
    return json_data


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("\nUsage: %s [InsetChart-1] [InsetChart2]" % os.path.basename(sys.argv[0]))
        exit(0)

    json_data_1 = ReadJson(sys.argv[1])
    json_data_2 = ReadJson(sys.argv[2])

    if (json_data_1["Header"]["Timesteps"] != json_data_2["Header"]["Timesteps"]):
        print("\nData files dont have the same number of Timesteps.")
        exit(0)

    channels_1 = set(json_data_1["Channels"])
    channels_2 = set(json_data_2["Channels"])

    for chan_title in (channels_1 & channels_2):
        num_steps_1 = len(json_data_1["Channels"][chan_title]["Data"])
        num_steps_2 = len(json_data_2["Channels"][chan_title]["Data"])

        if (num_steps_1 != num_steps_2):
            print("\nChannels dont have same number of data-data1=" + num_steps_1 + "  data2=" + num_steps_2)
            exit(0)

        for tstep_idx in range(0, num_steps_1):
            json_data_1["Channels"][chan_title]["Data"][tstep_idx] = json_data_1["Channels"][chan_title]["Data"][
                                                                         tstep_idx] - \
                                                                     json_data_2["Channels"][chan_title]["Data"][
                                                                         tstep_idx]

    # -------
    # --- plot
    # --------
    num_chans = json_data_1["Header"]["Channels"]

    square_root = ceil(sqrt(num_chans))

    n_figures_x = square_root
    n_figures_y = ceil(float(num_chans) / float(
        square_root))  #Explicitly perform a float division here as integer division floors in Python 2.x

    #figure = plt.figure(sys.argv[1] + " - " + sys.argv[2])   # label includes the full (relative) path to the scenario, take just the final directory

    idx = 1
    for chan_title in sorted(json_data_1["Channels"]):
        try:
            subplot = plt.subplot(n_figures_x, n_figures_y, idx)
            subplot.plot(json_data_1["Channels"][chan_title]["Data"], 'r-')
            plt.setp(subplot.get_xticklabels(), fontsize='5')
            plt.title(chan_title, fontsize='6')
            idx += 1
        except Exception as ex:
            print(str(ex))

    plt.suptitle(sys.argv[1] + " - " + sys.argv[2])
    plt.subplots_adjust(bottom=0.05)
    plt.show()
