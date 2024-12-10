# convert_csv_to_bin_vector_migration.py
# -----------------------------------------------------------------------------
# This script converts a CSV formatted txt file to an EMOD binary-formatted migration file.
# It also creates the required metadata file.
#
# The CSV file can have several configuration of columns:
#
# 1) Headers:  FromNodeID, ToNodeID, Rate (Average # of Trips Per Day)
# If the csv/text file does not have column headers and three entries, this is the format we assume.
# This can be used for human and vector migration. The Rate is for any/all agents regardless of sex or age.
#
# 2) Headers:  FromNodeID, ToNodeID, RateMales, RateFemales
# If the csv/text file does not have column headers and four entries, this is the format we assume.
# RateMales are rates for male migration, RateFemales for female migration and are Average # of Trips Per Day.
# This can be used for human and vector migration when using sex-based migration without age.
#
# 3) Headers:  FromNodeID, ToNodeID, [], arrays denoting Allele_Combinations
# Allele_Combinations example: [["a1", "a1"], ["b1", "b1"]];  [["X1","Y2"]]; [["*", "a0"], ["X1", "Y1"]]
# Due to use of commas in headers, it is best to use Excel to create them (or look at a sample text csv)
# This is to support VECTOR_MIGRATION_BY_GENETICS.
# Headers are required for this csv file.
# The first (empty, []) array is used as a "default rate" if the vector's genetics doesn't match any of the
# Allele_Combinations. The other column headers denote the rate that the vector will travel at if it matches the
# Allele_Combination listed. Vectors are checked against Allele_Combinations from most-specific, to least-specific,
# regardless of the order in the csv file. Allele_Combinations can, but don't have to, include sex-alleles. Without
# specified sex-alleles, any vector that matches the alleles regardless of sex will travel at that rate.
#
# The FromNodeIDs and ToNodeIDs are the external ID's found in the demographics file.
# Each node ID in the migration file must exist in the demographics file.
# One can have node ID's in the demographics that don't exist in the migration file.
#
# The CSV file does not have to have the same number of entries for each FromNodeID.
# The script will find the FromNodeID that has the most and use that for the
# DestinationsPerNode. The binary file will have DestinationsPerNode entries
# per node.
#
# -----------------------------------------------------------------------------

import collections
import datetime
import json
import os
import struct
import sys
import pandas as pd
from enum import Enum
import ast


class GenderDataType(Enum):
    SAME_FOR_BOTH_GENDERS = "SAME_FOR_BOTH_GENDERS"
    ONE_FOR_EACH_GENDER = "ONE_FOR_EACH_GENDER"
    VECTOR_MIGRATION_BY_GENETICS = "VECTOR_MIGRATION_BY_GENETICS"

class MetaData:
    def __init__(self):
        self.node_count = 0
        self.offset_str = ""
        self.max_destinations_per_node = 0
        self.gender_data_type = None
        self.filename_out = ""
        self.has_headers = False
        self.num_columns = 0
        self.allele_combinations = []
        self.data_df = None
        self.ref_id = None

def ShowUsage():
    print('\nUsage: %s [input-migration-csv] [idreference]' % os.path.basename(sys.argv[0]))


# -----------------------------------------------------------------------------
# WriteMetadataFile
# -----------------------------------------------------------------------------
def WriteMetadataFile(metadata):
    output_json = collections.OrderedDict([])

    output_json["Metadata"] = {}
    output_json["Metadata"]["IdReference"] = metadata.ref_id
    output_json["Metadata"]["DateCreated"] = datetime.datetime.now().ctime()
    output_json["Metadata"]["Tool"] = os.path.basename(sys.argv[0])
    output_json["Metadata"]["DatavalueCount"] = metadata.max_destinations_per_node
    output_json["Metadata"]["GenderDataType"] = metadata.gender_data_type.value
    if metadata.allele_combinations:
        output_json["Metadata"]["AlleleCombinations"] = metadata.allele_combinations
    output_json["Metadata"]["NodeCount"] = metadata.node_count
    output_json["NodeOffsets"] = metadata.offset_str

    metadata_filename = metadata.filename_out + ".json"
    with open(metadata_filename, 'w') as file_out_json:
        json.dump(output_json, file_out_json, indent=4)


def is_number(value):
    try:
        float(value)  # Attempt to convert to a float
        return True
    except ValueError:
        return False


