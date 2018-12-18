#include "pch.h"
#pragma comment(lib, "ws2_32")
#pragma warning(disable:4996)
#include <WinSock2.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define SERVERPORT 9000

// 소켓 함수 오류 출력 후 종료
void err_quit(const char *);

// 소켓 함수 오류 출력
void err_display(const char *);

// 사용자 정의 데이터 수신 함수
int recvn(SOCKET, char *, int, int);

// 시간 출력 함수
void getISOTime(char *, size_t);

// 파일 기본 정보
typedef struct Files
{
	char name[255];
	unsigned int byte;
}Files;

DWORD WINAPI ProcessClient(LPVOID arg) {

	SOCKET client_sock = (SOCKET)arg;
	int retval;
	SOCKADDR_IN clientaddr;
	int addrlen;
	int recv_buffer = 65536 * 100;
	char *buf = new char[recv_buffer + 1];
	unsigned int count, per;
	
	// 타임스탬프 확인
	char timeBuffer[80];
	clock_t start, finish;
	double duration = 0.0;
	int minute, second;

	addrlen = sizeof(clientaddr);

	getpeername(client_sock, (SOCKADDR*)&clientaddr, &addrlen);
	while (1) {
		// 클라이언트로부터 파일 기본 정보 받기
		FILE *fp = NULL;
		Files files;

		retval = recvn(client_sock, (char *)&files, sizeof(files), 0);
		if (retval == SOCKET_ERROR)
		{
			err_display("recv()");
			exit(1);
		}

		// 기존 파일 여부 확인
		fp = fopen(files.name, "rb");
		if (fp == NULL)
			printf("같은 파일 이름이 없으므로 전송을 진행합니다.\n");
		else
		{
			system("cls");
			printf("이미 같은 이름의 파일이 존재 합니다.\n");
			printf("전송을 종료합니다.\n");
			fclose(fp);
			break;
		}

		printf("파일을 전송 받습니다.\n");
		getISOTime(timeBuffer, sizeof(timeBuffer));
		printf("받는 시각: %s\n", timeBuffer);
		printf("전송빋는 파일: %s, 전송받는 파일 크기: %d Byte\n", files.name, files.byte);
		printf("\n클라이언트로 부터 파일을 전송 받는 중 입니다.\n");

		fp = fopen(files.name, "wb");

		count = per = files.byte / recv_buffer;

		start = clock();	// 파일 전송 시작 시간을 start에 저장.

		while (count)
		{
			// 받기
			retval = recvn(client_sock, buf, recv_buffer, 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("recv()");
				exit(1);
			}
			
			if (((per - count) * 100 / per) % 10 == 0)
				printf(".");
			
			fwrite(buf, 1, recv_buffer, fp);

			count--;
		}

		// 남은 파일 크기만큼 나머지 받기
		count = files.byte - ((files.byte / recv_buffer) * recv_buffer);

		retval = recvn(client_sock, buf, recv_buffer, 0);
		if (retval == SOCKET_ERROR)
		{
			err_display("recv()");
			exit(1);
		}

		fwrite(buf, 1, count, fp);

		//파일 포인터 닫기
		fclose(fp);

		finish = clock();	// 파일 전송 완료 시간을 finish에 저장.

		printf("\n파일 전송이 완료 되었습니다.\n");
		getISOTime(timeBuffer, sizeof(timeBuffer));
		printf("완료 시각: %s\n", timeBuffer);

		duration = (double)(finish - start) / CLOCKS_PER_SEC;
		minute = duration / 60;
		second = duration - minute * 60;

		printf("총 걸린 시간: %d분 %d초\n", minute, second);

		closesocket(client_sock);

		return 0;
	}
}

int main(int argc, char* argv[])
{
	// send, recv 함수 출력값 저장용
	int retval;

	// (파일 크기 / 버퍼 사이즈) 계산한 값을 while문으로 돌리기 위한 변수
	unsigned int count;

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// socket()
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit("socket()");

	// bind()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("bind()");

	// setsockopt()
	int optval, optlen;

	// 초기 버퍼값 확인
	optlen = sizeof(optval);	// 65536
	retval = getsockopt(listen_sock, SOL_SOCKET, SO_RCVBUF, (char *)&optval, &optlen);	// 운영체제가 소켓에 할당해준 초기 버퍼값 가져오기.
	if (retval == SOCKET_ERROR) err_quit("getsockopt()");	// 에러핸들링

	// 수신 버퍼값 재설정
	optval *= 1000;
	retval = setsockopt(listen_sock, SOL_SOCKET, SO_RCVBUF, (char *)&optval, sizeof(optval));
	if (retval == SOCKET_ERROR) err_quit("setsockopt()");

	// 재설정한 버퍼값 확인
	optlen = sizeof(optval);
	retval = getsockopt(listen_sock, SOL_SOCKET, SO_RCVBUF, (char *)&optval, &optlen);
	if (retval == SOCKET_ERROR) err_quit("getsockopt()");

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) err_quit("listen()");

	// 서버 대기 상태 완료 =========================================

	// 데이터 통신에 사용할 변수
	SOCKET client_sock;	// 클라이언트 저장 소켓
	SOCKADDR_IN clientaddr;	// 클라이언트 주소 저장
	int addrlen;	// 주소 길이
	HANDLE hThread;

	while (1)
	{
		// accept()
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (SOCKADDR *)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET)
		{
			err_display("accept()");
			break;
		}

		// 접속한 클라이언트 정보 출력
		printf("\n[TCP 서버] 클라이언트 접속: IP 주소 = %s, 포트 번호 = %d \n",
			inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

		hThread = CreateThread(NULL, 0, ProcessClient, (LPVOID)client_sock, 0, NULL);	// 쓰레드 생성.
		if (hThread == NULL) closesocket(client_sock);
		else
		{
			SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL);
			CloseHandle(hThread);
		}
	}

	// 소켓 닫기
	closesocket(listen_sock);

	// 윈속 종료
	WSACleanup();
}

void err_display(const char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL
	);
	printf("[%s] %s", msg, (char *)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

void err_quit(const char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL
	);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, (LPCWSTR)msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

int recvn(SOCKET s, char *buf, int len, int flags)
{
	int received;
	char *ptr = buf;
	int left = len;

	while (left > 0)
	{
		received = recv(s, ptr, left, flags);
		if (received == SOCKET_ERROR)
			return SOCKET_ERROR;
		else if (received == 0)
			break;
		left -= received;
		ptr += received;
	}

	return (len - left);
}

// 시간함수 정의.
void getISOTime(char* buffer, size_t bufferSize) {
	struct tm t;
	time_t timer;

	timer = time(NULL);    // 현재 시각을 초 단위로 얻기
	localtime_s(&t, &timer); // 초 단위의 시간을 분리하여 구조체에 넣기

	sprintf_s(buffer, bufferSize, "%04d-%02d-%02d %02d:%02d:%02d",
		t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
		t.tm_hour, t.tm_min, t.tm_sec
	);
}
