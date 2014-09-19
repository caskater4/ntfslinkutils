///////////////////////////////////////////////////////////////////////////////
//
// This file is part of ntfslinkutils.
//
// Copyright (c) 2014, Jean-Philippe Steinmetz
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
// 
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// 
// * Neither the name of the {organization} nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include <Junction.h>
#include <memory.h>
#include <Symlink.h>
#include <strsafe.h>

#include "DataTypes.h"
#include "StringUtils.h"

using namespace libntfslinks;

mvlinkOptions Options;
mvlinkStats Stats;

/**
 * Prints a friendly message based on the given error code.
 */
void PrintErrorMessage(DWORD ErrorCode, LPCTSTR Path)
{
	switch (ErrorCode)
	{
	case ERROR_FILE_NOT_FOUND: printf("Error: File not found: %s.\n", Path); break;
	case ERROR_PATH_NOT_FOUND: printf("Error: Path not found: %s.\n", Path); break;
	case ERROR_ACCESS_DENIED: printf("Error: Access denied: %s.\n", Path); break;
	}
}

/**
 * Moves all reparse points in the specified source path to a given destination and rebases the target of each based on
 * the options set (when applicable).
 *
 * @param Src The path of the source file to move.
 * @param Dest The path of the destination to move Src to.
 * @param CurDepth The current level that has been traversed in the filesystem tree.
 * @return Returns zero if the operation was successful, otherwise a non-zero value on failure.
 */
