import json, sys, os, shutil
from dtk_report_vector_migration_support import ConvertReportVectorMigration
from dtk_vector_stats_support import JustMaleFemalePopulations

def application(output_path):
    files_to_convert = ["ReportVectorMigration_female.csv", "ReportVectorMigration_male.csv"]
    for file in files_to_convert:
        ConvertReportVectorMigration(output_path, file)

    JustMaleFemalePopulations(output_path, "ReportVectorStats.csv")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("need output path")
        exit(0)

    output_path = sys.argv[1]
    application( output_path )
