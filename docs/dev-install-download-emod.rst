=================================
Download the |EMOD_s| source code
=================================

The |EMOD_s| source code is available on GitHub. The |EMOD_s| source includes the source code,
Visual Studio solution, sample configuration files, as well as :term:`regression test` and other
files needed to fully build and test the |exe_l|.

You can have multiple versions of the |EMOD_s| source code in separate directories on your local
computer. For example, you might want to download a new release of |EMOD_s| but also keep a previous
release of the source. In the following examples, the source code is downloaded to the directory
|EMOD_s| at C:/IDM, but you can save to any location you want.

You can use any Git client of your choice to clone the EMOD repository from from GitHub. These instructions walk through the process using Git GUI or Git Bash if you are new to using Git.

Install Git GUI and Git Bash
============================

To install Git GUI and Git Bash, download a 64-bit version of Git from https://git-scm.com/download.
On the **Select Components** installer window, you can select one or both of **Git GUI Here** for a
GUI or **Git Bash Here** for a command window.

Use Git GUI to download the |EMOD_s| source
===========================================

#.  Launch the Git GUI application and click **Clone Existing Repository**.

#.  From the **Clone Existing Repository** window:

    #.  In **Source Location**, enter https://github.com/EMOD-Hub/EMOD

    #.  In **Target Directory**, enter the location and target directory name: C:/IDM/EMOD

    #.  Click **Clone**. Git GUI will create the directory and download the source code.
#.  Close the Git GUI window when the download completes.

Use Git Bash to download the |EMOD_s| source
============================================

.. note::

    For a list of the Git Bash commands, type ``git help git`` or type ``git help
    <command>`` for information about a specific command.

#.  Launch the Git Bash application. From the command line:

    #.  Navigate to the location where you want to save your copy of the |EMOD_s| source code::

            cd C:/IDM

    #.  Clone the repository from GitHub::

            git clone https://github.com/EMOD-Hub/EMOD

Git Bash will copy the |EMOD_s| source to a directory named |EMOD_s| by default.

Verify that all directories on https://github.com/EMOD-Hub/EMOD are now reflected
on your local clone of the repository.


.. include:: reuse/install-inputdata.txt