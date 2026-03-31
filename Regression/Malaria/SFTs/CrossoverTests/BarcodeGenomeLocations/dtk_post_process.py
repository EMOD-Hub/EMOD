
import dtk_test.dtk_malaria_support as mal_sup
import dtk_test.dtk_sft as dtk_sft


"""
Verifying that Barcode_Genome_Locations are used correctly. We expect the index of the crossover on the barcode
to be the same at the index of the first location that's higher in value than the obligate. 
For this test, we have only one crossover at the obligate (controlled by Crossover_Gamma_K and Theta)
ex:
index:                      0   1   2  3   4   5
Barcode_Genome_Locations = [1, 15, 20, 35, 60, 70]
obligate = 45
we expect the crossover index to be 4 since the obligate, and therefore the crossover falls between index 3 and 4, 
which means that location 60, index 4 is from the other chromatid
 
"""

config_name = "Config_Name"
barcode_locations = "Barcode_Genome_Locations"


def find_crossover(children_group):
    for child in range(1, len(children_group)):
        kid = list(children_group[child])
        compare_gene = kid[0]
        for gene_location in range(len(kid)):
            new_gene = kid[gene_location]
            if new_gene != compare_gene:
                return gene_location

    return -1  # if we didn't find a crossover location


def create_report_file(children, param_obj, report_name, debug):
    success = True
    fails = []
    fails_log_file = "DEBUG_fails_log.txt"
    with open(report_name, "w") as outfile:
        outfile.write(f"Running test: {param_obj[config_name]} : \n")
        if not children:
            success = False
            outfile.write("BAD: No logging data for child genes found.\n")
        else:
            barcode = param_obj[barcode_locations]
            for num in range(len(children)):
                obligate = int(children[num][0])
                obligate_crossover_at = find_crossover(children[num])
                if obligate_crossover_at == -1:
                    success = False
                    outfile.write(f"BAD: We didn't find a crossover for obligate {obligate}. That's bad. There should "
                                  f"always be one!\n")
                else:
                    expected_crossover = -1
                    for i in range(len(barcode)):
                        if barcode[i] >= obligate:
                            expected_crossover = i
                            break
                    if expected_crossover == -1:
                        success = False
                        outfile.write(f"BAD: We didn't find a barcode index for obligate {obligate}. That's bad. There "
                                      f"should be one!\n")
                    else:
                        if obligate_crossover_at != expected_crossover:
                            success = False
                            outfile.write(f"BAD: The crossover found in barcode was at {obligate_crossover_at}, but we "
                                          f"expected it at {expected_crossover} based on {barcode_locations}. "
                                          f"Please check the {fails_log_file} file.\n")
                        elif debug:
                            outfile.write(f"GOOD: The crossover found in barcode was at {obligate_crossover_at} and we "
                                          f"expected it at {expected_crossover} based on {barcode_locations}. \n")

                fails.append(children[num])

            if not success:
                with open(fails_log_file, "w") as failfile:
                    for group in fails:
                        for i in range(len(group)):
                            failfile.write(f"{group[i]}\n")
                        failfile.write("\n")

        outfile.write(dtk_sft.format_success_msg(success))


def application(output_folder="output", stdout_filename="test.txt",
                config_filename="config.json",
                report_name=dtk_sft.sft_output_filename,
                debug=False):
    if debug:
        print("output_folder: " + output_folder)
        print("stdout_filename: " + stdout_filename + "\n")
        print("config_filename: " + config_filename + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    dtk_sft.wait_for_done()
    children = mal_sup.parse_output_file_for_children(stdout_filename, debug)
    param_obj = mal_sup.load_genetics_parameters(config_filename, debug)
    create_report_file(children, param_obj, report_name, debug)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-r', '--reportname', default=dtk_sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', action='store_true', help="Turns on debugging")
    args = parser.parse_args()

    application(output_folder=args.output, stdout_filename=args.stdout,
                config_filename=args.config, report_name=args.reportname, debug=args.debug)