DWORD mvlink(LPCTSTR Src, LPCTSTR Dest, int CurDepth = 0)
{
	DWORD result = 0;

	// If applicable, do not go further than the specified maximum depth
	if (Options.MaxDepth >= 0 && CurDepth > Options.MaxDepth)
	{
		return result;
	}

	// Expand the source to a full path
	TCHAR SrcPath[MAX_PATH] = {0};
	if (GetFullPathName(Src, MAX_PATH, SrcPath, NULL) == 0)
	{
		Stats.NumFailed++;
		printf("Invalid source path specified.\n");
		return 1;
	}
	
	// Expand the destination to a full path
	TCHAR DestPath[MAX_PATH] = {0};
	if (GetFullPathName(Dest, MAX_PATH, DestPath, NULL) == 0)
	{
		Stats.NumFailed++;
		printf("Invalid destination path specified.\n");
		return 1;
	}

	// Retrieve the file attributes of SrcPath
	WIN32_FILE_ATTRIBUTE_DATA srcAttributeData = {0};
	if (GetFileAttributesEx(SrcPath, GetFileExInfoStandard, &srcAttributeData))
	{
		// Reparse points must be processed first as they can also be considered a directory.
		if ((srcAttributeData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
		{
			// Check if the destination already exists
			WIN32_FILE_ATTRIBUTE_DATA destAttributeData = {0};
			if (GetFileAttributesEx(DestPath, GetFileExInfoStandard, &destAttributeData))
			{
				// Ask permission to delete the destination
				// TODO

				// Delete the existing reparse point destinations
				if ((destAttributeData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
				{
					if (IsJunction(DestPath))
					{
						result = DeleteJunction(DestPath);
					}
					else if (IsSymlink(DestPath))
					{
						result = DeleteSymlink(DestPath);
					}
				}
			}

			// Was there a failure deleting the existing destination?
			if (result == 0)
			{
				// Is this a junction or a symlink?
				if (IsJunction(SrcPath))
				{
					// Retrieve the existing target
					TCHAR Target[MAX_PATH] = {0};
					result = GetJunctionTarget(SrcPath, Target, sizeof(Target));
					if (result == 0)
					{
						// If specified, rebase the target to the new root
						if (Options.NewTargetBase[0] != 0 && Options.OldTargetBase[0] != 0)
						{
							TCHAR NewTarget[MAX_PATH] = {0};
							StrReplace(Target, Options.OldTargetBase, Options.NewTargetBase, NewTarget, -1, -1);

							// Create the junction at the destination
							result = CreateJunction(DestPath, NewTarget);
							if (result == 0 && Options.bVerbose)
							{
								printf("junction created for %s <<===>> %s\n", DestPath, NewTarget);
							}
						}
						// Otherwise create a junction to the existing target at the destination
						else
						{
							result = CreateJunction(DestPath, Target);
							if (result == 0 && Options.bVerbose)
							{
								printf("junction created for %s <<===>> %s\n", DestPath, Target);
							}
						}

						// Was the junction created successfully?
						if (result == 0)
						{
							Stats.NumMoved++;

							// Remove the original
							result = DeleteJunction(SrcPath);
						}
					}
				}
				else if (IsSymlink(SrcPath))
				{
					// Retrieve the existing target
					TCHAR Target[MAX_PATH] = {0};
					result = GetSymlinkTarget(SrcPath, Target, sizeof(Target));
					if (result == 0)
					{
						// If specified, rebase the target to the new root
						if (Options.NewTargetBase[0] != 0 && Options.OldTargetBase[0] != 0)
						{
							TCHAR NewTarget[MAX_PATH] = {0};
							StrReplace(Target, Options.OldTargetBase, Options.NewTargetBase, NewTarget, -1, -1);

							// Create the symlink at the destination
							result = CreateSymlink(DestPath, NewTarget);
							if (result == 0 && Options.bVerbose)
							{
								printf("symbolic link created for %s <<===>> %s\n", DestPath, NewTarget);
							}
						}
						// Otherwise create a symlink to the existing target at the destination
						else
						{
							result = CreateSymlink(DestPath, Target);
							if (result == 0 && Options.bVerbose)
							{
								printf("symbolic link created for %s <<===>> %s\n", DestPath, Target);
							}
						}

						// Was the symlink created successfully?
						if (result == 0)
						{
							Stats.NumMoved++;

							// Remove the original
							result = DeleteSymlink(SrcPath);
						}
					}
				}
				else
				{
					result = GetLastError();
					if (result != 0)
					{
						PrintErrorMessage(result, SrcPath);
					}
					else
					{
						printf("Unrecognized reparse point: %s\n", SrcPath);
						Stats.NumSkipped++;
					}
				}
			}
		}
		else if ((srcAttributeData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
		{
			// Make sure the the destination directory exists. If not create it.
			WIN32_FILE_ATTRIBUTE_DATA destAttributeData = {0};
			if (GetFileAttributesEx(DestPath, GetFileExInfoStandard, &destAttributeData))
			{
				if ((destAttributeData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
				{
					// TODO Copy security descriptor?
					if (!CreateDirectoryEx(SrcPath, DestPath, NULL))
					{
						// Failed to create the destination directory. Already exists?
						// TODO
					}
				}
			}

			WIN32_FIND_DATA ffd;
			TCHAR szDir[MAX_PATH] = {0};
			HANDLE hFind;

			// The search path must include '\*'
			StringCchCopy(szDir, sizeof(szDir), SrcPath);
			StringCchCat(szDir, sizeof(szDir), TEXT("\\*"));

			// Iterate through the list of files in the directory and mvlink each one. This will be done in two passes,
			// the first pass will move all regular files and directories. The second pass will move all reparse points.
			hFind = FindFirstFile(szDir, &ffd);
			if (hFind != INVALID_HANDLE_VALUE)
			{
				do
				{
					// Ignore the '.' and '..' entries
					size_t fileNameLength;
					if (FAILED(StringCchLength(ffd.cFileName, MAX_PATH, &fileNameLength)) || fileNameLength == 0 ||
						(fileNameLength == 1 && ffd.cFileName[0] == '.') ||
						(fileNameLength == 2 && ffd.cFileName[0] == '.' && ffd.cFileName[1] == '.'))
					{
						continue;
					}

					// Ignore anything that isn't a directory or reparse point
					if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 ||
						(ffd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
					{
						TCHAR srcFile[MAX_PATH];
						StringCchCopy(srcFile, sizeof(srcFile), SrcPath);
						StringCchCat(srcFile, sizeof(srcFile), TEXT("\\"));
						StringCchCat(srcFile, sizeof(srcFile), ffd.cFileName);

						TCHAR destFile[MAX_PATH];
						StringCchCopy(destFile, sizeof(destFile), DestPath);
						StringCchCat(destFile, sizeof(destFile), TEXT("\\"));
						StringCchCat(destFile, sizeof(destFile), ffd.cFileName);

						result = mvlink(srcFile, destFile, CurDepth+1);
					}
				} while (FindNextFile(hFind, &ffd) != 0);

				// Close the handle for the first pass
				FindClose(hFind);
			}
			else
			{
				// Failed to find the first file
				result = GetLastError();
			}
		}
	}
	else
	{
		result = GetLastError();
	}

	// Was the operation successful?
	if (result != 0)
	{
		Stats.NumFailed++;
		PrintErrorMessage(result, SrcPath);
	}

	return result;
}

void PrintUsage()
{
	printf("Moves all symbolic links and junctions from one path to another.\n\n");
	printf("Usage: mvlink [/V] [/LEV:n] [/R <find> <replace>] <source> <destination>\n\n");
	printf("Options:\n");
	printf("\t\t/LEV:n\t\tOnly move the top n levels of the source directory tree.\n");
	printf("\t\t/R <old> <new>\tModifies the target path of all links, replacing the last occurrence of <old> with <new>.\n");
	printf("\t\t/V\t\tEnable verbose output and display more information.\n");
	printf("\t\t/VER\t\tDisplay the version and copyright information.\n");
	printf("\t\t/?\t\tView this list of options.\n");
}

void PrintVersion()
{
	printf("Copyright (C) 2014, Jean-Philippe Steinmetz. All rights reserved.\n");
	printf("\n");
	printf("Redistribution and use in source and binary forms, with or without\n");
	printf("modification, are permitted provided that the following conditions are met:\n");
	printf("\n");
	printf("* Redistributions of source code must retain the above copyright notice, this\n");
	printf("  list of conditions and the following disclaimer.\n");
	printf("\n");
	printf("* Redistributions in binary form must reproduce the above copyright notice,\n");
	printf("  this list of conditions and the following disclaimer in the documentation\n");
	printf("  and/or other materials provided with the distribution.\n");
	printf("\n");
	printf("* Neither the name of the {organization} nor the names of its\n");
	printf("  contributors may be used to endorse or promote products derived from\n");
	printf("  this software without specific prior written permission.\n");
	printf("\n");
	printf("THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\"\n");
	printf("AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE\n");
	printf("IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE\n");
	printf("DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE\n");
	printf("FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL\n");
	printf("DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR\n");
	printf("SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER\n");
	printf("CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,\n");
	printf("OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE\n");
	printf("OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.");
}

int _tmain(int argc, TCHAR* argv[])
{
	DWORD result;
	int requiredArgs = 3;

	// Parse the command line arguments
	TCHAR Value[1024];
	for (int i = 1; i < argc; i++)
	{
		if (StrFind(argv[i], TEXT("/VER")) >= 0 || StrFind(argv[i], TEXT("/ver")) >= 0)
		{
			PrintVersion();
			return 0;
		}
		else if (StrFind(argv[i], TEXT("/?")) >= 0)
		{
			PrintUsage();
			return 0;
		}
		else if (StrFind(argv[i], TEXT("/LEV")) >= 0 || StrFind(argv[i], TEXT("/lev")) >= 0)
		{
			memset(Value, 0, sizeof(Value));
			StringCchCopy(Value, sizeof(Value), &argv[i][5]);
			Options.MaxDepth = _ttoi(Value);
		}
		else if (StrFind(argv[i], TEXT("/R")) >= 0 || StrFind(argv[i], TEXT("/r")) >= 0)
		{
			requiredArgs += 3;
			if (argc < requiredArgs || argv[i+1][0] == '/' || argv[i+2][0] == '/')
			{
				printf("Error: Invalid argument(s).\n");
				PrintUsage();
				return 1;
			}

			StringCchCopy(Options.OldTargetBase, sizeof(Options.OldTargetBase), argv[i+1]);
			StringCchCopy(Options.NewTargetBase, sizeof(Options.NewTargetBase), argv[i+2]);
		}
		else if (StrFind(argv[i], TEXT("/V")) >= 0 || StrFind(argv[i], TEXT("/v")) >= 0)
		{
			Options.bVerbose = true;
		}
	}

	// Check the minimum required arguments
	if (argc < requiredArgs)
	{
		printf("Error: Missing argument(s).\n");
		PrintUsage();
		return 1;
	}

	// Execute mvlink
	result = mvlink(argv[argc-2], argv[argc-1]);

	// Print the execution statistics
	printf("Moved: %d\n", Stats.NumMoved);
	printf("Skipped: %d\n", Stats.NumSkipped);
	printf("Failed: %d\n", Stats.NumFailed);

	return Stats.NumFailed > 0 ? (int)result : 0;
}
