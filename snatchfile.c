/*-
 * Copyright (c) 2012 Ryan Kwolek
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice, this list of
 *     conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, this list
 *     of conditions and the following disclaimer in the documentation and/or other materials
 *     provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <shellapi.h>

#define ARRLEN(x) (sizeof(x) / sizeof((x)[0]))
#define STRBUFLEN 256
#define USAGE_STR "snatchfile [watch_dir] [-c copy_dir] [-e command_string] " \
	              "[-h] [-m match_string] [-o] [-s]"

int ParseCmdLine(int argc, wchar_t *argv[]);
int WildcardMatch(const WCHAR *pattern, const WCHAR *text);

WCHAR watchdir[MAX_PATH] = L"";
WCHAR copydir[MAX_PATH]  = L"";
WCHAR matchstr[256]      = L"";
WCHAR cmdstr[256]        = L"";
int failifexists = 0;
int watchsubtree = 0;


///////////////////////////////////////////////////////////////////////////////


int wmain(int argc, wchar_t *argv[]) {
	char buffer[sizeof(FILE_NOTIFY_INFORMATION) + MAX_PATH * sizeof(WCHAR)];
	WCHAR fullsrcpath[MAX_PATH], fulldestpath[MAX_PATH];
	PFILE_NOTIFY_INFORMATION pfni;
	unsigned long byteswritten;
	HANDLE hDir;

	if (ParseCmdLine(argc, argv))
		return 1;

	if (watchdir[0] == L'\0' && copydir[0] == L'\0' && cmdstr[0] == L'\0') {
		fprintf(stderr, "ERROR: Please specify either a directory to watch, "
			"a directory to copy to, or a command string to execute on match.\n");
		return 1;
	}
	
	hDir = CreateFile(watchdir, FILE_LIST_DIRECTORY,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL, OPEN_ALWAYS, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if (hDir == INVALID_HANDLE_VALUE) {
		fprintf(stderr, "ERROR: CreateFile() failed, err %d\n", GetLastError());
		return 1;
	}

	while (1) {
		if (!ReadDirectoryChangesW(hDir, buffer, sizeof(buffer), watchsubtree,
			FILE_NOTIFY_CHANGE_LAST_WRITE, &byteswritten, NULL, NULL)) {
			fprintf(stderr, "ERROR: ReadDirectoryChangesW() failed, err %d\n", GetLastError());
			return 1;
		}

		pfni = (PFILE_NOTIFY_INFORMATION)buffer;

		/*
		fnlen = WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, pfni->FileName,
			pfni->FileNameLength / sizeof(WCHAR), filename, sizeof(filename), NULL, NULL);
		if (!fnlen) {
			fprintf(stderr, "ERROR: WideCharToMultiByte() failed, err: %d\n", GetLastError());
			return 1;
		}
		
		filename[fnlen] = '\0';
		*/

		//we can do this without bounds checking because a FILE_NOTIFY_INFORMATION
		//structure already has 1 WCHAR element in FileName
		pfni->FileName[pfni->FileNameLength] = L'\0';

		if (matchstr[0] == L'\0' || WildcardMatch(matchstr, pfni->FileName)) {
			if (watchdir[0] != L'\0')
				wcscpy(fullsrcpath, watchdir);
			wcscat_s(fullsrcpath, ARRLEN(fullsrcpath), pfni->FileName);

			if (copydir[0] != L'\0')
				wcscpy(fulldestpath, copydir);
			wcscat_s(fulldestpath, ARRLEN(fulldestpath), pfni->FileName);

			if (cmdstr[0] == L'\0') {
				if (!CopyFile(fullsrcpath, fulldestpath, failifexists)) {
					fprintf(stderr, "ERROR: CopyFile() failed, err %d\n", GetLastError());
					return 1;
				}
				wprintf(L"Copied\n  %s\nto\n  %s\n\n", fullsrcpath, fulldestpath);
			} else {
				if ((int)ShellExecute(NULL, L"open", cmdstr, 
					fulldestpath, NULL, SW_SHOWDEFAULT) <= 32) {
					fprintf(stderr, "ERROR: ShellExecute() failed, err %d\n", GetLastError());
					return 1;
				}
				wprintf(L"Executed %s %s\n", cmdstr, fulldestpath);
			}
		}
	}

	return 0;
}


int ParseCmdLine(int argc, wchar_t *argv[]) {
	WCHAR *str;
	size_t len;
	int i;

	for (i = 1; i < argc; i++) {
		if (argv[i][0] != L'-') {
			if (watchdir[0] == L'\0') {
				str = watchdir; //Directory to watch
				goto case_cd_common;
			}
			continue;
		}
		switch (argv[i][1]) {
			case L'c': //directory to Copy files to
				str = copydir;
				i++;
				if (i == argc) {
					puts("option requires an argument -- c");
					puts(USAGE_STR);
					return 1;
				}
			case_cd_common:
				if (wcscpy_s(str, MAX_PATH - 1, argv[i])) {
					fprintf(stderr, "ERROR: parameter too big\n");
					return 1;
				}
				len = wcslen(str);
				if (len && str[len - 1] != L'\\') {
					str[len]     = L'\\';
					str[len + 1] = L'\0';
				}
				break;
			case L'h': //Help
				puts(USAGE_STR);
				return 0;
			case L'e': //Execute command on event
				str = cmdstr;
				goto case_em_common;
			case L'm': //Match files
				str = matchstr;
			case_em_common:
				i++;
				if (i == argc) {
					puts("option requires an argument -- c");
					puts(USAGE_STR);
					return 1;
				}
				if (wcscpy_s(str, STRBUFLEN, argv[i])) {
					fprintf(stderr, "ERROR: parameter too big\n");
					return 1;
				}
				break;
			case L'o': //don't Overwrite file if existing
				failifexists = 1;
				break;
			case L's': //watch Subdirectories too
				watchsubtree = 1;
				break;
			default:
				fprintf(stderr, "WARNING: ignoring unknown flag\n");
		}
	}
	return 0;
}


int WildcardMatch(const WCHAR *pattern, const WCHAR *text) {
	while (*pattern != L'\0') {
		if (*pattern == L'*') {
			text++;
			if (*text == pattern[1]) {
				if (WildcardMatch(pattern + 1, text))
					pattern++;
			}
		} else {
			if (*pattern != *text)
				return 0;
			pattern++;
			text++;
		}
	}
	return 1;
}

