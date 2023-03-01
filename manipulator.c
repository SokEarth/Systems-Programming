/* Chapter 3. Windows lsW file list command. */
/* lsW[options][files]
	List the attributes of one or more files.
	Options:
		-R	recursive
		-l	longList listing(size and time of modification)
			Depending on the ProcessItem function, this will
			also list the owner and permissions(See Chapter 5 - security). */

/* This program illustrates:
		1.	Search handles and directory traversal
		2.	File attributes, including time
		3.	Using generic string functions to output file information */
/* THIS PROGRAM IS NOT A FAITHFUL IMPLEMENATION OF THE POSIX ls COMMAND - IT IS INTENDED TO ILLUSRATE THE WINDOWS API */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* BEGIN BOILERPLATE CODE */

#include "INCLUDE\Everything.h"

BOOL TraverseDirectory(LPTSTR, LPTSTR, DWORD, LPBOOL);
DWORD FileType(LPWIN32_FIND_DATA);
BOOL ProcessItem(LPWIN32_FIND_DATA, DWORD, LPBOOL);
DWORD Options (int argc, LPCTSTR argv [], LPCTSTR OptStr, ...);
BOOL PrintStrings (HANDLE hOut, ...);
BOOL PrintMsg (HANDLE hOut, LPCTSTR pMsg);
BOOL ConsolePrompt (LPCTSTR pPromptMsg, LPTSTR pResponse, DWORD maxChar, BOOL echo);

int _tmain(int argc, LPTSTR argv[])
{
	BOOL flags[MAX_OPTIONS], ok = TRUE;
	TCHAR searchPattern[MAX_PATH + 1], currPath[MAX_PATH_LONG+1], parentPath[MAX_PATH_LONG+1];
	LPTSTR pSlash, pSearchPattern;
	int i, fileIndex;
	DWORD pathLength;

	fileIndex = Options(argc, argv, _T("Rl"), &flags[0], &flags[1], NULL);

	/* "Parse" the search pattern into two parts: the "parent"
		and the file name or wild card expression. The file name
		is the longest suffix not containing a slash.
		The parent is the remaining prefix with the slash.
		This is performed for all command line search patterns.
		If no file is specified, use * as the search pattern. */

	pathLength = GetCurrentDirectory(MAX_PATH_LONG, currPath); 
	if (pathLength == 0 || pathLength >= MAX_PATH_LONG) { /* pathLength >= MAX_PATH_LONG (32780) should be impossible */
		ReportError(_T("GetCurrentDirectory failed"), 1, TRUE);
	}

	if (argc < fileIndex + 1) 
		ok = TraverseDirectory(currPath, _T("*"), MAX_OPTIONS, flags);
	else for (i = fileIndex; i < argc; i++) {
		if (_tcslen(argv[i]) >= MAX_PATH) {
			ReportError(_T("The command line argument is longer than the maximum this program supports"), 2, FALSE);
		}
		_tcscpy(searchPattern, argv[i]);
		_tcscpy(parentPath, argv[i]);

		/* Find the rightmost slash, if any.
			Set the path and use the rest as the search pattern. */
		pSlash = _tstrrchr(parentPath, _T('\\')); 
		if (pSlash != NULL) {
			*pSlash = _T('\0');
			_tcscat(parentPath, _T("\\"));         
			SetCurrentDirectory(parentPath);
			pSlash = _tstrrchr(searchPattern, _T('\\'));  
			pSearchPattern = pSlash + 1;
		} else {
			_tcscpy(parentPath, _T(".\\"));
			pSearchPattern = searchPattern;
		}
		ok = TraverseDirectory(parentPath, pSearchPattern, MAX_OPTIONS, flags) && ok;
		SetCurrentDirectory(currPath);	 /* Restore working directory. */
	}

	return ok ? 0 : 1;
}

static BOOL TraverseDirectory(LPTSTR parentPath, LPTSTR searchPattern, DWORD numFlags, LPBOOL flags)

/* Traverse a directory, carrying out an implementation specific "action" for every
	name encountered. The action in this version is "list, with optional attributes". */
