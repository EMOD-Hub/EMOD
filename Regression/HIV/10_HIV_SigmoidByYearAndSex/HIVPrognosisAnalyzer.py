import matplotlib.pyplot as plt
import numpy as np
import os
import json
import math
import scipy.stats as sps

BIN_WIDTH = 5.0
DAYS_PER_YEAR = 365.0

FERRAND_CHILD_AGE_YEARS = 15.0
FERRAND_MAX_AGE_YEARS = 50.0

#% Todd et al, AIDS 2007: 11 pp. S55
ADULT_LAMBDA_SLOPE = -0.2717
ADULT_LAMBDA_INTERCEPT = 21.182
ADULT_KAPPA_SLOPE = 0.0
ADULT_KAPPA_INTERCEPT = 2.0

# Ferrand, R. A. et al. AIDS among older childern and adolescents in Southern Africa: projecting the time course and magnitude of the epidemic. AIDS 23, 2039-2046 (2009)
CHILD_BETA = 1.52
CHILD_S = 2.7
CHILD_MU = 16.0
CHILD_ALPHA = 0.57

class HIVPrognosisAnalyzer():

    def __init__(self, plot=False, alpha=1e-3, verbose=False, output_dir='output'):

        self.filenames = [os.path.join(output_dir, 'ReportHIVInfection.csv')]

        self.plot = plot
        self.alpha = alpha
        self.verbose = verbose
        self.output_dir = output_dir
        self.data = {}  # observed relationship duration data by type
        self.fun = {}   # truth function by type
        self.results = {} # Place to store the boolean validity and other statistics of sub-tests

    def kstest(self, duration, fun, alpha):

        (D, p_val) = sps.kstest( duration, fun)

        if p_val < alpha:
            return {'Valid': False, 'Test_Statistic': D, 'P_Value': p_val}

        return {'Valid': True, 'Test_Statistic': D, 'P_Value': p_val}


    def map(self, output_data):
        emit_data = {}

        hiv_infections = output_data[ self.filenames[0] ]    # CSV, has 'data' and 'colMap'
        rows = hiv_infections['data']
        colMap = hiv_infections['colMap']

        age = [ float(r[colMap['Age']]) for r in rows]
        prognosis = [ float(r[colMap['Prognosis']]) for r in rows]

        age_bin = [int(math.floor(a/BIN_WIDTH)) for a in age]

        for abi in range(1+max(age_bin)):
            key = ( id(self), abi )
            emit_data[key] = []

        for (abi, prog) in zip(age_bin, prognosis):
            key = ( id(self), abi )
            emit_data[key].append(prog)

        return emit_data

    def reduce(self, parsers):

        # Accumulate grouped data over parsers
        for k,p in parsers.items():

            my_items = [ (key,val) for (key,val) in p.emit_data.items() if key[0] == id(self) ]

            for (key, val) in my_items:
                if key not in self.data:
                    self.data[key] = np.array(val)
                else:
                    self.data[key] = np.hstack( self.data[key], np.array(val) )

        # Now do the stat analysis
        for (key, prognosis) in self.data.items():
            (abi,) = key[1:2]
            age_range = [BIN_WIDTH*abi, BIN_WIDTH*(abi+1)]

            # Note dummy parameters in the lambda below kapp necessary variable (lam, kap) in scope
            # Using bin center
            mean_age = np.mean(age_range)

            if mean_age < FERRAND_CHILD_AGE_YEARS:
                self.fun[abi] =  lambda x, a=CHILD_ALPHA, b=CHILD_BETA, m=CHILD_MU, s=CHILD_S:    \
                            a     * sps.expon.cdf([xx*b/DAYS_PER_YEAR for xx in x]) +             \
                            (1-a) * sps.exponweib(1,s).cdf([ np.log(2.0)**(1/s) * xx/m/DAYS_PER_YEAR for xx in x])

            else:
                mean_age = min(mean_age, FERRAND_MAX_AGE_YEARS)
                lam = ADULT_LAMBDA_SLOPE*mean_age + ADULT_LAMBDA_INTERCEPT
                kap = ADULT_KAPPA_SLOPE*mean_age + ADULT_KAPPA_INTERCEPT

                lam = DAYS_PER_YEAR * lam

                self.fun[abi] = lambda x, lam=lam, kap=kap: sps.exponweib(1,kap).cdf([xx/lam for xx in x]) 

            self.results[abi] = self.kstest(prognosis, self.fun[abi], self.alpha)

            if self.verbose:
                if self.results[abi]['Valid']:
                    print("Sub-test for age range " + str(age_range) + " passed.")
                else:
                    print("Sub-test for age range " + str(age_range) + " failed.")

    def finalize(self):
        if self.plot:
            fig = plt.figure('HIV Prognosis', figsize=(20, 10))
            nrows = 2.0
            ncols = math.ceil(len(self.results)/nrows)

            keys = self.data.keys()
            for idx, key in enumerate(sorted(keys, key = lambda x: x[1])):
                prognosis = self.data[key]
                (abi,) = key[1:2]

                plt.subplot(nrows, ncols, idx+1)

                x = sorted(prognosis)
                y = [ i/float(len(prognosis)) for i in range(0,len(prognosis)) ]

                xp = [ xx/DAYS_PER_YEAR for xx in x]

                plt.plot(xp,y)

                y_true = self.fun[abi](x)
                plt.plot( xp, y_true, linestyle="dashed")

                valid_str = {}
                valid_str[True] = "Pass"
                valid_str[False] = "Fail"
                plt.title("Age bin %s: %s (p=%0.2f)" % (abi, valid_str[self.results[abi]['Valid']], self.results[abi]['P_Value']) )

            #fig.tight_layout()

        each_valid = [y['Valid'] for y in [ self.results[x] for x in self.results]]
        all_valid = all( each_valid )

        return all_valid