def GetSummaryData(metadata):
    # ----------------------------
    # collect data from CSV file
    # ----------------------------
    data_df = metadata.data_df
    metadata.num_columns = len(data_df.iloc[0])
    if metadata.num_columns < 3:
        raise ValueError(f"There are {metadata.num_columns} in the file, but we expect at least three. Please review comments"
                         f" for expected column configurations and try again.")
    if not metadata.has_headers:  # no column headers
        if metadata.num_columns == 3:
            print(f"File doesn't seem to have headers, and with {metadata.num_columns} columns, "
                  "we are assuming 'FromNodeID', 'ToNodeID', 'Rate' column configuration.")
            metadata.gender_data_type = GenderDataType.SAME_FOR_BOTH_GENDERS
        elif metadata.num_columns == 4:
            print(f"File doesn't seem to have headers, and with {metadata.num_columns} columns, "
                  "we are assuming 'FromNodeID', 'ToNodeID', 'RateMales', 'RateFemales' column configuration.")
            metadata.gender_data_type = GenderDataType.ONE_FOR_EACH_GENDER
        else:
            raise ValueError(f"File doesn't seem to have headers, and with {metadata.num_columns} columns, it is not "
                             f"obvious what the column configuration should be. If you are trying to create a "
                             f"VECTOR_MIGRATION_BY_GENETICS file, please add headers as shown in the comments.")
    else:  # has headers, force user to use one of the three formats
        if 'FromNodeID' not in headers[0] or 'ToNodeID' not in headers[1]:
            raise ValueError(f"With headers, we expect first two column headers to be 'FromNodeID', 'ToNodeID', but "
                             f"they are {headers[0]} and {headers[1]}.")
        elif metadata.num_columns == 3:
            if 'Rate' not in headers[2]:
                raise ValueError(f"With headers and {metadata.num_columns}, we expect the third column to be 'Rate', "
                                 f"but it is {headers[2]}, please check and correct.")
            else:
                metadata.gender_data_type = GenderDataType.SAME_FOR_BOTH_GENDERS
        elif metadata.num_columns > 3:
            if "[]" in headers[2]:
                metadata.gender_data_type = GenderDataType.VECTOR_MIGRATION_BY_GENETICS
                for alleles in data_df.columns.tolist()[2:]:
                    metadata.allele_combinations.append(ast.literal_eval(alleles))
            elif metadata.num_columns == 4:
                if 'RateMales' in headers[2] and 'RateFemales' in headers[3]:
                    metadata.gender_data_type = GenderDataType.ONE_FOR_EACH_GENDER
                else:
                    raise ValueError(f"With column headers and {metadata.num_columns} and not Allele_Combinations in "
                                     f"headers, we expect 'RateMales' in third column and 'RateFemales' in forth "
                                     f"column, but they are"
                                     f" {headers[2]} and {headers[3]}, please check and correct.")
            else:
                raise ValueError(f"File has column headers with {metadata.num_columns} columns, but does not have the "
                                 f"expected headers. Please review the headers expected in the comments, correct, "
                                 f"and try again.")

    # -------------------------------------------------------------------------
    # Find the list node that individuals can migrate from
    # Also find the maximum number of nodes that one can go to from a give node.
    # This max is used in determine the layout of the binary data.
    # -------------------------------------------------------------------------
    from_node_id_list = data_df[data_df.columns[0]].unique().tolist()
    for from_node_id in from_node_id_list:
        to_node_id_list = data_df[data_df.iloc[:, 0] == from_node_id].iloc[:, 1]
        if len(to_node_id_list) != len(to_node_id_list.unique()):
            raise ValueError(f"For 'FromNodeID' = {from_node_id}, there are non-unique 'ToNodeIDs'.")
        if len(to_node_id_list) > metadata.max_destinations_per_node:
            metadata.max_destinations_per_node = len(to_node_id_list)

    # -------------------------------------------------------------------
    # Create NodeOffsets string
    # This contains the location of each From Node's data in the bin file
    # -------------------------------------------------------------------
    for from_node_id in from_node_id_list:
        metadata.offset_str += '%0.8X' % from_node_id
        metadata.offset_str += '%0.8X' % (metadata.node_count * metadata.max_destinations_per_node * 12) # 12 -> sizeof(uint32_t) + sizeof(double)
        metadata.node_count += 1

    # return metadata

# -----------------------------------------------------------------------------
# WriteBinFileGender
# -----------------------------------------------------------------------------
def WriteBinFile(metadata):
    bin_file = open(metadata.filename_out, 'wb')
    data_df = metadata.data_df
    from_node_id_list = data_df[data_df.columns[0]].unique().tolist()
    for data_index in range(2, metadata.num_columns):
        for from_node_id in from_node_id_list:
            # Initialize with zeros
            to_node_id_array = [0] * metadata.max_destinations_per_node
            rates_array = [0] * metadata.max_destinations_per_node

            # Populate arrays with data
            to_node_id_list = data_df[data_df.iloc[:, 0] == from_node_id].iloc[:, 1].tolist()
            for index, to_node_id in enumerate(to_node_id_list):
                rate_list = data_df[(data_df.iloc[:, 0] == from_node_id) & (data_df.iloc[:, 1] == to_node_id)].iloc[:, data_index].to_list()
                if len(rate_list) > 1:  # should only have one entry for the to/from node id combination
                    raise ValueError(f"For FromNodeID {from_node_id} and ToNodeID {to_node_id}, there are multiple "
                                     f"rates in column index {data_index}.")
                to_node_id_array[index] = to_node_id
                rates_array[index] = rate_list[0]  # get the one rate

            # Format data into binary
            bin_data_id = struct.pack('I' * len(to_node_id_array), *to_node_id_array)
            bin_data_rt = struct.pack('d' * len(rates_array), *rates_array)

            bin_file.write(bin_data_id)
            bin_file.write(bin_data_rt)

    bin_file.close()



if __name__ == "__main__":
    if not (len(sys.argv) == 2 or len(sys.argv) == 3):
        ShowUsage()
        exit(0)

    filename_in = sys.argv[1]
    id_ref = "temp_ref_id"
    if len(sys.argv) == 3:
        id_ref = sys.argv[1]

    meta_data = MetaData()
    meta_data.ref_id = id_ref
    meta_data.data_df = pd.read_csv(filename_in)
    headers = meta_data.data_df.columns.tolist()
    if is_number(headers[0]):  # no headers
        meta_data.data_df = pd.read_csv(filename_in, header=None)
    else:
        meta_data.has_headers = True  # False by default
    meta_data.filename_out = filename_in.split(".")[0] + ".bin"

    GetSummaryData(meta_data)
    WriteBinFile(meta_data)
    WriteMetadataFile(meta_data)

    print(f"max_destinations_per_node = {meta_data.max_destinations_per_node}")
    print(f"Finished converting {filename_in} to {meta_data.filename_out} and +.json metadata file.")
    if len(sys.argv) == 2:
        print(f"IdReference in {meta_data.filename_out}.json file is set to a temporary value, please update it"
              f" to match your demographics.")
