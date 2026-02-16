import sys
#from SimpleInsetChartAnalyzer import SimpleInsetChartAnalyzer
from RelationshipDurationAnalyzer import RelationshipDurationAnalyzer
from OutputParser import DTKOutputParser
import matplotlib.pyplot as plt

def main():

    # Create list of analyzers, appending each one in turn
    analyzers = []

    a = RelationshipDurationAnalyzer(plot=True)
    analyzers.append(a)

    #a = SimpleInsetChartAnalyzer(group_by='all', channels=['Statistical Population', 'Active Marital Relationships'])
    #analyzers.append(a)

    # Parsers
    sim_dir = '.'
    sim_id = ''
    sim_data = {}

    # Make the parsers
    parser = DTKOutputParser(sim_dir, sim_id, sim_data, analyzers)

    # Start the parser, also runs map from the analyzers
    parser.start()

    # Build parsers list by sim_id, which is here empty
    parsers = {}
    parsers[parser.sim_id] = parser

    for p in parsers.values():
        p.join()

    # Now analyzer the parsers
    all_valid = True
    for a in analyzers:
        a.reduce(parsers)
        valid = a.finalize()

        if type(valid) is not bool:
            print("WARNING: finalize did not return a bool!  Setting valid to False")
            valid = False

        if valid:
            print(a.__class__.__name__ + ': [ PASSED ]')
        else:
            print(a.__class__.__name__ + ': [ FAILED ]')

        all_valid &= valid

    plt.show()

    return all_valid

if __name__ == '__main__':
    all_valid = main()
    sys.exit(all_valid)
