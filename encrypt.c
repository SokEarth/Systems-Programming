/* Simple cci_f (modified Caesar cipher) implementation */
#include "Everything.h"
#include <io.h>

#define BUF_SIZE 65536	  /* Generally, you will get better performance with larger buffers (use powers of 2). 
					      /* 65536 worked well; larger numbers did not help in some simple tests. */

BOOL cci_f (LPCTSTR, LPCTSTR, DWORD);

int _tmain (int argc, LPTSTR argv [])
{
	if (argc != 4)
		ReportError  (_T ("Usage: cci shift file1 file2"), 1, FALSE);
	
	if (!cci_f (argv [2], argv [3], _ttoi(argv[1])))
		ReportError (_T ("Encryption failed."), 4, TRUE);

	return 0;
}

BOOL cci_f (LPCTSTR fIn, LPCTSTR fOut, DWORD shift)

/* Caesar cipher function  - Simple implementation
 *		fIn:		Source file pathname
 *		fOut:		Destination file pathname
 *		shift:		Numerical shift
 *	Behavior is modeled after CopyFile */
{
	HANDLE hIn, hOut;
	DWORD nIn, nOut, iCopy;
	BYTE buffer [BUF_SIZE], bShift = (BYTE)shift;
	BOOL writeOK = TRUE;

	hIn = CreateFile (fIn, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hIn == INVALID_HANDLE_VALUE) return FALSE;

	hOut = CreateFile (fOut, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hOut == INVALID_HANDLE_VALUE) {
		CloseHandle(hIn);
		return FALSE;
	}

	while (writeOK && ReadFile (hIn, buffer, BUF_SIZE, &nIn, NULL) && nIn > 0) {
		for (iCopy = 0; iCopy < nIn; iCopy++)
			buffer[iCopy] = buffer[iCopy] + bShift;
		writeOK = WriteFile (hOut, buffer, nIn, &nOut, NULL);
	}

	CloseHandle (hIn);
	CloseHandle (hOut);

	return writeOK;
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
