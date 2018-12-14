﻿// SPClient.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include "pch.h"
#pragma comment(lib, "ws2_32")
#pragma warning(disable:4996)
#include <WinSock2.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define SERVERIP "127.0.0.1"
#define SERVERPORT 9000
#define BUFSIZE 1020000

void err_quit(const char *);
void err_display(const char *);
int recvn(SOCKET, char *, int, int);
typedef struct Files
{
	char name[255];
	unsigned int byte;
}Files;

int main(int argc, char *argv[])
{
	unsigned int count;

	FILE *fp;
	Files files;

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
	
	fseek(fp, 0L, SEEK_END);
	files.byte = ftell(fp);
	fseek(fp, 0L, SEEK_SET);

	int retval;

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) err_quit("socket()");

	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(SERVERIP);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = connect(sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("connect()");

	char buf[BUFSIZE];

	retval = send(sock, (char *)&files, sizeof(files), 0);
	if (retval == SOCKET_ERROR)
	{
		err_display("send()");
		exit(1);
	}

	unsigned int per;
	per = count = files.byte / BUFSIZE;
	while (count)
	{
		system("cls");
		printf("전송하는 파일: %s, 전송하는 파일 크기: %d Byte\n", files.name, files.byte);

		fread(buf, 1, BUFSIZE, fp);

		retval = send(sock, buf, BUFSIZE, 0);
		if (retval == SOCKET_ERROR)
		{
			err_display("send()");
			exit(1);
		}

		printf("\n진행도: %d % %", (per - count) * 100 / per);

		count--;
	}

	count = files.byte - (per * BUFSIZE);
	fread(buf, 1, count, fp);

	retval = send(sock, buf, BUFSIZE, 0);
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

// 프로그램 실행: <Ctrl+F5> 또는 [디버그] > [디버깅하지 않고 시작] 메뉴
// 프로그램 디버그: <F5> 키 또는 [디버그] > [디버깅 시작] 메뉴

// 시작을 위한 팁: 
//   1. [솔루션 탐색기] 창을 사용하여 파일을 추가/관리합니다.
//   2. [팀 탐색기] 창을 사용하여 소스 제어에 연결합니다.
//   3. [출력] 창을 사용하여 빌드 출력 및 기타 메시지를 확인합니다.
//   4. [오류 목록] 창을 사용하여 오류를 봅니다.
//   5. [프로젝트] > [새 항목 추가]로 이동하여 새 코드 파일을 만들거나, [프로젝트] > [기존 항목 추가]로 이동하여 기존 코드 파일을 프로젝트에 추가합니다.
//   6. 나중에 이 프로젝트를 다시 열려면 [파일] > [열기] > [프로젝트]로 이동하고 .sln 파일을 선택합니다.