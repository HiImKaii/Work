#include <windows.h>
#include <stdio.h>
#include <conio.h>

static void win_error()
{
  int e = GetLastError();
  char * lpMsgBuf;
  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                FORMAT_MESSAGE_FROM_SYSTEM | 
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                e,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR) &lpMsgBuf,
                0,
                NULL 
                );
  fprintf(stderr, "%s", lpMsgBuf); 
  fflush(stderr);
}



main()
{
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  BOOL b;

  memset(&si, 0, sizeof(si));
  si.cb = sizeof(si);
  b = CreateProcess( "google.exe", 
		    "google.exe", 
		    0, 0, FALSE, 0, 0, 0, &si, &pi );

  if (b) {
    WaitForSingleObject( pi.hProcess, INFINITE );
  } else {
    win_error();
  }
  printf( "Press any key to continue..." );
  getch();
}

