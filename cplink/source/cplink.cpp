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

cplinkOptions Options;
cplinkStats Stats;

/**
 * Prints a friendly message based on the given error code.
 */
void PrintErrorMessage(DWORD ErrorCode, LPCTSTR Path)
{
	switch (ErrorCode)
	{
	case ERROR_FILE_NOT_FOUND: _tprintf(TEXT("File not found: %s.\n"), Path); break;
	case ERROR_PATH_NOT_FOUND: _tprintf(TEXT("Path not found: %s.\n"), Path); break;
	case ERROR_ACCESS_DENIED: _tprintf(TEXT("Access denied: %s.\n"), Path); break;
	}
}

/**
 * Copies all reparse points in the specified source path to a given destination and rebases the target of each based on
 * the options set (when applicable).
 *
 * @param Src The path of the source file to copy.
 * @param Dest The path of the destination to copy Src to.
 * @param CurDepth The current level that has been traversed in the filesystem tree.
 * @return Returns zero if the operation was successful, otherwise a non-zero value on failure.
 */
DWORD cplink(LPCTSTR Src, LPCTSTR Dest, int CurDepth = 0)
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
		_tprintf(TEXT("Invalid source path specified.\n"));
		return 1;
	}
	
	// Expand the destination to a full path
	TCHAR DestPath[MAX_PATH] = {0};
	if (GetFullPathName(Dest, MAX_PATH, DestPath, NULL) == 0)
	{
		Stats.NumFailed++;
		_tprintf(TEXT("Invalid destination path specified.\n"));
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
								_tprintf(TEXT("junction created for %s <<===>> %s\n"), DestPath, NewTarget);
							}
						}
						// Otherwise create a junction to the existing target at the destination
						else
						{
							result = CreateJunction(DestPath, Target);
							if (result == 0 && Options.bVerbose)
							{
								_tprintf(TEXT("junction created for %s <<===>> %s\n"), DestPath, Target);
							}
						}

						// Was the junction created successfully?
						if (result == 0)
						{
							Stats.NumCopied++;
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
								_tprintf(TEXT("symbolic link created for %s <<===>> %s\n"), DestPath, NewTarget);
							}
						}
						// Otherwise create a symlink to the existing target at the destination
						else
						{
							result = CreateSymlink(DestPath, Target);
							if (result == 0 && Options.bVerbose)
							{
								_tprintf(TEXT("symbolic link created for %s <<===>> %s\n"), DestPath, Target);
							}
						}

						// Was the symlink created successfully?
						if (result == 0)
						{
							Stats.NumCopied++;
						}
					}
				}
				else
				{
					result = GetLastError();
					if (result == 0)
					{
						_tprintf(TEXT("Unrecognized reparse point: %s\n"), SrcPath);
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

			// Iterate through the list of files in the directory and cplink each one. This will be done in two passes,
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

						result = cplink(srcFile, destFile, CurDepth+1);
					}
				} while (FindNextFile(hFind, &ffd) != 0);

				// Close the handle for the first pass
				FindClose(hFind);
			}
			else
			{
				// If we failed to be able to read the directory listing due to a access violation count it as a skip
				// instead of a complete failure.
				if (GetLastError() == ERROR_ACCESS_DENIED)
				{
					PrintErrorMessage(GetLastError(), SrcPath);
					Stats.NumSkipped++;
				}
				else
				{
					result = GetLastError();
				}
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
	_tprintf(TEXT("Copies all symbolic links and junctions from one path to another.\n\n"));
	_tprintf(TEXT("Usage: cplink [/V] [/LEV:n] [/R <find> <replace>] <source> <destination>\n\n"));
	_tprintf(TEXT("Options:\n"));
	_tprintf(TEXT("\t\t/LEV:n\t\tOnly copy the top n levels of the source directory tree.\n"));
	_tprintf(TEXT("\t\t/R <old> <new>\tModifies the target path of all links, replacing the last occurrence of <old> with <new>.\n"));
	_tprintf(TEXT("\t\t/V\t\tEnable verbose output and display more information.\n"));
	_tprintf(TEXT("\t\t/VER\t\tDisplay the version and copyright information.\n"));
	_tprintf(TEXT("\t\t/?\t\tView this list of options.\n"));
}

void PrintVersion()
{
	_tprintf(TEXT("Copyright (C) 2014, Jean-Philippe Steinmetz. All rights reserved.\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("Redistribution and use in source and binary forms, with or without\n"));
	_tprintf(TEXT("modification, are permitted provided that the following conditions are met:\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("* Redistributions of source code must retain the above copyright notice, this\n"));
	_tprintf(TEXT("  list of conditions and the following disclaimer.\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("* Redistributions in binary form must reproduce the above copyright notice,\n"));
	_tprintf(TEXT("  this list of conditions and the following disclaimer in the documentation\n"));
	_tprintf(TEXT("  and/or other materials provided with the distribution.\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\"\n"));
	_tprintf(TEXT("AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE\n"));
	_tprintf(TEXT("IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE\n"));
	_tprintf(TEXT("DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE\n"));
	_tprintf(TEXT("FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL\n"));
	_tprintf(TEXT("DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR\n"));
	_tprintf(TEXT("SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER\n"));
	_tprintf(TEXT("CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,\n"));
	_tprintf(TEXT("OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE\n"));
	_tprintf(TEXT("OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n"));
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
				_tprintf(TEXT("Error: Invalid argument(s).\n"));
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
		_tprintf(TEXT("Error: Missing argument(s).\n"));
		PrintUsage();
		return 1;
	}

	// Execute cplink
	result = cplink(argv[argc-2], argv[argc-1]);

	// Print the execution statistics
	_tprintf(TEXT("Copied: %d\n"), Stats.NumCopied);
	_tprintf(TEXT("Skipped: %d\n"), Stats.NumSkipped);
	_tprintf(TEXT("Failed: %d\n"), Stats.NumFailed);

	// Make sure that if there were errors it is reflected in the result
	if (result == 0 && Stats.NumFailed > 0)
	{
		result = 1;
	}

	return result;
}