/* searchPattern: Relative or absolute searchPattern to traverse in the parentPath.  */
/* On entry, the current direcotry is parentPath, which ends in a \ */
{
	HANDLE searchHandle;
	WIN32_FIND_DATA findData;
	BOOL recursive = flags[0];
	DWORD fType, iPass, lenParentPath;
	TCHAR subdirectoryPath[MAX_PATH + 1];

	/* Open up the directory search handle and get the
		first file name to satisfy the path name.
		Make two passes. The first processes the files
		and the second processes the directories. */

	if ( _tcslen(searchPattern) == 0 ) {
		_tcscat(searchPattern, _T("*"));
	}
	/* Add a backslash, if needed, at the end of the parent path */
	if (parentPath[_tcslen(parentPath)-1] != _T('\\') ) { /* Add a \ to the end of the parent path, unless there already is one */
		_tcscat (parentPath, _T("\\"));
	}


	/* Open up the directory search handle and get the
		first file name to satisfy the path name. Make two passes.
		The first processes the files and the second processes the directories. */

	for (iPass = 1; iPass <= 2; iPass++) {
		searchHandle = FindFirstFile(searchPattern, &findData);
		if (searchHandle == INVALID_HANDLE_VALUE) {
			ReportError(_T("Error opening Search Handle."), 0, TRUE);
			return FALSE;
		}

		/* Scan the directory and its subdirectories for files satisfying the pattern. */
		do {

		/* For each file located, get the type. List everything on pass 1.
			On pass 2, display the directory name and recursively process
			the subdirectory contents, if the recursive option is set. */
			fType = FileType(&findData);
			if (iPass == 1) /* ProcessItem is "print attributes". */
				ProcessItem(&findData, MAX_OPTIONS, flags);

			lenParentPath = (DWORD)_tcslen(parentPath);
			/* Traverse the subdirectory on the second pass. */
			if (fType == TYPE_DIR && iPass == 2 && recursive) {
				_tprintf(_T("\n%s%s:"), parentPath, findData.cFileName);
				SetCurrentDirectory(findData.cFileName);
				if (_tcslen(parentPath) + _tcslen(findData.cFileName) >= MAX_PATH_LONG-1) {
					ReportError(_T("Path Name is too long"), 10, FALSE);
				}
				_tcscpy(subdirectoryPath, parentPath);
				_tcscat (subdirectoryPath, findData.cFileName); /* The parent path terminates with \ before the _tcscat call */
				TraverseDirectory(subdirectoryPath, _T("*"), numFlags, flags);
				SetCurrentDirectory(_T("..")); /* Restore the current directory */
			}

			/* Get the next file or directory name. */

		} while (FindNextFile(searchHandle, &findData));

		FindClose(searchHandle);
	}
	return TRUE;
}

static DWORD FileType(LPWIN32_FIND_DATA pFileData)

