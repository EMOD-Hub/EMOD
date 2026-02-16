import matplotlib.pyplot as plt
import numpy as np
import os
import json
import math
import scipy.stats as sps

MALE = 0
FEMALE = 1
DAYS_PER_YEAR = 365.0
gendernames = ['Male', 'Female']

class HIVDebutAnalyzer():

    def __init__(self, plot=False, alpha=1e-3, verbose=False, output_dir='output'):
        self.plot = plot
        self.alpha = alpha
        self.verbose = verbose
        self.output_dir = output_dir
        self.filenames = ['config.json', os.path.join(self.output_dir, 'ReportHIVInfection.csv')]
        self.data = {}  # observed relationship debut_age data by type
        self.fun = {}   # truth function by type
        self.results = {} # Place to store the boolean validity and other statistics of sub-tests

    def kstest(self, debut_age, fun, alpha):
        print("Running kstest on", len(debut_age), "samples.")
        (D, p_val) = sps.kstest( debut_age, fun)
        return {'Valid': p_val >= alpha, 'Test_Statistic': D, 'P_Value': p_val}

    def map(self, output_data):
        emit_data = {}

        hiv_infections = output_data[ self.filenames[1] ]    # CSV, has 'data' and 'colMap'
        rows = hiv_infections['data']
        colMap = hiv_infections['colMap']

        # Extract parameters from config
        config_json = output_data[ self.filenames[0] ]
        cp = config_json['parameters']
        muv = {}
        kapv = {}
        muv[MALE]  = float(cp['Sexual_Debut_Age_Male_Mean'])
        kapv[MALE] = float(cp['Sexual_Debut_Age_Male_Shape'])
        muv[FEMALE]   = float(cp['Sexual_Debut_Age_Female_Mean'])
        kapv[FEMALE]     = float(cp['Sexual_Debut_Age_Female_Shape'])

        for gender in [MALE, FEMALE]:
            gendername = gendernames[gender]

            # Choose rows corresponding to this relationship type
            gender_rows = [r for r in rows if r[colMap['Gender']] is str(gender)]

            # Get debut_age for each gender_row
            debut_age = [ float(r[colMap['DebutAge']])/DAYS_PER_YEAR for r in gender_rows]

            mu = float(muv[gender])
            kap = float(kapv[gender])
            lam = mu / math.gamma(1+1/kap)
            #z = 1.0+1.0/float(kap)
            #gamma_approx_sterling = math.sqrt(2*math.pi/z) * (1/math.e*(z + 1/(12*z - 1/(10*z))))**z
            #gamma_approx_winschitl = (2.5066282746310002 * math.sqrt(1.0/z) * ((z/math.e) * math.sqrt(z*math.sinh(1/z) + 1/(810*z**6)))**z)
            #lam = mu / gamma_approx_winschitl

            key = ( id(self), gender, lam, kap )
            emit_data[key] = {'DebutAge': debut_age}

            # Note dummy parameters in the lambda below kapp necessary variable (lam, kap) in scope
            if key not in self.fun:
                self.fun[key] = lambda x, lam=lam, kap=kap: \
                    sps.exponweib(1,kap).cdf([xx/lam for xx in x])

        return emit_data

    def reduce(self, parsers):
        # Accumulate grouped data over parsers
        for k,p in parsers.items():

            my_items = [ (key,val) for (key,val) in p.emit_data.items() if key[0] == id(self) ]

            for (key, val) in my_items:

                if key not in self.data:
                    self.data[key] = np.array(val['DebutAge'])
                else:
                    self.data[key] = np.hstack( self.data[key], np.array(val['DebutAge']) )

        # Now do the stat analysis
        for (key, debut_age) in self.data.items():
            (gender, lam, kap) = key[1:4]

            self.results[key] = self.kstest(debut_age, self.fun[key], self.alpha, )

            if self.verbose:
                if self.results[key]['Valid']:
                    print("Sub-test for " + str(key) + " passed.")
                else:
                    print("Sub-test for " + str(key) + " failed.")
                    print(self.results[key])

    def finalize(self):
        if self.plot:
            fig = plt.figure(self.__class__.__name__, figsize=(10, 5))
            ncols = len(self.fun)

            idx = 0
            for key, debut_age in self.data.items():
                (gender, lam, kap) = key[1:4]
                gendername = gendernames[gender]

                plt.subplot(1, ncols, idx+1)

                x = sorted(debut_age)
                y = [ i/float(len(debut_age)) for i in range(0,len(debut_age)) ]

                plt.plot(x,y)

                y_true = self.fun[key](x)
                plt.plot( x, y_true, linestyle="dashed")

                valid_str = {True: "PASS", False: "FAIL"}
                plt.title("%s: %s (p=%0.2f)" % (gendername, valid_str[self.results[key]['Valid']], self.results[key]['P_Value']) )
                idx = idx + 1

        each_valid = [y['Valid'] for y in [ self.results[x] for x in self.results]]
        all_valid = all( each_valid )

        return all_valid
