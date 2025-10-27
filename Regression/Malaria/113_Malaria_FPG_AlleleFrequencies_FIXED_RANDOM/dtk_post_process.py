# dtk_post_process.py
# -----------------------------------------------------------------------------
# DMB 1/11/2020
# PURPOSE: This script is used to verify that the frequencies of the alleles
# in the parasite genomes distributed match what was specified in the campaign.
# -----------------------------------------------------------------------------

import sys, os, json, collections, struct, datetime
import pandas as pd
import sqlite3

def CompareValues( messages, var, exp, act ):
    """
    Compare two values - My unit test like feature
    """
    success = True
    if( abs(exp - act) > 0.005 ):
        messages.append( var + ": Expected " + str(exp) +" but got " + str(act) )
        success = False
    return success


def CompareArrays( messages, name, exp_data, act_data ):
    """
    Compare to the two arrays to make sure the values are 'very' similar
    """
    
    messages.append("!!!!!!!!!!!!!!!!!! Compare "+name+" !!!!!!!!!!!!!!!!!!!!!!!")
    success = True
    
    exp_num = len(exp_data)
    act_num = len(act_data)
    success = CompareValues( messages, name+":num", exp_num, act_num )
    if not success:
        return success
    
    i = 0
    for exp_val, act_val in zip(exp_data, act_data):
        #print(str(i)+"-"+str(exp_val)+" ? "+str(act_val))
        success = CompareValues( messages, name+":val["+str(i)+"]", exp_val, act_val )
        if not success:
            return success
        i += 1
        
    messages.append( "!!!!!!!!!!!!!!!!!! PASSED - "+name+" !!!!!!!!!!!!!!!!!!!!!!!")
    
    return success


class Campaign:
    """
    A class for reading an campaign.json file.
    """
    
    def __init__( self ):
        self.fn = ""
        self.json_data = {}

    def Read( self, filename, messages ):
        """
        Read the file given by filename and verify that the file has two CampaignEvents.
        """
        self.fn = filename

        with open( filename, 'r' ) as file:
            self.json_data = json.load( file )
            
        exp_num_events = 2 # one for outbreak and one for drugs
        act_num_events = len(self.json_data["Events"])
        CompareValues( messages, "Campaign:Num Events", exp_num_events, act_num_events )
        
    def GetBarcodeAlleleFrequencies( self, messages ):
        """
        The outbreak is expected to be in the first event.
        """
        freqs = self.json_data["Events"][0]["Event_Coordinator_Config"]["Intervention_Config"]["Barcode_Allele_Frequencies_Per_Genome_Location"]
        print(freqs)
        return freqs
        
    
class MalariaSqlReport:
    """
    A class for reading MalariaSqlReport
    """
    
    def __init__( self ):
        self.fn = ""
        self.conn = None
        self.cursor = None

    def Open( self, filename, messages ):
        """
        Open the database specified by filename and prepare for queries
        """
        self.fn = filename

        self.conn = sqlite3.connect( self.fn )
        self.conn.row_factory = lambda cursor, row: row[0] # makes the output lists of values instead of tuples
        self.cursor = self.conn.cursor()
        
    def Close( self ):
        self.conn.close()

    def ConvertCharacterToVal( self, messages, c ):
        if c == 'A':
            return 0
        elif c == 'C':
            return 1
        elif c == 'G':
            return 2
        elif c == 'T':
            return 3
        else:
            messages.append("Unexpected character='"+str(c)+"' in barcode='"+str(barcode)+"'")
            return 0
        
    def GetBarcodeAlleleFrequencies( self, messages ):
        """
        It is assumed that the genomes in the database are only created by OutbreakIndividual
        and not via recombination.  This implies no transmission
        """
        self.cursor.execute("SELECT Barcode FROM ParasiteGenomes" )
        barcodes = self.cursor.fetchall()
        
        # Initialize 2D array of frequencies
        freqs = []
        for i in range(10):
            freqs.append( [ 0.0, 0.0, 0.0, 0.0 ] )
        
        for code in barcodes:
            # Verify that the lenght of all the barcodes is 10
            if len(code) != 10:
                messages.append("Invalid barcode.  Expected length 10, but got "+str(len(code))+"-"+code)
                return freqs
            
            # count the occurances of the different characters at each location
            for i in range(len(code)):
                c = code[ i ]
                val = self.ConvertCharacterToVal( messages, c )
                freqs[ i ][ val ] += 1.0
        
        print( freqs )
        # Divide by the number of barcodes to get the fraction of each character
        for i in range(len(freqs)):
            for j in range(len(freqs[ i ])):
                freqs[ i ][ j ] = freqs[ i ][ j ] / len(barcodes)
        
        print( freqs )
        return freqs
        

def ShowUsage():
    print ('\nUsage: %s [output directory]' % os.path.basename(sys.argv[0]))


def application(output_path="output"):
    print("!!!!! Check allele frequencies !!!!!")
    
    # Define the names of the files to be used in the test
    campaign_fn = "campaign.json"
    sql_db_fn = os.path.join( output_path, "SqlReportMalariaGenetics.db" )
        
    messages = []

    # Open all of the files
    campaign = Campaign()
    campaign.Read( campaign_fn, messages )
    
    sql_db = MalariaSqlReport()
    sql_db.Open( sql_db_fn, messages )

    # Get the expected allele frequencies from the campaign file
    exp_allele_freqs = campaign.GetBarcodeAlleleFrequencies( messages )

    # Get the actual allele frequencies from the database
    act_allele_freqs = sql_db.GetBarcodeAlleleFrequencies( messages )
    
    # Verify the reports have similar data
    CompareValues( messages, "Campaign vs SqlDB:Num Allele Frequencies", len(exp_allele_freqs), len(act_allele_freqs) )
    for i in range(len(exp_allele_freqs)):
        CompareArrays( messages, "Campaign vs SqlDB:Allele Frequencies["+str(i)+"]", exp_allele_freqs[i], act_allele_freqs[i] )
    
    sql_db.Close()
    
    # Create a file with the results of the test
    output = {}
    output["messages"] = []
    for line in messages:
        output["messages"].append(line)
    
    report_fn = os.path.join( output_path, "report_sync_check.json" )
    with open(report_fn, 'w') as report_file:
        json.dump(output, report_file, indent=4)
    
    for line in messages:
        print(line)
    
    print("!!!!! Done checking allele frequencies !!!!!")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        ShowUsage()
        exit(0)

    application( sys.argv[1] )
    
# ---------------------------------------------------------------------------------------
# ---------------------------------------------------------------------------------------
