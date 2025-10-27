import json, sys, os, shutil
import dtk_node_demographics_support as dnds

def application( output_path ):
    dnds.ConvertReportNodeDemographics( output_path, "ReportNodeDemographicsMalariaGenetics.csv" )
    

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("need output path")
        exit(0)

    output_path = sys.argv[1]

    application( output_path )
