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

rmlinkOptions Options;
rmlinkStats Stats;

/**
 * Prints a friendly message based on the given error code.
 */
void PrintErrorMessage(DWORD ErrorCode)
{
	switch (ErrorCode)
	{
	case ERROR_FILE_NOT_FOUND: printf("Error: File not found.\n"); break;
	case ERROR_PATH_NOT_FOUND: printf("Error: Path not found.\n"); break;
	case ERROR_ACCESS_DENIED: printf("Error: Access denied.\n"); break;
	}
}

/**
 * Deletes all reparse points in the specified source path to a given destination.
 *
 * @param Src The path of the source file to delete.
 * @param CurDepth The current level that has been traversed in the filesystem tree.
 * @return Returns zero if the operation was successful, otherwise a non-zero value on failure.
 */
DWORD rmlink(LPCTSTR Path, int CurDepth = 0)
{
	DWORD result = 0;

	// If applicable, do not go further than the specified maximum depth
	if (Options.MaxDepth >= 0 && CurDepth > Options.MaxDepth)
	{
		return result;
	}

	// Retrieve the file attributes of Path
	WIN32_FILE_ATTRIBUTE_DATA pathAttributeData = {0};
	if (GetFileAttributesEx(Path, GetFileExInfoStandard, &pathAttributeData))
	{
		// Reparse points must be processed first as they can also be considered a directory.
		if ((pathAttributeData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
		{
			// Is this a junction or a symlink?
			if (IsJunction(Path))
			{
				// Delete the junction
				result = DeleteJunction(Path);
				if (result == 0)
				{
					Stats.NumDeleted++;
				}
			}
			else if (IsSymlink(Path))
			{
				// Delete the symlink
				result = DeleteSymlink(Path);
				if (result == 0)
				{
					Stats.NumDeleted++;
				}
			}
			else
			{
				printf("Unrecognized reparse point: %s", Path);
				Stats.NumSkipped++;
			}
		}
		else if ((pathAttributeData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
		{
			WIN32_FIND_DATA ffd;
			TCHAR szDir[MAX_PATH] = {0};
			HANDLE hFind;

			// The search path must include '\*'
			StringCchCopy(szDir, sizeof(szDir), Path);
			StringCchCat(szDir, sizeof(szDir), TEXT("\\*"));

			// Iterate through the list of files in the directory and rmlink each one. This will be done in two passes,
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
						TCHAR filePath[MAX_PATH];
						StringCchCopy(filePath, sizeof(filePath), Path);
						StringCchCat(filePath, sizeof(filePath), TEXT("\\"));
						StringCchCat(filePath, sizeof(filePath), ffd.cFileName);

						result = rmlink(filePath, CurDepth+1);
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
		PrintErrorMessage(result);
	}

	return result;
}

void PrintUsage()
{
	printf("Deletes all symbolic links and junctions from the specified list of paths.\n\n");
	printf("Usage: rmlink [/V] [/LEV:n] <path>...\n\n");
	printf("Options:\n");
	printf("\t\t/LEV:n\t\tOnly remove links in the top n levels of the path.\n");
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
	int requiredArgs = 2;

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

	// Iterate through each argument that isn't an option and execute rmlink on it
	for (int i = 1; i < argc; i++)
	{
		// Ignore options
		if (argv[i][0] == '/')
		{
			continue;
		}

		result = rmlink(argv[i]);
		if (result != 0)
		{
			// Exit on failure
			break;
		}
	}

	// Print the execution statistics
	if (Options.bVerbose)
	{
		printf("Deleted: %d\n", Stats.NumDeleted);
		printf("Skipped: %d\n", Stats.NumSkipped);
		printf("Failed: %d\n", Stats.NumFailed);
	}

	return Stats.NumFailed > 0 ? (int)result : 0;
}
