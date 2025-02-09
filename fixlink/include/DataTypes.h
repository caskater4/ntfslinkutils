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

#ifndef DATATYPES_H
#define DATATYPES_H
#pragma once

#include <memory.h>

struct fixlinkOptions
{
	/** Set to true to enable verbose logging. */
	bool bVerbose;
	/** The maximum file tree depth to traverse before stopping. */
	int MaxDepth;
	/** The path to rebase targets to. */
	TCHAR NewTargetBase[MAX_PATH];
	/** The path to rebase targets from. */
	TCHAR OldTargetBase[MAX_PATH];

	fixlinkOptions()
		: bVerbose(false)
		, MaxDepth(-1)
	{
		memset(NewTargetBase, 0, sizeof(NewTargetBase));
		memset(OldTargetBase, 0, sizeof(OldTargetBase));
	}
};

struct fixlinkStats
{
	/** The number of file objects that failed to be moved. */
	size_t NumFailed;
	/** The number of file objects successfully modified. */
	size_t NumModified;
	/** The number of file objects that were skipped. */
	size_t NumSkipped;

	fixlinkStats()
		: NumFailed(0)
		, NumModified(0)
		, NumSkipped(0)
	{
	}
};

#endif //DATATYPES_H