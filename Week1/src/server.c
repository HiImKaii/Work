#include <windows.h>
#include <assert.h>
//#include <Winsock2.h>


#define HELLO_MR_SERVER "hello, Mr. server"

void sock_error()
{
  int e = WSAGetLastError();
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
  printf("ERROR: %s\n", lpMsgBuf);
  exit(1);
}

void sock_init()
{
  WORD wVersionRequested = MAKEWORD( 2, 2 );
  WSADATA wsaData[1];
  /* initialize winsock */
  int r = WSAStartup(wVersionRequested, wsaData);
  if (r == -1) sock_error();
  return;
}

int main()
{
  SOCKET s, new_s;
  int l;
  struct sockaddr_in a;
  sock_init();
  /* make socket */
  if (INVALID_SOCKET == (s = socket(AF_INET, SOCK_STREAM, 0)))
    sock_error();

  /* assign port */
  a.sin_family = AF_INET;
  a.sin_addr.s_addr = INADDR_ANY;
  a.sin_port = htons(5000);
  if (SOCKET_ERROR == bind(s, (struct sockaddr *)&a, sizeof(a)))
    sock_error();

  /* specify queue length */
  if (SOCKET_ERROR == listen(s, 1)) 
    sock_error();

  l = sizeof(a);
  if (SOCKET_ERROR == getsockname(s,  (struct sockaddr *)&a, &l)) 
    sock_error();
  
  printf("accepting connection at port %d...\n", ntohs(a.sin_port));
  l = sizeof(a);
  if (SOCKET_ERROR == (new_s = accept(s, (struct sockaddr *)&a, &l))) 
    sock_error();

  {
    /* now ready to receive stuff */
    char buffer[100];
    int expect_len = strlen(HELLO_MR_SERVER) + 1;
    printf("receiving\n");
    if (expect_len != recv(new_s, buffer, expect_len, 0))
      sock_error();
    printf("received: %s\n", buffer);
    printf("done\n");
  }
  return 0;
}

