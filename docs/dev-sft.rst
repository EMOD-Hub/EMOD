==========================
Scientific feature testing
==========================

Scientific feature testing (SFT) verifies that |EMOD_s| features are functioning as expected. That
is, they are requirements-based tests of model features that are quantitatively verified. They are
implemented as a Python post-processing script named dtk_post_process.py. 

All of the files can be found in the Regression directory of the EMOD_ GitHub repository.

Run locally
===========

If you are running locally without access to an HPC, do one the following depending on your operating system:

Run on Windows using regression_test.py
---------------------------------------

#.  In the Regression directory, edit the regression_test.cfg file to list the directories where you want the simulations to be run and output saved.

#.  Make a file listing all of the SFTs you want to run, using on of the files in Regression\<sim>_science.json as an example.

#.  From the Regression directory in a command prompt window, run the following, adding ``--scons`` if you built the |exe_s| using SCONS::

        python regression_test.py my_sfts.json --local

    .. note::

        Enter ``python regression_test.py --help`` for a list of all arguments you can use with the testing script.     

Run on Windows using run_test.cmd
---------------------------------

#.  Generate a single config.json file from a base file and the parameter_overrides.json that contains.
    For instructions, see :doc:`emod-generic:software-configuration-overlay` for generic. 
    :doc:`emodpy-hiv:emod/software-configuration-overlay` for HIV, or 
    :doc:`emodpy-malaria:emod/software-configuration-overlay` for malaria. 

#.  Navigate to the directory that contains your config.json file and other test files.

    .. note::

        Verify in the config.json file that the paths to the demographics and other additional input files are correct.

#.  In a command prompt window, enter the following, substituting your path to the testing script 
    as necessary::

        ../../run_test.cmd 


Run on |Ubuntu| using run_test.sh
---------------------------------

#.  Generate a single config.json file from a base file and the parameter_overrides.json that contains.
    For instructions, see :doc:`emod-generic:software-configuration-overlay` for generic. 
    :doc:`emodpy-hiv:emod/software-configuration-overlay` for HIV, or 
    :doc:`emodpy-malaria:emod/software-configuration-overlay` for malaria. 

#.  Navigate to the directory that contains your config.json file and other test files.

    .. note::

        Verify in the config.json file that the paths to the demographics and other additional input files are correct.

#.  In a command prompt window, enter the following, substituting your path to the testing script
    as necessary::

        ../../run_test.sh 
  
.. _EMOD: https://github.com/EMOD-Hub/EMOD/tree/main/Regression
