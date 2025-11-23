# -*- mode: python; -*-
# This Python script, SConscript, invoked by the SConstruct in this directory,
#
# 1. delegates to other per-module SConscript files for executable and library 
# (static and/or dynamic)
import os
import sys
import shutil

Import('env')

def InstallEmodules(src, dst):
    
    print( "\nInstalling from " + src + " to " + dst + "..." )
    if os.path.isfile(dst):
        print( "Warning: " + dst + " is a file\n" )
        return;

    if os.path.exists(dst) != True:
        print( "Creating " + dst + " in " + os.getcwd() )
        os.mkdir(dst)

    srcfiles = os.path.join(src,'*.dll')
    for root, dirs, files in os.walk(src):
        for file in files:
            if file.endswith('.dll') or file.endswith('.exe'):
                full_fn = os.path.join(root,file);
                print( "copying: " + full_fn )
                shutil.copy2(full_fn, dst);
    
# if --install is on, just copy the dlls (assumed there already) and finish
dst_path = env['Install']
if dst_path != "":
    InstallEmodules(Dir('.').abspath, dst_path)
    #InstallEmodules(Dir('#').abspath, dst_path)
    print("Finished installing.\n")
    sys.exit(0)

env.Prepend( CPPPATH=[
              "#/Eradication",
              "#/interventions",
              "#/campaign",
              "#/baseReportLib",
              "#/utils",
              "#/libsqlite",
              "#/cajun/include",
              "#/rapidjson/include",
              "#/rapidjson/modp",
              "#/snappy",
              "#/lz4/lib"])

# set the common libraries
env.Prepend( LIBPATH = [
              "$BUILD_DIR/reporters", 
              "$BUILD_DIR/baseReportLib",
              "$BUILD_DIR/campaign", 
              "$BUILD_DIR/cajun", 
              "$BUILD_DIR/libsqlite", 
              "$BUILD_DIR/snappy", 
              "$BUILD_DIR/lz4", 
              "$BUILD_DIR/utils"])

env.Prepend( LIBS=[
              "reporters",
              "baseReportLib", 
              "campaign",
              "cajun",
              "libsqlite",
              "snappy",
              "lz4",
              "utils"])

# First static libs
print( "Build static libraries baseReportLib, cajun, campaign, coreLib, sqlite, snappy, and utils lib's." )
SConscript( [ 'baseReportLib/SConscript',
              'cajun/SConscript',
              'campaign/SConscript',
              'reporters/SConscript',
              'Eradication/SConscript_coreLib',
              'libsqlite/SConscript',
              'snappy/SConscript',
              'lz4/SConscript',
              'utils/SConscript' ])


# Finally executable
SConscript('Eradication/SConscript')

def OptionalScript(sconscript_name):
    sconscript_path = os.path.join(Dir('#').abspath, sconscript_name)
    if os.path.isfile(sconscript_path):
        SConscript(sconscript_name)
    else:
        print("Skipping missing script: '{0}'".format(sconscript_path))

disease = "ALL"
if 'Disease' in env and len(env['Disease']) > 0:
    disease = env["Disease"]

if( (disease == "ALL") or (disease == "STI") or (disease == "HIV") ):
    OptionalScript('reporters/SConscript_STI_RelationshipQueue')

if( disease == "ALL"):
    OptionalScript('UnitTest++/SConscript')
    OptionalScript('componentTests/SConscript')
