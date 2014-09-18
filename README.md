ntfslinkutils
===========

A collection of Windows command line utilities for manipulating NTFS reparse
points (junctions, symbolic links) in a directory tree. These utilities were
written to address general shortcomings with the built-in tools provided
by Microsoft (namely mklink and robocopy).

The following Windows versions are supported:

- Windows Vista 32/64-bit	[UNTESTED]
- Windows 7 32/64-bit		[UNTESTED]
- Windows 8 32/64-bit		[UNTESTED]
+ Windows 8.1 32/64-bit		[VERIFIED]

Pre-built binaries are available in the bin directory.

#cplink

The cplink utility can copy all reparse points in a given directory path to
another. The utility can also rewrite the all or part of the target for each
reparse point.
```
Usage: cplink [/V] [/LEV:n] [/R <find> <replace>] <source> <destination>

Options:
                /LEV:n          Only copy the top n levels of the source
								directory tree.
                /R <old> <new>  Modifies the target path of all links,
								replacing the last occurrence of <old> with
								<new>.
                /V              Enable verbose output and display more
								information.
                /VER            Display the version and copyright information.
                /?              View this list of options.
```
#mvlink

The mvlink utility moves all reparse points in a given directory path to
another. The utility also is capable of rewriting all or part of the target
for each reparse point.
```
Usage: mvlink [/V] [/LEV:n] [/R <find> <replace>] <source> <destination>

Options:
                /LEV:n          Only move the top n levels of the source
								directory tree.
                /R <old> <new>  Modifies the target path of all links,
								replacing the last occurrence of <old> with
								<new>.
                /V              Enable verbose output and display more
								information.
                /VER            Display the version and copyright information.
                /?              View this list of options.
```
#rmlink

The rmlink utility removes all reparse points from the specified list of paths.
```
Usage: rmlink [/V] [/LEV:n] <path>...

Options:
                /LEV:n          Only remove links in the top n levels of the
								path.
                /V              Enable verbose output and display more
								information.
                /VER            Display the version and copyright information.
                /?              View this list of options.
```
#How to Build

The solution files for this project were created for Visual Studio 2012. Any
version after 2012 should work but has not been tested.

1. Open ntfslinkutils.sln in Visual Studio.

2. Select the desired platform and configuration (e.g. Release|x64)

3. Build the solution (Build->Build Solution)

Once successfully built all of the utilities will be available in the
ntfslinkutils\bin directory.