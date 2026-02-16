import matplotlib.pyplot as plt
import numpy as np
import os
import json
import math
import scipy.stats as sps
import random

MALE = 0
FEMALE = 1
DAYS_PER_YEAR = 365.0
YES = "YES"
NO = "NO"
yes_no = { True: YES, False: NO }

class TransInfo():
    def __init__(self, sti=False, circumcision=False, condom=False, art=False, acute=False, aids=False, prob_per_act=0):
        self.sti            = sti
        self.circumcision   = circumcision
        self.condom         = condom
        self.art            = art
        self.acute          = acute
        self.aids           = aids
        self.prob_per_act   = prob_per_act

    def to_tuple(self):
        return ( self.sti, self.circumcision, self.condom, self.art, self.acute, self.aids, self.prob_per_act )

    def from_tuple(self, entry):
        (self.sti, self.circumcision, self.condom, self.art, self.acute, self.aids, self.prob_per_act) = entry

    def __str__(self):
        return "STI: " + str(self.sti) + \
                ", Circumcision: "+  str(self.circumcision) + \
                ", Condom: " + str(self.condom) + \
                ", ART: " + str(self.art) + \
                ", Acute: " + str(self.acute) + \
                ", AIDS: " + str(self.aids) + \
                "--> Prob: " + str(self.prob_per_act)

