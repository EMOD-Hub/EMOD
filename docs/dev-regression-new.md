# Create a new regression test

You can create a new regression based off one of the ones included with the EMOD source code
using the steps below.

1.  Under the [Regression](https://github.com/EMOD-Hub/EMOD/tree/main/Regression) directory, create a new subdirectory.
1.  Copy the contents of the regression test that you want to base the new test on into the subdirectory.
1.  Modify the configuration, campaign, and demographic files as desired.
1.  Create the reference output by doing one of the following:

    -   Modify the InsetChart.json file to match the output you expect.
    -   Run simulations until you have an acceptable InsetChart.json that you wish to use as
        the reference.
