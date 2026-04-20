==================
Regression testing
==================

After building the |exe_l|, it's important to verify that |exe_s| is performing properly. Regression
testing is a method by which the built code is tested to see if it has "regressed" or moved
backwards in any way, such as previously reported (and fixed) issues reappearing.

Within the |EMOD_s| Regression_ directory there are many subdirectories that correspond to different
disease scenarios in a variety of locations. Each of these contains the configuration and campaign
files needed to run the simulation and the reference output, which represents the expected results.
These reference outputs have been calculated by the scientific researchers modeling each scenario.
Configurations and campaigns that use base and overlay files will be flattened as part of the
regression tests.

|EMOD_s| regression testing consists of running one or more of these simulations and comparing the
output to the reference output. If the two outputs are comparable, the test passes; if they differ,
the test fails. Because |EMOD_s| is :term:`stochastic`, a passing test will fall within some
acceptable range of the reference output, rather than be an identical match. If a regression test
fails, |EMOD_s| will produce a matplotlib chart of the first 16 channels in the InsetChart.json
:term:`output report`. You can then review the charts to identify the problem. Base and overlay configuration files will be flattened

If you want to quickly compare a simulation output to the reference output, you can also run any of
the regression scenarios as a typical simulation, as described in 
:doc:`emod-generic:software-run-simulation` for generic, :doc:`emodpy-hiv:emod/software-run-simulation` 
for HIV, or :doc:`emodpy-malaria:emod/software-run-simulation` for malaria. However,
this will not include the comparison and pass/fail evaluation that regression_test.py conducts. In
addition, if you choose to do this, be sure to specify a different output directory, such as
"testing", so as not to overwrite the reference output.

.. toctree::
   :maxdepth: 2

   dev-regression-run
   dev-regression-new

.. _Regression: https://github.com/EMOD-Hub/EMOD/tree/main/Regression