class HIVTransmission():

    def __init__(self, plot=False, alpha=0.05, verbose=False, output_dir='output'):
        self.filenames = ['config.json', os.path.join(output_dir, 'RelationshipConsummated.csv'), os.path.join(output_dir, 'TransmissionReport.csv')]
        self.plot = plot
        self.alpha = alpha
        self.verbose = verbose
        self.output_dir = output_dir
        self.data = {}  # observed relationship duration data by type
        self.fun = {}   # Truth function by type
        self.results = {} 

    def binom_test(self, success, fail, prob_success, alpha):
        p_val = sps.binomtest( [success,fail], p=prob_success)
        return {'Valid': p_val >= alpha, 'P_Value': p_val}

    def map(self, output_data):
        emit_data = {}

        # Extract parameters from config
        config_json = output_data[ self.filenames[0] ]
        cp = config_json['parameters']
        Base_Infectivity = float(cp['Base_Infectivity'])
        Simulation_Timestep = float(cp['Simulation_Timestep'])

        # Coital Acts
        coital_acts = output_data[ self.filenames[1] ]
        rows = coital_acts['data']
        colMap = coital_acts['colMap']

        # Transmission
        transmission = output_data[ self.filenames[2] ]
        t_rows = transmission['data']
        t_colMap = transmission['colMap']

        rel_uids = list(set( [ int(float(r[colMap['Rel_ID']])) for r in rows] ))

        #for rid in [rel_uids[2]]:
        for rid in rel_uids:
            if self.verbose:
                print("Rel_ID: " + str(rid))

            # Choose rows corresponding to this relationship
            rid_rows = [r for r in rows if r[colMap['Rel_ID']] == str(rid)]

            A_id = int(float(rid_rows[0][colMap['A_ID']]))
            B_id = int(float(rid_rows[0][colMap['B_ID']]))

            trans_time = next( (float(r[t_colMap['SIM_TIME']]) for r in t_rows if \
                (int(float(r[t_colMap['SRC_ID']])) == A_id and int(float(r[t_colMap['DEST_ID']])) == B_id) \
                or \
                (int(float(r[t_colMap['SRC_ID']])) == B_id and int(float(r[t_colMap['DEST_ID']])) == A_id) ) \
                , None)

            infected = [ ( \
                    bool(float(r[colMap['A_Is_Infected']])),
                    bool(float(r[colMap['B_Is_Infected']])) \
                ) for r in rid_rows]

            if self.verbose:
                print('Infected: ')
                print(infected)

            discordant = [a ^ b for (a,b) in infected]

            if self.verbose:
                print('Discordant: ')
                print(discordant)

            try:
                first_discordant = discordant.index(True)
            except ValueError:
                continue    # rel is never discordant

            if self.verbose:
                print('First discordant: ' + str(first_discordant))

            concordant_pos = [a and b for (a,b) in infected]
            if self.verbose:
                print('Concordant Pos: ')
                print(concordant_pos)

            n_discordant_acts = None
            transmission = False
            try:
                first_concordant_pos = concordant_pos.index(True)
                # Transmission after n_discordant_acts
                n_discordant_acts = first_concordant_pos - first_discordant
                transmission = True
            except ValueError:
                if self.verbose:
                    print("Rel is discordant, but no transmission")
                first_concordant_pos = None
                # No transmission even after n_discordant_acts
                n_discordant_acts = sum(discordant)

            time_first_discordant = float(rid_rows[first_discordant][colMap['Time']])
            time_last_discordant = float(rid_rows[first_discordant + n_discordant_acts - 1][colMap['Time']])

            if self.verbose:
                print (first_discordant, first_concordant_pos, n_discordant_acts, time_first_discordant, time_last_discordant, len(rid_rows))

            # Loop
            for t in np.arange(time_first_discordant, time_last_discordant+1, Simulation_Timestep):
                rid_time_rows = [r for r in rid_rows if float(r[colMap['Time']]) == t ]

                acts_this_dt = len(rid_time_rows)
                if acts_this_dt == 0:
                    continue

                transmission_this_dt = trans_time is not None and float(rid_time_rows[0][colMap['Time']]) == trans_time
                if self.verbose:
                    print("Time: " + str(t) + ", Acts this DT: " + str(
                        len(rid_time_rows)) + ", Transmission this DT: " + str(transmission_this_dt))

                meta_vec = []

                for (idx,r) in enumerate(rid_time_rows):

                    metainfo = TransInfo()

                    a_inf = bool(float(r[colMap['A_Is_Infected']]))
                    b_inf = bool(float(r[colMap['B_Is_Infected']]))

                    if (a_inf and b_inf) or (not a_inf and not b_inf):
                        print("ERROR: Somehow encountered a concordant act?")

                    # Base
                    prob_per_act = Base_Infectivity

                    # STI
                    a_sti = bool(float(r[colMap['A_Has_CoInfection']]))
                    b_sti = bool(float(r[colMap['B_Has_CoInfection']]))
                    if a_sti or b_sti:
                        prob_per_act *= cp['STI_Coinfection_Multiplier']
                        metainfo.sti = True

                    a_gender = int(float(r[colMap['A_Gender']]))
                    b_gender = int(float(r[colMap['B_Gender']]))
                    a_circ = bool(float(r[colMap['A_Is_Circumcised']]))
                    b_circ = bool(float(r[colMap['B_Is_Circumcised']]))

                    # Circumcision
                    if (a_inf and b_gender == MALE and b_circ) or \
                            (b_inf and a_gender == MALE and a_circ):
                        prob_per_act *= cp['Circumcision_Reduced_Acquire']
                        metainfo.circumcision = True

                    # Condom
                    if bool(float(r[colMap['Did_Use_Condom']])):
                        prob_per_act *= (1 - cp['Condom_Transmission_Blocking_Probability'])
                        metainfo.condom = True

                    # Stage and ART
                    if a_inf:
                        stage = float(r[colMap['A_HIV_Infection_Stage']])
                        art = bool(float(r[colMap['A_Is_On_ART']]))
                    else:
                        stage = float(r[colMap['B_HIV_Infection_Stage']])
                        art = bool(float(r[colMap['B_Is_On_ART']]))

                    if stage == 1:
                        prob_per_act *= cp['Acute_Stage_Infectivity_Multiplier']
                        metainfo.acute = True
                    elif stage == 3:
                        prob_per_act *= cp['AIDS_Stage_Infectivity_Multiplier']
                        metainfo.aids = True

                    # ART (suppressive only for now!)
                    if art:
                        prob_per_act *= cp['ART_Viral_Suppression_Multiplier']
                        metainfo.art = True

                    # Male to female multiplier - try with a step

                    # Look at individual independent transmission acts - Binomial (Y/N) for each act by probabilty
                    #transmission_this_act = transmission and i == n_discordant_acts -1

                    #transmission_this_act = trans_time is not None and float(r[colMap['Time']]) == trans_time
                    #print (trans_time, float(r[colMap['Time']]), transmission_this_act)

                    #transmission_this_act = transmission_this_dt and idx == trans_idx

                    #if self.verbose:
                    #    print (t,a_inf, b_inf, prob_per_act, yes_no[transmission_this_act])


                    metainfo.prob_per_act = prob_per_act

                    meta_vec.append(metainfo)

                if transmission_this_dt:
                    # Draw act for transmission from cumulative distribution
                    C = [0]
                    random.shuffle(meta_vec)
                    for meta in meta_vec:
                        C.append( C[-1] + meta.prob_per_act*(1-C[-1]) )
                    C = [c/C[-1] for c in C]
                    rnd = random.random()
                    # Look for the last time that rnd < C
                    trans_idx = [c >= rnd for c in C].index(True) - 1  # -1 accounts for leading 0

                    # Could pop off acts after transmission

                for (idx, meta) in enumerate(meta_vec):
                    transmission_this_act = False

                    if transmission_this_dt:
                        if idx < trans_idx:
                            transmission_this_act = False
                        elif idx == trans_idx:
                            transmission_this_act = True
                        else:
                            continue    # Do not count acts after transmission

                    key = (id(self), metainfo.to_tuple())
                    if key not in emit_data:
                        emit_data[key] = { YES: 0, NO: 0 }
                    emit_data[key][yes_no[transmission_this_act]] += 1

        return emit_data

    def reduce(self, parsers):
        meta = TransInfo()
        # Accumulate grouped data over parsers
        for k,p in parsers.items():
            my_items = [ (key,val) for (key,val) in p.emit_data.items() if key[0] == id(self) ]

            if self.verbose:
                print(my_items)

            for (key, val) in my_items:
                tup = key[1]
                meta.from_tuple(tup)

                if meta not in self.data:
                    self.data[tup] = val
                else:
                    self.data[tup][YES] += val[YES]
                    self.data[tup][NO] += val[NO]

        # Now do the stat analysis
        for (tup, val) in self.data.items():
            meta.from_tuple(tup)
            n = float(val[YES]+val[NO])
            prob_per_act = meta.prob_per_act
            self.results[tup] = self.binom_test( val[YES], val[NO], prob_per_act, self.alpha)
            print("For prob %f, observed %f of %f --> %f, p=%f, [ %s ]" % (prob_per_act, val[YES], n, val[YES] / n,
                                                                           self.results[tup]["P_Value"],
                                                                           yes_no[self.results[tup]['Valid']]))
            print('\t' + str(meta))

    def finalize(self):
        each_valid = [y['Valid'] for y in [ self.results[x] for x in self.results]]

        if self.verbose:
            print(each_valid)

        all_valid = all( each_valid )

        return all_valid
