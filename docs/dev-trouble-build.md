# Troubleshooting EMOD builds

If you encounter any of the following warnings or errors when attempting to build the EMOD executable (Eradication.exe) or
Eradication binary for Linux, see the information below to resolve the issue.

If you need assistance, you can contact support for help with solving issues. You can contact
Institute for Disease Modeling (IDM) support at idmsupport@gatesfoundation.org. When submitting the issue, please include any error
information.

## Unknown compiler version

If you encounter the warning "Unknown compiler version - please run the configure tests and report
the results" when attempting to build the Eradication.exe or Eradication binary for Linux, this indicates you are using a
version of Boost that is no longer supported. Install Boost 1.77.0.

## Inconsistent DLL linkage

If you see the following warning on some files, "c:python27includepymath.h(22): warning C4273:
'round': inconsistent dll linkage", this indicates that you are using a version of Python that is
no longer supported. Install Python 3.13 64-bit.

## Error 255

Check to see if you have any white spaces in the path to your local EMOD source code. If you do,
remove the white spaces in all of the directory names in the path.

## Error LNK4272

If you see the error "LNK4272 library machine type 'X86' conflicts with target machine type 'x64'",
this indicates that you need to uninstall 32-bit Python and reinstall 64-bit Python. Install
Python 3.13 64-bit.