/* Return file type from the find data structure.
	Types supported:
		TYPE_FILE:	If this is a file
		TYPE_DIR:	If this is a directory other than . or ..
		TYPE_DOT:	If this is . or .. directory */
{
	BOOL isDir;
	DWORD fType;
	fType = TYPE_FILE;
	isDir =(pFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
	if (isDir)
		if (lstrcmp(pFileData->cFileName, _T(".")) == 0
				|| lstrcmp(pFileData->cFileName, _T("..")) == 0)
			fType = TYPE_DOT;
		else fType = TYPE_DIR;
	return fType;
}
/*  END OF BOILERPLATE CODE */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static BOOL ProcessItem(LPWIN32_FIND_DATA pFileData, DWORD numFlags, LPBOOL flags)

/* Function to process(list attributes, in this case)
	the file or directory. This implementation only shows
	the low order part of the file size. */
{
	const TCHAR fileTypeChar[] = {_T(' '), _T('d')};
	DWORD fType = FileType(pFileData);
	BOOL longList = flags[1];
	SYSTEMTIME lastWrite;

	if (fType != TYPE_FILE && fType != TYPE_DIR) return FALSE;

	_tprintf(_T("\n"));
	if (longList) {
		_tprintf(_T("%c"), fileTypeChar[fType - 1]);
		_tprintf(_T("%10d"), pFileData->nFileSizeLow);
		FileTimeToSystemTime(&(pFileData->ftLastWriteTime), &lastWrite);
		_tprintf(_T("	%02d/%02d/%04d %02d:%02d:%02d"),
				lastWrite.wMonth, lastWrite.wDay,
				lastWrite.wYear, lastWrite.wHour,
				lastWrite.wMinute, lastWrite.wSecond);
	}
	_tprintf(_T(" %s"), pFileData->cFileName);
	return TRUE;
}

DWORD Options (int argc, LPCTSTR argv [], LPCTSTR OptStr, ...)

/* argv is the command line.
	The options, if any, start with a '-' in argv[1], argv[2], ...
	OptStr is a text string containing all possible options,
	in one-to-one correspondence with the addresses of Boolean variables
	in the variable argument list (...).
	These flags are set if and only if the corresponding option
	character occurs in argv [1], argv [2], ...
	The return value is the argv index of the first argument beyond the options. */

{
	va_list pFlagList;
	LPBOOL pFlag;
	int iFlag = 0, iArg;

	va_start (pFlagList, OptStr);

	while ((pFlag = va_arg (pFlagList, LPBOOL)) != NULL
				&& iFlag < (int)_tcslen (OptStr)) {
		*pFlag = FALSE;
		for (iArg = 1; !(*pFlag) && iArg < argc && argv [iArg] [0] == _T('-'); iArg++)
			*pFlag = _memtchr (argv [iArg], OptStr [iFlag],
					_tcslen (argv [iArg])) != NULL;
		iFlag++;
	}

	va_end (pFlagList);

	for (iArg = 1; iArg < argc && argv [iArg] [0] == _T('-'); iArg++);

	return iArg;
}

BOOL PrintStrings (HANDLE hOut, ...)

/* Write the messages to the output handle. Frequently hOut
	will be standard out or error, but this is not required.
	Use WriteConsole (to handle Unicode) first, as the
	output will normally be the console. If that fails, use WriteFile.

	hOut:	Handle for output file. 
	... :	Variable argument list containing TCHAR strings.
		The list must be terminated with NULL. */
{
	DWORD msgLen, count;
	LPCTSTR pMsg;
	va_list pMsgList;	/* Current message string. */
	va_start (pMsgList, hOut);	/* Start processing msgs. */
	while ((pMsg = va_arg (pMsgList, LPCTSTR)) != NULL) {
		msgLen = lstrlen (pMsg);
		if (!WriteConsole (hOut, pMsg, msgLen, &count, NULL)
				&& !WriteFile (hOut, pMsg, msgLen * sizeof (TCHAR),	&count, NULL)) {
			va_end (pMsgList);
			return FALSE;
		}
	}
	va_end (pMsgList);
	return TRUE;
}


BOOL PrintMsg (HANDLE hOut, LPCTSTR pMsg)

/* For convenience only - Single message version of PrintStrings so that
	you do not have to remember the NULL arg list terminator.

	hOut:	Handle of output file
	pMsg:	Single message to output. */
{
	return PrintStrings (hOut, pMsg, NULL);
}


BOOL ConsolePrompt (LPCTSTR pPromptMsg, LPTSTR pResponse, DWORD maxChar, BOOL echo)

/* Prompt the user at the console and get a response
	which can be up to maxChar generic characters.

	pPromptMessage:	Message displayed to user.
	pResponse:	Programmer-supplied buffer that receives the response.
	maxChar:	Maximum size of the user buffer, measured as generic characters.
	echo:		Do not display the user's response if this flag is FALSE. */
{
	HANDLE hIn, hOut;
	DWORD charIn, echoFlag;
	BOOL success;
	hIn = CreateFile (_T("CONIN$"), GENERIC_READ | GENERIC_WRITE, 0,
			NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	hOut = CreateFile (_T("CONOUT$"), GENERIC_WRITE, 0,
			NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	/* Should the input be echoed? */
	echoFlag = echo ? ENABLE_ECHO_INPUT : 0;

	/* API "and" chain. If any test or system call fails, the
		rest of the expression is not evaluated, and the
		subsequent functions are not called. GetStdError ()
		will return the result of the failed call. */

	success =  SetConsoleMode (hIn, ENABLE_LINE_INPUT | echoFlag | ENABLE_PROCESSED_INPUT)
			&& SetConsoleMode (hOut, ENABLE_WRAP_AT_EOL_OUTPUT | ENABLE_PROCESSED_OUTPUT)
			&& PrintStrings (hOut, pPromptMsg, NULL)
			&& ReadConsole (hIn, pResponse, maxChar - 2, &charIn, NULL);
	
	/* Replace the CR-LF by the null character. */
	if (success)
		pResponse [charIn - 2] = _T('\0');
	else
		ReportError (_T("ConsolePrompt failure."), 0, TRUE);

	CloseHandle (hIn);
	CloseHandle (hOut);
	return success;
}

VOID ReportError (LPCTSTR userMessage, DWORD exitCode, BOOL printErrorMessage)

/* General-purpose function for reporting system errors.
	Obtain the error number and convert it to the system error message.
	Display this information and the user-specified message to the standard error device.
	userMessage:		Message to be displayed to standard error device.
	exitCode:		0 - Return.
					> 0 - ExitProcess with this code.
	printErrorMessage:	Display the last system error message if this flag is set. */
{
	DWORD eMsgLen, errNum = GetLastError ();
	LPTSTR lpvSysMsg;
	_ftprintf (stderr, _T("%s\n"), userMessage);
	if (printErrorMessage) {
		eMsgLen = FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				NULL, errNum, MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPTSTR) &lpvSysMsg, 0, NULL);
		if (eMsgLen > 0)
		{
			_ftprintf (stderr, _T("%s\n"), lpvSysMsg);
		}
		else
		{
			_ftprintf (stderr, _T("Last Error Number; %d.\n"), errNum);
		}

		if (lpvSysMsg != NULL) LocalFree (lpvSysMsg); /* Explained in Chapter 5. */
	}

	if (exitCode > 0)
		ExitProcess (exitCode);

	return;
}

/* Extension of ReportError to generate a user exception
	code rather than terminating the process. */

VOID ReportException (LPCTSTR userMessage, DWORD exceptionCode)
			
/* Report as a non-fatal error.
	Print the system error message only if the message is non-null. */
{	
	if (lstrlen (userMessage) > 0)
		ReportError (userMessage, 0, TRUE);
			/* If fatal, raise an exception. */

	if (exceptionCode != 0) 
		RaiseException (
			(0x0FFFFFFF & exceptionCode) | 0xE0000000, 0, 0, NULL);
	
	return;
}
