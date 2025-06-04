#!/usr/bin/python

import pandas as pd
import os


def mortality_data_compare(output_folder="output"):
    """
    This function compares the mortality data from two different sources:
    - ReportHIVByAgeAndGender.csv
    - HIVMortality.csv

    Note: This function assumes a timestep of 30.416666667 days (which is approximately a month) and
    Report_HIV_Period is set to 365 days (which makes actual reporting period of 182.5 days or 6 timesteps/months).

    Args:
        output_folder:

    Returns:

    """
    hiv_by_age_and_gender = pd.read_csv(os.path.join(output_folder, "ReportHIVByAgeAndGender.csv"))
    hiv_mortality = pd.read_csv(os.path.join(output_folder, "HIVMortality.csv"), low_memory=False)

    # filtering to death columns and time, summing by "year" (every 6 months), renaming columns for consistency
    hbaag = hiv_by_age_and_gender[["Year", " Died", " Died_from_HIV"]]
    hbaag = hbaag.groupby('Year').sum().reset_index()
    hbaag = hbaag.rename(columns={" Died": "total_death", " Died_from_HIV": "hiv_death"})

    # filtering to death columns and time, summing by times (monthly)
    # adding columns for total deaths which is a count of entries for every time column
    hiv_mortality = hiv_mortality[["Death_time", "Death_was_HIV_cause"]]
    hiv_mortality = hiv_mortality.groupby('Death_time').agg(
        hiv_death=('Death_was_HIV_cause', 'sum'),
        total_death=('Death_was_HIV_cause', 'count')
    ).reset_index()
    # Create a grouping column for every 6-month interval
    hiv_mortality['6months'] = hiv_mortality['Death_time'] // (365 / 2)
    # Group by the interval and sum the values
    hiv_mortality = hiv_mortality.groupby('6months').agg(
        hiv_death=('hiv_death', 'sum'),
        total_death=('total_death', 'sum')).reset_index()

    # calculating the year from the time
    """
    Using 1960.92 as the base year, which is the midpoint TIMESTAMP of the reporting intervals
    """
    hiv_mortality["Year"] = hiv_mortality["6months"] / 2 + 1960.92  # adding in 0.5 year increments
    hiv_mortality = hiv_mortality[["Year", "total_death", "hiv_death", "6months"]]

    # HBAAG - HIVByAgeAndGender
    # HM - HIVMortality
    with open(os.path.join(output_folder, "hiv_mortality_compare.csv"), "w") as f:
        f.write("Year,HBAAG_total,HM_total,HBAAG_hiv,HM_hiv \n")
        for index, row in hiv_mortality.iterrows():
            # checking if the year exists in both datasets
            if row["Year"] in hbaag["Year"].values:
                hbaag_row = hbaag.loc[hbaag["Year"] == row["Year"]]
                f.write(f"{row['Year']},"
                        f"{hbaag_row['total_death'].values[0]},"
                        f"{row['total_death']},"
                        f"{hbaag_row['hiv_death'].values[0]},"
                        f"{row['hiv_death']}\n")
            else:
                # If the year does not exist in HBAAG, write HM data only
                f.write(f"{row['Year']},, {row['total_death']},, {row['hiv_death']}\n")


def application(output_folder):
    mortality_data_compare(output_folder)
    print("post processing complete")


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    args = parser.parse_args()

    application(output_folder=args.output)
