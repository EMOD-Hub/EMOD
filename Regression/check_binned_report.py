# check_binned_reports.py
# -----------------------------------------------------------------------------
# DMB 5/8/2017
# The purpose of this script is to compare/check a BinnedReport.json with its associated
# InsetChart.json.  It sums the bins of the BinnedReport and compares this sum
# with the value for that timestep in the InsetChart.
#
# As of 5/8/2017, the Infected channel of the InsetChart is one timestep behind.
# Hence, the script skips that channel at this time.
# -----------------------------------------------------------------------------

import sys, os, json, collections, struct, datetime
import matplotlib.pyplot as plt
import argparse


# ---------------------------------------------------------------------------------------
# Compare two values - My unit test like feature
#
# NOTE: In Scenarios/HINT/O4_WAPertussis, you may need to reduce the accuracy.
# ---------------------------------------------------------------------------------------

def CompareValues(var, exp, act):
    if (abs(exp - act) > 0.0000001):
        print(var + ": Expected " + str(exp) + " but got " + str(act))
        exit(-1)


# ---------------------------------------------------------------------------------------
# A class for reading an InsetChart.json file.
# ---------------------------------------------------------------------------------------
class InsetChart:
    def __init__(self):
        self.fn = ""
        self.data_for_channel = {}

    def Read(self, filename, ch_names):
        self.fn = filename

        with open(filename, 'r') as file:
            self.json_data = json.load(file)

        for ch in ch_names:
            self.data_for_channel[ch] = self.json_data["Channels"][ch]["Data"]

        if (("Infected" in ch_names) and ("Statistical Population" in ch_names)):
            for i in range(len(self.data_for_channel["Infected"])):
                inf_percentage = self.data_for_channel["Infected"][i]
                population = self.data_for_channel["Statistical Population"][i]
                inf_count = float(inf_percentage) * float(population)
                self.data_for_channel["Infected"][i] = inf_count


# ---------------------------------------------------------------------------------------
# A class for reading one channel of a BinnedReport.json file.
# ---------------------------------------------------------------------------------------
class SummedBinnedReport:
    def __init__(self):
        self.fn = ""
        self.summed_data_for_channel = {}

    def Read(self, filename, ch_names):
        self.fn = filename

        with open(filename, 'r') as file:
            self.json_data = json.load(file)

        num_timesteps = self.json_data["Header"]["Timesteps"]
        num_axises = self.json_data["Header"]["Subchannel_Metadata"]["NumBinsPerAxis"][0][0]

        for ch in ch_names:
            self.summed_data_for_channel[ch] = []
            cum = 0
            for t in range(num_timesteps):
                value = 0
                for a in range(num_axises):
                    value += self.json_data["Channels"][ch]["Data"][a][t]
                if ch == "Disease Deaths":
                    cum += value
                    value = cum
                self.summed_data_for_channel[ch].append(value)


# ---------------------------------------------------------------------------------------
# Compare each channel and exit if a difference is found.
# ---------------------------------------------------------------------------------------
if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('output_dir', help='Directory containing BinnedReport.json and InsetChart.json')
    parser.add_argument('--show_plot', action="store_true", help='Will show the data of both reports in a plot.')
    parser.add_argument('--omit_NewReportedInfections', action="store_false",
                        help='Will assume reports have the New Reported Infections channel')
    args = parser.parse_args()

    inset_chart_fn = os.path.join(args.output_dir, "InsetChart.json")
    binned_report_fn = os.path.join(args.output_dir, "BinnedReport.json")

    channel_name_map_br_to_ic = {}
    channel_name_map_br_to_ic["Disease Deaths"] = "Disease Deaths"
    #channel_name_map_br_to_ic[ "Infected"       ] = "Infected"
    channel_name_map_br_to_ic["New Infections"] = "New Infections"
    channel_name_map_br_to_ic["Population"] = "Statistical Population"
    if args.omit_NewReportedInfections:
        channel_name_map_br_to_ic["New Reported Infections"] = "New Reported Infections"

    inset_chart = InsetChart()
    inset_chart.Read(inset_chart_fn, channel_name_map_br_to_ic.values())

    binned_report = SummedBinnedReport()
    binned_report.Read(binned_report_fn, channel_name_map_br_to_ic.keys())

    for br_ch, ic_ch in channel_name_map_br_to_ic.iteritems():

        print("!!!!!!!!!!!!!!!!!! Compare - " + br_ch + " !!!!!!!!!!!!!!!!!!!!!!!")
        var = br_ch + "-num_values"
        CompareValues(var, len(inset_chart.data_for_channel[ic_ch]), len(binned_report.summed_data_for_channel[br_ch]))

        if args.show_plot:
            subplot = plt.subplot(1, 1, 1)
            subplot.plot(inset_chart.data_for_channel[ic_ch], 'r-', binned_report.summed_data_for_channel[br_ch], 'b-')
            plt.title(br_ch)
            plt.show()

        cum = 0
        for i in range(len(inset_chart.data_for_channel[ic_ch])):
            exp = inset_chart.data_for_channel[ic_ch][i]
            act = binned_report.summed_data_for_channel[br_ch][i]
            var2 = br_ch + "[" + str(i) + "]"
            CompareValues(var2, exp, act)
        print("!!!!!!!!!!!!!!!!!! PASSED - " + br_ch + " !!!!!!!!!!!!!!!!!!!!!!!")

# ---------------------------------------------------------------------------------------
# ---------------------------------------------------------------------------------------
