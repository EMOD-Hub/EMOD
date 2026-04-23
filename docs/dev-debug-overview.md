# Overview

When you build Eradication.exe or Eradication binary for Linux from the EMOD source code, it's important to debug the
code and run regression tests to be sure your changes didn't inadvertently change EMOD functionality.
You can run simulations directly from Visual Studio to step through the code, troubleshoot any build
errors you encounter, and run the standard EMOD regression tests or create a new regression test.

!!! warning
    If you modify the source code to add or remove configuration or campaign parameters, you may
    need to update the code used to produce the schema. You must also verify that your simulations
    are still scientifically valid.
