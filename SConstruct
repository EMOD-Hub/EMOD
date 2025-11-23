# This file configures the build environment, and then delegates to
# several subordinate SConscript files, which describe specific build rules.
#
# Simply type scons to build everything in DTK

import os
import re
import sys
import platform

import SCons.Tool.MSCommon.vc  as  scons_vc

def findSettingsSetup():
    sys.path.append( "." )
    sys.path.append( ".." )
    sys.path.append( "../../" )

msarch = "amd64"

# --- options ---

options = {}

options_topass = {}


def add_option( name, help, nargs, contributesToVariantDir,
                dest=None, default = None, type="string", choices=None ):

    if dest is None:
        dest = name

    AddOption( "--" + name , 
               dest=dest,
               type=type,
               nargs=nargs,
               action="store",
               choices=choices,
               default=default,
               help=help )

    options[name] = { "help" : help ,
                      "nargs" : nargs , 
                      "contributesToVariantDir" : contributesToVariantDir ,
                      "dest" : dest } 

def get_option( name ):
    return GetOption( name )

def set_option( name, value ):
    return SetOption(name, value)

def _has_option( name ):
    x = get_option( name )
    if x is None:
        return False

    if x == False:
        return False

    if x == "":
        return False

    return True

def has_option( name ):
    x = _has_option(name)

    if name not in options_topass:
        # if someone already set this, don't overwrite
        options_topass[name] = x

    return x

def get_variant_dir():
    
    a = []
    
    for name in options:
        o = options[name]
        if not has_option( o["dest"] ):
            continue
        if not o["contributesToVariantDir"]:
            continue
        
        if o["nargs"] == 0:
            a.append( name )
        else:
            x = get_option( name )
            x = re.sub( "[,\\\\/]" , "_" , x )
            a.append( name + "_" + x )
            
    s = "#build/${PYSYSPLATFORM}/"
    if len(a) > 0:
        a.sort()
        s += "/".join( a ) + "/"

    return s

def get_build_var():
    
    bv = ""
    if has_option( "Release" ):
        bv = "Release"
    elif has_option( "Debug" ):
        bv = "Debug"
    else:
        bv = "Release"

    return bv

# compiling options
add_option( "Release" , "release build" , 0 , True)
add_option( "Debug" , "debug build" , 0 , True )

# module/linking options
add_option( "DllDisease" , "build disease target dll" , 1 , True) #, Disease="Generic" )
add_option( "Disease" , "build only files for disease target " , 1 , True) #, Disease="Generic" )
add_option( "Report" , "build report target dll" , 1 , True) #, Report="Spatial" )
 
# installation options
add_option( "Install" , "install target dll into given directory" , 1 , True) #, Install="install dir" )

add_option( "TestSugar" , "Build in additional logging for scientific or other validation, might slow performance" , 0, True)

# current default is Release
Dbg = has_option( "Debug" )
Rel = has_option( "Release" )


# --- environment setup ---

s = "#build/${PYSYSPLATFORM}/"
bvar = get_build_var()
buildDir = s + bvar + "/"

def printLocalInfo():
    import sys, SCons
    print( "scons version: " + SCons.__version__ )
    print( "python version: " + " ".join( [ str(i) for i in sys.version_info ] ) )

printLocalInfo()


MSVC_ver = None
if os.sys.platform == 'win32':
    msvc_list = scons_vc.get_installed_vcs()

    if('14.5' in msvc_list):
        MSVC_ver = '14.5'
    elif('14.3' in msvc_list):
        MSVC_ver = '14.3'
    else:
        raise RuntimeError("No compatable version of MSVC Build Tools found.")

pa = platform.architecture()
pi = os.sys.platform
if pa[0].find("64") != -1:
    pi = 'x64'
path = os.environ['PATH']
env = Environment( BUILD_DIR=buildDir,
                   DIST_ARCHIVE_SUFFIX='.tgz',
                   MSVS_ARCH=msarch ,
                   TARGET_ARCH=msarch ,
                   PYSYSPLATFORM=pi,
                   MSVSPROJECTSUFFIX='.vcxproj' ,
                   MSVC_VERSION=MSVC_ver
                   )

if not(Dbg) and not(Rel):
    Rel = True
    env["Debug"] = False
    env["Release"] = True
else:
    env["Debug"] = Dbg
    env["Release"] = Rel

print( "Rel=" + str(Rel) + " Dbg=" + str(Dbg) )

