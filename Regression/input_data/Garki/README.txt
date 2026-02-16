Garki has 4 weather files that would require LFS.  If we zip them,
they do not.  Hence, after clone the repo, you need to get the files
out of the zip file so that the tests can run.

These large weather files should be specified in the .gitignore.