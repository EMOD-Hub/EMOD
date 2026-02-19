MigrationTest

This directory contains migration files for using in regression testing.

To create a migration file with fixed rated, one can convert a CSV file
to the appropriate binary files.  See the following script for more information:

    DTK/Scripts/MigrationTools/convert_txt_to_bin.py


To create a migration file that is dependent on gender and age, one can create
a JSON formatted file and then convert it to the appropriate binary files.
See the following script for more information:

    DTK/Scripts/MigrationTools/convert_json_to_bin.py