#print "BUILD_DIR=" + env['BUILD_DIR'] + " pi=" + pi
env['BUILD_VARIANT'] = bvar

#libdeps.setup_environment( env )

env['EXTRACPPPATH'] = []
env["LIBPATH"] = []
env['EXTRALIBPATH'] = []

if os.sys.platform == 'win32':
    if Dbg:
        print( "----------------------------------------------------" )
        print( "Visual Studio debug build not supported using SCONS." )
        print( "Please use the IDE for debugging." )
        print( "----------------------------------------------------" )
        Exit(-1)

    print('MSVC Version: ',env['MSVC_VERSION'])

    env['OS_FAMILY'] = 'win'
    
    # Boost
    env.Append( EXTRACPPPATH=[ os.environ['IDM_BOOST_PATH'] ] )

    env.Append( EXTRACPPPATH=[ os.environ['IDM_PYTHON3X_PATH']+"/include" ] )
    env.Append( EXTRALIBPATH=[ os.environ['IDM_PYTHON3X_PATH']+"/libs" ] )
    env.Append( LIBS=["python3.lib"] )
    env.Append( EXTRACPPPATH=[ "#/Dependencies/ComputeClusterPack/include" ] )
    env.Append( EXTRALIBPATH=[ "#/Dependencies/ComputeClusterPack/Lib/amd64" ] )
    env.Append( LIBS=["msmpi.lib"] )

else:
    env['ENV']['PATH'] = path
    env['OS_FAMILY'] = 'posix'
    env['CC'] = "mpicc"
    env['CXX'] = "mpicxx"
    env.Append( CCFLAGS=["-fpermissive"] )
    env.Append( CCFLAGS=["--std=c++0x"] )
    env.Append( CCFLAGS=["-w"] )
    env.Append( CCFLAGS=["-ffloat-store"] )
    env.Append( CCFLAGS=["-Wno-unknown-pragmas"] )
    env.Append( CCFLAGS=["-mavx2"] )

    # Python
    if(sys.version_info.minor == 9):
        env.Append( LIBS=["python3.9"] )
        env.Append( EXTRACPPPATH=["/usr/include/python3.9"] )
    elif(sys.version_info.minor == 10):
        env.Append( LIBS=["python3.10"] )
        env.Append( EXTRACPPPATH=["/usr/include/python3.10"] )
    elif(sys.version_info.minor == 11):
        env.Append( LIBS=["python3.11"] )
        env.Append( EXTRACPPPATH=["/usr/include/python3.11"] )
    elif(sys.version_info.minor == 12):
        env.Append( LIBS=["python3.12"] )
        env.Append( EXTRACPPPATH=["/usr/include/python3.12"] )
    elif(sys.version_info.minor == 13):
        env.Append( LIBS=["python3.13"] )
        env.Append( EXTRACPPPATH=["/usr/include/python3.13"] )
    else:
        raise RuntimeError("Unsupported python version")

# ---- other build setup -----

platform = os.sys.platform
if "uname" in dir(os):
    processor = os.uname()[4]
else:
    processor = "x86_64"

env['PROCESSOR_ARCHITECTURE'] = processor

nixLibPrefix = "lib"

dontReplacePackage = False
isBuildingLatest = False

def findVersion( root , choices ):
    if not isinstance(root, list):
        root = [root]
    for r in root:
        for c in choices:
            if ( os.path.exists( r + c ) ):
                return r + c
    raise RuntimeError("can't find a version of [" + repr(root) + "] choices: " + repr(choices))

if os.sys.platform.startswith("linux"):
    linux = True
    static = True
    platform = "linux"

    if os.uname()[4] == "x86_64":
        linux64 = True
        nixLibPrefix = "lib64"
        env.Append( EXTRALIBPATH=["/usr/lib64" , "/lib64" ] )

    env.Append( LIBS=["pthread", "dl", "m", "sqlite3"] )
    env.Append( EXTRALIBPATH=[ "/usr/local/lib", "/usr/lib64/mpich/lib" ] )

    if static:
        #env.Append( LINKFLAGS=" -static " )
        env.Append( LINKFLAGS=" -rdynamic " )
    if Dbg:
        env.Append( CCFLAGS=["-O0", "-g", "-fPIC"] )
        env.Append( CPPDEFINES=["_DEBUG"] )
    else:
        env.Append( CCFLAGS=["-O3", "-fPIC"] )

    # enable AES support or get [wmmintrin.h:34:3: error: #error "AES/PCLMUL instructions not enabled"]
    env.Append( CCFLAGS=["-maes"] )

    # trying to avoid [smmintrin.h:31:3: error: #error "SSE4.1 instruction set not enabled"]
    # trying to avoid [tmmintrin.h:31:3: error: #error "SSSE3 instruction set not enabled"]
    env.Append( CCFLAGS=[ "-msse", "-msse3", "-msse4.1" ] )

