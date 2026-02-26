#include <windows.h>
#include <assert.h>

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
}

int main()
{
  int s;
  struct hostent * hp;
  struct sockaddr_in a;
  short port = 993;
  char * buffer = HELLO_MR_SERVER;
  int len = strlen(buffer) + 1;

  sock_init();
  /* make socket */
  if (INVALID_SOCKET == (s = socket(AF_INET, SOCK_STREAM, 0))) 
    sock_error();

  /* get server address */
  if (0 == (hp = gethostbyname("mail.design.t.u-tokyo.ac.jp")))
    sock_error();

  /* connect to server */
  a.sin_family = AF_INET;
  a.sin_addr.s_addr = *((unsigned long *)hp->h_addr);
  a.sin_port = htons(port);

  printf("connecting to server mail.design.t.u-tokyo.ac.jp:%d\n", port);
  if (SOCKET_ERROR == connect(s, (struct sockaddr *)&a, sizeof(a)))
    sock_error();
  printf( "Successfully connected\n" );
/*
  printf("sending\n");
  if (len != send(s, buffer, len, 0))
    sock_error();
*/
  printf("done\n");
}

