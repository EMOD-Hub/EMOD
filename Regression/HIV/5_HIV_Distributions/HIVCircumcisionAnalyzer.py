import matplotlib.pyplot as plt
import numpy as np
import os
import json
import math
import scipy.stats as sps

MALE = 0

class HIVCircumcisionAnalyzer():

    def __init__(self, plot=False, alpha=1e-3, verbose=False, output_dir='output'):
        self.plot = plot
        self.alpha = alpha
        self.verbose = verbose
        self.output_dir = output_dir
        self.filenames = ['campaign.json', os.path.join( self.output_dir, 'ReportHIVInfection.csv')]
        self.data = {}  # observed circumcision
        self.fun = {}   # truth function
        self.results = {} # Place to store the boolean validity and other statistics of sub-tests

    def binom_test(self, x, n, p, alpha):
        p_val = sps.binomtest(x, n, p)

        if self.verbose:
            print(p_val)

        if p_val < alpha:
            return {'Valid': False, 'Test_Statistic': x/n, 'P_Value': p_val}

        return {'Valid': True, 'Test_Statistic': x/n, 'P_Value': p_val}

#   def ztest(self, x, n, p, alpha):

#       if n*p < 10 or n*(1-p) < 10:
#           print "WARNING: not enough samples to use a Z-Test, marking as Fail!"
#           return {'Valid': False, 'Test_Statistic': None, 'P_Value': None}

#       p_hat = x / float(n)
#       std = math.sqrt( p*(1-p)/float(n) )
#       z_score = (p_hat - p) / std
#       p_val = 2 * sps.norm.cdf(-abs(z_score))

#       if self.verbose:
#           print p_val

#       if p_val < alpha:
#           return {'Valid': False, 'Test_Statistic': z_score, 'P_Value': p_val}

#       return {'Valid': True, 'Test_Statistic': z_score, 'P_Value': p_val}

    def map(self, output_data):
        emit_data = {}

        hiv_infections = output_data[ self.filenames[1] ]    # CSV, has 'data' and 'colMap'
        rows = hiv_infections['data']
        colMap = hiv_infections['colMap']

        male_circumcision = [ bool(float(r[colMap['IsCircumcised']])) for r in rows if r[colMap['Gender']] is str(MALE)]

        #emit_data['MC'] = male_circumcision

        # Get target MC from campaign
        campaign = output_data[ self.filenames[0] ]
        circumcision_event_name = 'Male circumcision for initial population'
        target_mc_frac = 0
        found = False
        for e in campaign['Events']:
            if str(e['Event_Name']) == circumcision_event_name:
                target_mc_frac = e["Event_Coordinator_Config"]["Coverage"]
                found = True

        if not found:
            print(
                "WARNING: Did not find campaign even with name '%s', assuming target male circumcision fraction is %f" % (
                    circumcision_event_name, target_mc_frac))

        key = ( id(self), target_mc_frac )
        emit_data[key] = {'MC': male_circumcision}

        return emit_data

    def reduce(self, parsers):
        # Accumulate grouped data over parsers
        for k,p in parsers.items():

            my_items = [ (key,val) for (key,val) in p.emit_data.items() if key[0] == id(self) ]

            for (key, val) in my_items:
                target_mc_frac = key[1]

                if target_mc_frac not in self.data:
                    self.data[target_mc_frac] = val['MC']
                else:
                    self.data[target_mc_frac] += val['MC']

        for (target_mc_frac, is_circ) in self.data.items():
            circ = [ c for c in is_circ if c is True ]
            num_circ = len( circ )
            num_males = len( is_circ )

            # Underlying distribution is binomial.  Why using z-test?
# For large samples, can use Pearson's chi-squared test (and the G-test) / Wikipedia

            #self.results[target_mc_frac] = self.ztest( num_circ, num_males, target_mc_frac, self.alpha )
            self.results[target_mc_frac] = self.binomtest( num_circ, num_males, target_mc_frac, self.alpha )

    def finalize(self):
        tmp = [ self.results[x] for x in self.results]

        each_valid = [y['Valid'] for y in [ self.results[x] for x in self.results]]

        if self.verbose:
            print(each_valid)

        all_valid = all( each_valid )

        return all_valid
