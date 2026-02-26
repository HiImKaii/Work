#include <windows.h>
#include <assert.h>

main()
{
  HANDLE h, m;
  DWORD n;
  int i;
  char * a;
  h = CreateFile("map.c", GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
  assert(h != INVALID_HANDLE_VALUE);
  n = GetFileSize(h, 0);
  m = CreateFileMapping(h, 0, PAGE_READONLY, 0, 0, 0);
  assert(m != INVALID_HANDLE_VALUE);
  a = (char *)MapViewOfFile(m, FILE_MAP_READ, 0, 0, 0);
  for (i = 0; i < n; i++) {
    putchar(a[i]);
  }
}
