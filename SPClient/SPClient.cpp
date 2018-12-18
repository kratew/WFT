#include "pch.h"
#pragma comment(lib, "ws2_32")
#pragma warning(disable:4996)
#include <WinSock2.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define SERVERIP "192.168.7.180"
//#define SERVERIP "127.0.0.1"
#define SERVERPORT 9000

// 소켓 함수 오류 출력 후 종료
void err_quit(const char *);

// 소켓 함수 오류 출력
void err_display(const char *);

// 사용자 정의 데이터 수신 함수
int recvn(SOCKET, char *, int, int);

// 파일 기본 정보
typedef struct Files
{
	char name[255];
	unsigned int byte;
}Files;

int main(int argc, char *argv[])
{
	// (파일크기 / 버퍼 사이즈) 계산한 값을 while문으로 돌리기 위한 변수
	unsigned int count;
	int send_buffer;

	// 파일 이름 및 크기 확인
	FILE *fp;
	Files files;
	int i, j;
	int sum = 0;

	// 타임스탬프 확인
	time_t start, end;
	double result;
	
	// 전송할 파일 입력
	do
	{
		printf("보낼 파일을 적어 주세요: ");
		scanf("%s", files.name);
		getchar();
	} while (files.name == NULL);

	fp = fopen(files.name, "rb");
	if (fp == NULL)
	{
		printf("FILE Pointer ERROR");
		exit(1);
	}

	// 파일 끝으로 위치 옮김
	fseek(fp, 0L, SEEK_END);

	// 파일 바이트값 출력
	files.byte = ftell(fp);

	// 다시 파일 처음으로 위치 옮김
	fseek(fp, 0L, SEEK_SET);

	// send, recv 함수 출력값 저장용
	int retval;

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// socket()
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) err_quit("socket()");

	// setsockopt()
	int optlen;

	// 초기 버퍼값 확인
	optlen = sizeof(send_buffer);
	retval = getsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char *)&send_buffer, &optlen);
	if (retval == SOCKET_ERROR) err_quit("getsockopt()");

	// 송신 버퍼값 재설정
	send_buffer *= 1000; // 65535 * 1000
	retval = setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char *)&send_buffer, sizeof(send_buffer));
	if (retval == SOCKET_ERROR) err_quit("setsockopt()");
	
	// 재설정한 버퍼값 확인
	optlen = sizeof(send_buffer);
	retval = getsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char *)&send_buffer, &optlen);
	if (retval == SOCKET_ERROR) err_quit("getsockopt()");

	// connect()
	SOCKADDR_IN serveraddr;	// 서버와 통신용 소켓
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(SERVERIP);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = connect(sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("connect()");

	// 데이터 통신에 사용할 변수
	char *buf = new char[send_buffer];	// 보낼 데이터를 저장할 공간

	// 파일 기본 정보 전송
	retval = send(sock, (char *)&files, sizeof(files), 0);
	if (retval == SOCKET_ERROR)
	{
		err_display("send()");
		exit(1);
	}

	unsigned int per;
	per = count = files.byte / send_buffer;
	while (count)
	{
		system("cls");
		printf("전송하는 파일: %s, 전송하는 파일 크기: %d Byte\n", files.name, files.byte);

		// 파일 읽어서 버퍼에 저장
		fread(buf, 1, send_buffer, fp);

		// 전송
		retval = send(sock, buf, send_buffer, 0);
		if (retval == SOCKET_ERROR)
		{
			err_display("send()");
			exit(1);
		}

		//printf("\n진행도: %d % %", (per - count) * 100 / per, per);

		count--;

	}

	// 남은 파일 크기만큼 나머지 전송
	count = files.byte - (per * send_buffer);
	fread(buf, 1, count, fp);

	retval = send(sock, buf, send_buffer, 0);
	if (retval == SOCKET_ERROR)
	{
		err_display("send()");
		exit(1);
	}

	system("cls");
	printf("전송하는 파일 : %s, 전송하는 파일 크기 : %d Byte\n", files.name, files.byte);
	printf("\n진행도 : 100 %%");
	printf("\n\n전송이 완료되었습니다.");


	// closesocket()
	closesocket(sock);

	// 윈속 종료
	WSACleanup();

	//파일포인터 닫기
	fclose(fp);
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
