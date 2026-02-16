
import matplotlib.pyplot as plt
import numpy as np
import os
import json
import math
import scipy.stats as sps

colors = ['b', 'g', 'k', 'r', 'm', 'c', 'y']
TRANSITORY = 0
INFORMAL = 1
MARITAL = 2
relnames = ['Transitory', 'Informal', 'Marital']
DAYS_PER_YEAR = 365

# a class to analyze relationship durations
class RelationshipDurationAnalyzer():

    def __init__(self, plot=False, alpha=1e-3, verbose=False, output_dir='output'):
        self.plot = plot
        self.alpha = alpha
        self.verbose = verbose
        self.output_dir = output_dir
        self.filenames = [os.path.join( self.output_dir, 'RelationshipStart.csv'), 'config.json']
        self.data = {}  # observed relationship duration data by type
        self.fun = {}   # truth function by type
        self.results = {} # Place to store the boolean validity and other statistics of sub-tests

        for reltype in [TRANSITORY, INFORMAL, MARITAL]:
            relname = relnames[reltype]
            self.results[relname] = {}
            self.results[relname]['Valid'] = False

    def weib_cdf(self, x,lam,kap):
        return [1-np.exp( -(xx/lam)**kap ) for xx in x]

    def kstest(self, duration, fun, alpha):

        (D, p_val) = sps.kstest( duration, fun)

        if p_val < alpha:
            return {'Valid': False, 'Test_Statistic': D, 'P_Value': p_val}

        return {'Valid': True, 'Test_Statistic': D, 'P_Value': p_val}


    def map(self, output_data):
        emit_data = {}

        # Pass config json through
        emit_data[ self.filenames[1] ] = output_data[ self.filenames[1] ]

        rel_start = output_data[ self.filenames[0] ]    # CSV, has 'data' and 'colMap'
        rows = rel_start['data']
        colMap = rel_start['colMap']

        for reltype in [TRANSITORY, INFORMAL, MARITAL]:
            relname = relnames[reltype]

            if self.verbose:
                print("Rel type: " + relname + ": lam=" + str(lam / DAYS_PER_YEAR) + " kap=" + str(kap) + " mu=" + str(
                    mu / DAYS_PER_YEAR))

            # Choose rows corresponding to this relationship type
            type_rows = [r for r in rows if r[colMap['Rel_type']] is str(reltype)]

            # Get relationship duration for each type_row
            emit_data[relname] = [ float(r[colMap['Rel_scheduled_end_time']]) - float(r[colMap['Rel_start_time']]) for r in type_rows]

        return emit_data

    def reduce(self, parsers):
        # Accumulate grouped data over parsers
        for k,p in parsers.items():
            for reltype in [TRANSITORY, INFORMAL, MARITAL]:
                relname = relnames[reltype]

                if relname not in self.data:
                    self.data[relname] = np.array(p.output_data[relname])
                else:
                    self.data[relname] = np.hstack( self.data[relname], np.array(p.output_data[relname]) )

        # Now run accumulated data through KS test to see if valid
        config_json = p.output_data[ self.filenames[1] ]

        cp = config_json['parameters']
        muv = {}
        kapv = {}
        muv[TRANSITORY]  = float(cp['Mean_Length_Transitory_Relationships'])
        kapv[TRANSITORY] = float(cp['Spread_Length_Transitory_Relationships'])
        muv[INFORMAL]    = float(cp['Mean_Length_Informal_Relationships'])
        kapv[INFORMAL]   = float(cp['Spread_Length_Informal_Relationships'])
        muv[MARITAL]     = float(cp['Mean_Length_Marital_Relationships'])
        kapv[MARITAL]    = float(cp['Spread_Length_Marital_Relationships'])


        for reltype in [TRANSITORY, INFORMAL, MARITAL]:
            relname = relnames[reltype]
            mu = DAYS_PER_YEAR * muv[reltype]
            kap = kapv[reltype]
            lam = mu / math.gamma(1+1/kap)

            # Note dummy parameters in the lambda below kepp necessary variable (lam, kap) in scope
            self.fun[relname] = lambda x, lam=lam, kap=kap: self.weib_cdf(x, lam, kap)

            self.results[relname] = self.kstest(self.data[relname], self.fun[relname], self.alpha, )

            if self.verbose:
                if self.results[relname]['Valid']:
                    print("Sub-test for " + relname + " passed.")
                else:
                    print("Sub-test for " + relname + " failed.")

    def finalize(self):
        if self.plot:
            fig = plt.figure('Relationship Duration', figsize=(10, 5))
            ncols = len(relnames)

            for (i,reltype) in enumerate([TRANSITORY, INFORMAL, MARITAL]):
                plt.subplot(1, ncols, i+1)

                relname = relnames[reltype]

                duration = self.data[relname]

                x = sorted(duration)
                y = [ i/float(len(duration)) for i in range(0,len(duration)) ]

                plt.plot(x,y)

                y_true = self.fun[relname](x)
                plt.plot( x, y_true, linestyle="dashed")

                valid_str = {}
                valid_str[True] = "PASS"
                valid_str[False] = "FAIL"
                plt.title("%s: %s (p=%0.2f)" % (relname, valid_str[self.results[relname]['Valid']], self.results[relname]['P_Value']) )

            plt.show()

        each_valid = [y['Valid'] for y in [ self.results[x] for x in self.results]]
        all_valid = all( each_valid )

        return all_valid
