import matplotlib.pyplot as plt
import numpy as np
import os
import json
import math
import scipy.stats as sps

DAYS_PER_YEAR = 365.0

#define WHO_STAGE_KAPPA_0 (0.9664f)
#define WHO_STAGE_KAPPA_1 (0.9916f)
#define WHO_STAGE_KAPPA_2 (0.9356f)
#define WHO_STAGE_LAMBDA_0 (0.26596f)
#define WHO_STAGE_LAMBDA_1 (0.19729f*4/3)
#define WHO_STAGE_LAMBDA_2 (0.34721f*4.0)

Stage_Duration_Param = {}
Stage_Duration_Param['Stage_1'] = {'Lambda': 0.26596, 'Kappa': 0.9664}
Stage_Duration_Param['Stage_2'] = {'Lambda': 0.19729, 'Kappa': 0.9916}
Stage_Duration_Param['Stage_3'] = {'Lambda': 0.34721, 'Kappa': 0.9356}
Stage_Max_Frac_Prog = 1.0

#print "WARNING: Param manip"
#Stage_Duration_Param['Stage_2'] = {'Lambda': 0.19729 * 4.0/3.0, 'Kappa': 0.9916}
#Stage_Duration_Param['Stage_3'] = {'Lambda': 0.34721 * 4.0, 'Kappa': 0.9356}
#Stage_Max_Frac_Prog = 0.9

class WHOStageAnalyzer():

    def __init__(self, plot=False, alpha=1e-3, verbose=False, output_dir='output'):

        self.filenames = [os.path.join(output_dir, 'ReportHIVInfection.csv')]

        self.plot = plot
        self.alpha = alpha
        self.verbose = verbose
        self.output_dir = output_dir
        self.data = {}  # observed relationship duration data by type
        self.fun = {}   # truth function by type
        self.fun_tot = {}   # truth function by type
        self.results = {} # Place to store the boolean validity and other statistics of sub-tests

        self.map_count = 0

        self.fun = {}
        for stage in range(2,5):
            k = 'Stage_' + str(stage-1)
            self.fun[k] = []

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

        iid = [ int(float(r[colMap['Id']])) for r in rows]
        uids = set(iid)

        for stage in range(1,5):
            emit_data[(id(self),stage)]=[]

        start_year = float(rows[0][colMap['Year']])
        end_year = float(rows[-1][colMap['Year']])

        for uid in uids:
            myrows = [r for r in rows if float(r[colMap['Id']]) == uid]

            prognosis = float(myrows[0][colMap['Prognosis']]) / DAYS_PER_YEAR

            max_pf = Stage_Max_Frac_Prog
            if start_year + prognosis > end_year:
                # Sim will terminate before reaching prognosis fraction of 1
                # Need to compute new maximum observable prognosis fraction
                slope = ( float(myrows[-1][colMap['PrognosisCompletedFraction']]) - float(myrows[0][colMap['PrognosisCompletedFraction']]) ) / \
                        ( float(myrows[-1][colMap['Year']]) - float(myrows[0][colMap['Year']]) )
                max_pf = min(Stage_Max_Frac_Prog, slope * (end_year - start_year))

            if self.verbose:
                print (start_year, end_year, prognosis, max_pf)

            for stage in range(1,5):
                key = (id(self),stage)
                prev_stage_key = (id(self),stage-1)

                pf = next( \
                        ( float(r[colMap['PrognosisCompletedFraction']]) \
                        for r in myrows if float(r[colMap['WHOStage']]) >= stage ) \
                    , None)

                if pf is None:
                    continue

                if self.verbose:
                    print("id: %d, stage: %d, pf: %f" % (uid, stage, pf))
                    print('Found stage ' + str(stage) + ' for uid=' + str(uid) + ' start at prog frac of ' + str(pf))

                if stage == 1:
                    initial_stage = next((float(r[colMap['WHOStage']]) for r in rows if float(r[colMap['Id']]) == uid), None)
                    if initial_stage >= 2:
                        print("WARNING: Initial WHO stage for id=%d is %f" % (uid, initial_stage))
                    if key not in self.results:
                        self.results[key] = {'Valid': initial_stage < 2} 
                    else:
                        self.results[key] = {'Valid': self.results[key]['Valid'] and initial_stage < 2}

                    emit_data[key].append(pf)    # Stage 1 entry prognosis fraction
                else:
                    pf_prev = emit_data[prev_stage_key][-1]
                    stage_duration = pf - pf_prev
                    emit_data[key].append(stage_duration)   # Stage duration (actually of previous stage!)

                    k = 'Stage_' + str(stage-1)
                    self.fun[k].append(
                        lambda prog_frac,
                               lam=Stage_Duration_Param[k]['Lambda'],
                               kap=Stage_Duration_Param[k]['Kappa'],
                               max_delta_pf=max_pf-pf_prev:
                        [1 if fp > max_delta_pf else sps.exponweib(1,kap).cdf(fp/lam) / sps.exponweib(1,kap).cdf(max_delta_pf/lam) for fp in prog_frac]
                        )

        return emit_data

    def reduce(self, parsers):

        # Accumulate grouped data over parsers
        for k,p in parsers.items():

            my_items = [ (key,val) for (key,val) in p.emit_data.items() if key[0] == id(self) ]

            for (key, val) in my_items:
                if key not in self.data:
                    self.data[key] = val
                else:
                    self.data[key] = self.data[key] + val

        # Average the individual functions by stage
        for stage in range(2,5):
            k = 'Stage_' + str(stage-1)
            self.fun_tot[k] = lambda x, k=k: [np.mean([f([xx]) for f in self.fun[k]]) for xx in x]

        keys = self.data.keys()
        for idx, key in enumerate(sorted(keys, key = lambda x: x[1])):
            (stage,) = key[1:2]

            if stage == 1:
                pass
            else:
                duration = self.data[key]
                k = 'Stage_' + str(stage-1) # -1 because "stage" is the next stage, e.g. 2 is for duation from 1-->2
                self.results[key] = self.kstest(duration, self.fun_tot[k], self.alpha)

        if self.verbose:
            print(self.results)

    def finalize(self):
        if self.plot:
            fig = plt.figure(self.__class__.__name__, figsize=(10, 5))
            ncols = len(self.data)

            for idx, key in enumerate(self.data):
                duration = self.data[key]
                (stage, ) = key[1:2]

                plt.subplot(1, ncols, idx+1)

                x = sorted(duration)
                y = [ i/float(len(duration)) for i in range(0,len(duration)) ]

                plt.plot(x,y)

                valid_str = {True: "PASS", False: "FAIL"}

                if stage == 1:
                    plt.title("S%d Entry: %s" % (stage, valid_str[self.results[key]['Valid']]) )
                else:
                    k = 'Stage_' + str(stage-1) # -1 because "stage" is the next stage, e.g. 2 is for duation from 1-->2
                    y_true = self.fun_tot[k](x)

                    plt.plot( x, y_true, linestyle="dashed")
                    plt.title("S%d->%d: %s (p=%0.2f)" % (stage-1, stage, valid_str[self.results[key]['Valid']], self.results[key]['P_Value']) )

        each_valid = [ self.results[x]['Valid'] for x in self.results]
        all_valid = all( each_valid )

        return all_valid
