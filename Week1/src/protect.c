#include <windows.h>
#include <assert.h>

int foo(LPEXCEPTION_POINTERS);

char a[16384];

main()
{
  DWORD old_protect;
  char * p = a + 4096;
  BOOL b = VirtualProtect(p, 1, PAGE_NOACCESS, &old_protect);
  assert(b);
  printf("protected %x\n", p);
  __try {
    *p = 0;
    printf("done\n");
  } __except(foo(GetExceptionInformation())) {
    ;
  }
}

int foo(LPEXCEPTION_POINTERS info) 
{
  PEXCEPTION_RECORD e = info->ExceptionRecord; 
  if (e->ExceptionCode == STATUS_ACCESS_VIOLATION) {
    void * i_addr = e->ExceptionAddress;
    DWORD rw_flag = e->ExceptionInformation[0];
    void * d_addr = (void * )e->ExceptionInformation[1];
    DWORD old_protect;
    BOOL b;
    printf("violation when accessing %x (instruction at %x)\n", 
	   d_addr, i_addr);
    b = VirtualProtect(d_addr, 1, PAGE_READWRITE, &old_protect);
    assert(b);
    return EXCEPTION_CONTINUE_EXECUTION;
  } else {
    return EXCEPTION_CONTINUE_SEARCH;
  }
}

#if 0
/* alternatively, we can do */
BOOL b = VirtualProtect(p, 1, PAGE_GUARD | PAGE_READWRITE, &old_protect);
/* and */
if (e->ExceptionCode == 0x80000001) {
  ...
}
#endif

