// server1.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include "pch.h"
#pragma comment(lib, "ws2_32")
#pragma warning(disable:4996)
#include <WinSock2.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define SERVERPORT 9000
#define BUFSIZE 1020000

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
	char buf[BUFSIZE + 1];
	unsigned int count;
	char timeBuffer[80];

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
		getISOTime(buf, sizeof(buf));
		printf("전송 시각: %s\n", buf);
		printf("전송하는 파일: %s, 전송하는 파일 크기: %d Byte\n", files.name, files.byte);
		printf("\n클라이언트로 부터 파일을 전송 받는 중 입니다.\n");

		fp = fopen(files.name, "wb");

		count = files.byte / BUFSIZE;

		while (count)
		{
			// 받기
			retval = recvn(client_sock, buf, BUFSIZE, 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("recv()");
				exit(1);
			}

			// 파일 작성 작업
			fwrite(buf, 1, BUFSIZE, fp);

			count--;
		}

		// 남은 파일 크기만큼 나머지 받기
		count = files.byte - ((files.byte / BUFSIZE) * BUFSIZE);

		retval = recvn(client_sock, buf, BUFSIZE, 0);
		if (retval == SOCKET_ERROR)
		{
			err_display("recv()");
			exit(1);
		}

		fwrite(buf, 1, count, fp);

		//파일 포인터 닫기
		fclose(fp);

		printf("\n파일 전송이 완료 되었습니다.\n");
		getISOTime(buf, sizeof(buf));
		printf("완료 시각: %s\n", buf);

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

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) err_quit("listen()");

	// 서버 대기 상태 완료 =========================================

	// 데이터 통신에 사용할 변수
	SOCKET client_sock;	// 클라이언트 저장 소켓
	SOCKADDR_IN clientaddr;	// 클라이언트 주소 저장
	int addrlen;	// 주소 길이
	char buf[BUFSIZE];	// 전송하는 데이터
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

		hThread = CreateThread(NULL, 0, ProcessClient, (LPVOID)client_sock, 0, NULL);
		if (hThread == NULL) {
			closesocket(client_sock);
		}
		else {
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
// 프로그램 실행: <Ctrl+F5> 또는 [디버그] > [디버깅하지 않고 시작] 메뉴
// 프로그램 디버그: <F5> 키 또는 [디버그] > [디버깅 시작] 메뉴

// 시작을 위한 팁: 
//   1. [솔루션 탐색기] 창을 사용하여 파일을 추가/관리합니다.
//   2. [팀 탐색기] 창을 사용하여 소스 제어에 연결합니다.
//   3. [출력] 창을 사용하여 빌드 출력 및 기타 메시지를 확인합니다.
//   4. [오류 목록] 창을 사용하여 오류를 봅니다.
//   5. [프로젝트] > [새 항목 추가]로 이동하여 새 코드 파일을 만들거나, [프로젝트] > [기존 항목 추가]로 이동하여 기존 코드 파일을 프로젝트에 추가합니다.
//   6. 나중에 이 프로젝트를 다시 열려면 [파일] > [열기] > [프로젝트]로 이동하고 .sln 파일을 선택합니다.