elif "win32" == os.sys.platform:
    windows = True

    env['DIST_ARCHIVE_SUFFIX'] = '.zip'

    # PSAPI_VERSION relates to process api dll Psapi.dll.
    env.Append( CPPDEFINES=["_CONSOLE"] )
    env.Append( CPPDEFINES=[ "BOOST_ALL_NO_LIB" ] )
    env.Append( CPPDEFINES=[ "IDM_EXPORT"] )
    env.Append( CPPDEFINES=[ "NDEBUG"] )
    env.Append( CPPDEFINES=[ "_UNICODE" ] )
    env.Append( CPPDEFINES=[ "UNICODE" ] )
    env.Append( CPPDEFINES=[ "WIN32" ] )

    # some warnings we don't like:
    # c4355
    # 'this' : used in base member initializer list
    #    The this pointer is valid only within nonstatic member functions. It cannot be used in the initializer list for a base class.
    # c4800
    # 'type' : forcing value to bool 'true' or 'false' (performance warning)
    #    This warning is generated when a value that is not bool is assigned or coerced into type bool. 
    # c4267
    # 'var' : conversion from 'size_t' to 'type', possible loss of data
    # When compiling with /Wp64, or when compiling on a 64-bit operating system, type is 32 bits but size_t is 64 bits when compiling for 64-bit targets. To fix this warning, use size_t instead of a type.
    # c4244
    # 'conversion' conversion from 'type1' to 'type2', possible loss of data
    #  An integer type is converted to a smaller integer type.
    
    # this would be for pre-compiled headers, could play with it later
    # DMB - I tried to get this to work, but the conftest doesn't have the header file.
    #     - I tried moving this append to after we test the compiler but then I ran into
    #       it needing the stdafx.pch.  Punt.
    #env.Append( CCFLAGS=['/Yu"stdafx.h"'] )

    # docs say don't use /FD from command line (minimal rebuild)
    # /Gy         : function level linking (implicit when using /Z7)
    # /Z7         : debug info goes into each individual .obj file -- no .pdb created 
    # /MD         : Causes your application to use the multithread, dll version of the run-time library (LIBCMT.lib)
    # /MT         : use static lib
    # /O2         : optimize for speed (as opposed to size)
    # /MP         : build with multiple processes
    # /Gm-        : No minimal build
    # /WX-        : Do NOT treat warnings as errors
    # /Gd         : the default setting, specifies the __cdecl calling convention for all functions
    # /EHsc       : exception handling style for visual studio
    # /W3         : warning level
    # /WX         : abort build on compiler warnings
    # /bigobj     : for an object file bigger than 64K - DMB It is in the VS build parameters but it doesn't show up in the Command Line view
    # /fp:precise : Specifies floating-point behavior in a source code file. - DMB - I tried fp:fast and zero improvement
    # Not including debug information for SCONS builds
    env.Append( CCFLAGS= [ "/EHsc", "/errorReport:none"] )
    env.Append( CCFLAGS= [ "/fp:precise" ] )
    env.Append( CCFLAGS= [ "/Gd", "/Gm-", "/GS", "/Gy" ] )
    env.Append( CCFLAGS= [ "/MD", "/MP" ] )
    env.Append( CCFLAGS= [ "/O2", "/Oi", "/Ot" ] )
    env.Append( CCFLAGS= [ "/W3", "/WX-"] )
    env.Append( CCFLAGS= [ "/Zc:inline", "/Zc:forScope", "/Zc:wchar_t" ] )

    # /Zi    : debug info goes into a PDB file
    # /FS    : force to use MSPDBSRV.EXE (serializes access to .pdb files which is needed for multi-core builds)
    # /DEBUG : linker to create a .pdb file which WinDbg and Visual Studio will use to resolve symbols if you want to debug a release-mode image.
    # NOTE1: This means we can't do parallel links in the build.
    # NOTE2: /DEBUG and Dbghelp.lib go together with changes in Exception.cpp which adds the ability to print a stack trace.
    env.Append( LINKFLAGS=[ "/MACHINE:X64", "/MANIFEST", "/HEAP:\"100000000\"\",100000000\" ", "/OPT:REF", "/OPT:ICF ", "/DYNAMICBASE:NO", "/SUBSYSTEM:CONSOLE"] )
    env.Append( LINKFLAGS=[ "/ERRORREPORT:NONE", "/NOLOGO", "/TLBID:1" ] )

    # !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    # !!! See SConscript file for linker flags that are specific to the EXE and not the DLLS !!!
    # !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    winLibString = "Dbghelp.lib psapi.lib ws2_32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib"
    winLibString += ""
    env.Append( LIBS=Split(winLibString) )

