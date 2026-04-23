# Debug sims in Visual Studio

If you have modified the EMOD source code, you can run simulations directly from Visual Studio
as part of debugging your changes by using the built-in debugger. For example, you may want to run
one or more of the simulations in the [Regression][regression_example] directory to verify that the results match those
obtained prior to your changes.

1.  Open the EradicationKernel solution in Visual Studio.
1.  On the **Solution Explorer** pane, right-click the **Eradication** project and select **Properties**.
1.  On the **Property Pages** window, in the **Configuration Properties** pane, click **Debugging**.
1.  Set the **Command Arguments** and **Working Directory** to the appropriate values and click **OK**.
    For more information, see emod-generic:software-simulation-cli for generic,
    emodpy-hiv:emod/software-simulation-cli for HIV, or
    emodpy-malaria:emod/software-simulation-cli for malaria.

1.  On the **Debug** menu, do one of the following:

    -   Click **Start Without Debugging** to run the simulation in Release
        mode, where the simulation runs with essentially the same behavior and performance as
        running it at the command line.

    -   Click **Start Debugging** to run the simulation in Debug mode, where you have
        the ability to add break points and step through the code, inspecting the values of different
        variables throughout the simulation.