else:
    print( "No special config for [" + os.sys.platform + "] which probably means it won't work" )

env['STATIC_AND_SHARED_OBJECTS_ARE_THE_SAME'] = 1

env.Append( CPPPATH=['$EXTRACPPPATH'] )
env.Append( LIBPATH=['$EXTRALIBPATH'] )


# --- check system ---

def doConfigure(myenv):
    conf = Configure(myenv)

    if 'CheckCXX' in dir( conf ):
        if  not conf.CheckCXX():
            print( "c++ compiler test failed!" )
            print( "This sometimes happens even though the compiler is fine and can be resolved by performing a 'scons -c' followed by manually removing the .sconf_temp folder and .sconsign.dblite. It can also be because mpich_devel is not installed." )
            Exit(1)
            
    return conf.Finish()


def setEnvAttrs(myenv):

    diseasedlls = ['Generic', 'Vector', 'Malaria', 'STI', 'HIV' ]
    diseases = ['Generic', 'Vector', 'Malaria', 'STI', 'HIV' ]
    reportdlls = ['Spatial', 'Binned']
    campaigndlls = ['Bednet', 'IRSHousing']

    myenv['AllDlls'] = False
    dlldisease = has_option('DllDisease')
    dllreport = has_option('Report')
    #dllcampaign = has_option('Campaign')
    monodisease = has_option('Disease')

    #if has_option('Dlls'):
        #myenv['AllDlls'] = True

    myenv['AllInterventions'] = False
    #if has_option('Interventions'):
    #    myenv['AllInterventions'] = True

    #if has_option('Dlls') or dlldisease or dllreport or dllcampaign:
    if dlldisease or dllreport:
        myenv.Append( CPPDEFINES=["_DLLS_" ] )

    if dlldisease:
        myenv['DiseaseDll'] = get_option( 'DllDisease' ) # careful, tricky
        print( "DiseaseDll=" + myenv['DiseaseDll'] )
        if myenv['DiseaseDll'] not in diseasedlls:
            print( "Unknown disease (EMODule) type: " + myenv['DiseaseDll'] )
            exit(1)
    else:
        myenv['DiseaseDll'] = ""

    if monodisease:
        myenv['Disease'] = get_option( 'Disease' )
        print( "Disease=" + myenv['Disease'] )
        if myenv['Disease'] not in diseases:
            print( "Unknown disease type: " + myenv['Disease'] )
            exit(1)
    else:
        myenv['Disease'] = ""

    if dllreport:
        myenv['Report'] = get_option( 'Report' )
        print( "Report=" + myenv['Report'] )
        if myenv['Report'] not in reportdlls:
            print( "Unknown report type: " + myenv['Report'] )
            exit(1)
    else:
        myenv['Report'] = ""

    if has_option('Install'):
        myenv['Install'] = get_option( 'Install' )
    else:
        myenv['Install'] = ""

    print( "DLL=" + str(myenv['AllDlls']) )
    print( "Install=" + myenv['Install'] )

    if has_option( "TestSugar" ):
        print( "TestSugar ON, LOG_VALID enabled." )
        env.Append( CPPDEFINES= ["ENABLE_LOG_VALID"] )
    else:
        print( "TestSugar off, no LOG_VALID." )

# Main starting  point
env = doConfigure( env )

# set evn based on cmdline options
setEnvAttrs( env )

# Export the following symbols for them to be used in subordinate SConscript files.
Export("env")

# pass the build_dir as the variant directory
env.SConscript( 'SConscript', variant_dir='$BUILD_DIR', duplicate=False )

