#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#pragma comment(lib, "wsock32.lib")
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <map>
#include "iostream"
#include "base64.h"
#include "sha1.h"
#include "websocket_request.h"
#define PORT 9006
#define TIMEWAIT 100
#define BUFFLEN 2048
#define MAXEVENTSSIZE 20
#define MAGIC_KEY "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
using namespace std;

void respond2(SOCKET fd, char msg[]) {

	byte bmsg[1024];
	int length = strlen(msg);
	cout << "信息长度" << length << endl;
	memcpy(bmsg, msg, length);

	char buf[10];

	int first = 0x82;
	int tmp = 0;
	buf[0] = first;
	tmp = 1;
	unsigned int nunum = (unsigned)length;
	if (length < 126) {
		buf[1] = static_cast<uint8_t>(length);
		tmp = 2;
	}
	else if (length < 65535) {
		buf[1] = 126;
		buf[2] = nunum >> 8;
		buf[3] = static_cast<uint8_t>(length) & 0xff;
		tmp = 4;
	}
	else {
		buf[1] = 127;
		buf[2] = 0;
		buf[3] = 0;
		buf[4] = 0;
		buf[5] = 0;
		buf[6] = nunum >> 24;
		buf[7] = nunum >> 16;
		buf[8] = nunum >> 8;
		buf[9] = nunum & 0xff;
		tmp = 10;
	}
	//真实长度
	int realdatelength = length + tmp + 1;

	char* charbuf;
	charbuf = (char*)malloc(realdatelength + 1);
	/*for (int i = 0; i < length; i++)
	{
		buf[tmp + i] = bmsg[i];
	}*/

	memcpy(charbuf, buf, tmp);
	memcpy(charbuf + tmp, bmsg, length);
	charbuf[realdatelength] = 0x00;
	cout << "response :" << charbuf << endl;

	send(fd, charbuf, realdatelength, 0);
	/*string outMessage;
	wsEncodeFrame(msg, outMessage, WS_TEXT_FRAME);
	cout << "消息" << outMessage << endl;
	send(fd, outMessage.data(), outMessage.length(), 0);*/

}
void respond(SOCKET fd, char* msg) {
	
	byte bmsg[1024];
	int length = strlen(msg);
	cout << "msg length:" << length << endl;
	memcpy(bmsg, msg, length);
	cout << "bmsg:" << bmsg << endl;
	//真实长度
	int realdatelength;
	if (length < 126) {
		realdatelength = length + 2;
	}
	else if (length < 65536) {
		realdatelength = length + 4;
	}
	else {
		realdatelength = length + 10;
	}
	char* buf;
	buf = (char*)malloc(realdatelength);
	char* charbuf;
	charbuf = (char*)malloc(realdatelength);
	int first = 0x00;
	int tmp = 0;
	if (true) {
		first = first + 0x80;
		first = first + 0x1;
	}
	buf[0] = first;
	tmp = 1;
	unsigned int nunum = (unsigned)length;
	if (length < 126) {
		buf[1] = length;
		tmp = 2;
	}
	else if (length < 65535) {
		buf[1] = 126;
		buf[2] = nunum >> 8;
		buf[3] = length & 0xff;
		tmp = 4;
	}
	else {
		buf[1] = 127;
		buf[2] = 0;
		buf[3] = 0;
		buf[4] = 0;
		buf[5] = 0;
		buf[6] = nunum >> 24;
		buf[7] = nunum >> 16;
		buf[8] = nunum >> 8;
		buf[9] = nunum & 0xff;
		tmp = 10;

	
	}
	for (int i = 0; i < length; i++)
	{
		buf[tmp + i] = bmsg[i];
	}
	
	memcpy(charbuf, buf, realdatelength);
	charbuf[realdatelength] = 0x00;
	cout << "response:" << charbuf << endl;
	send(fd, charbuf, realdatelength, 0);
	/*string outMessage;
	wsEncodeFrame(msg, outMessage, WS_TEXT_FRAME);
	cout << "消息" << outMessage << endl;
	send(fd, outMessage.data(), outMessage.length(), 0);*/

}

char* getClinetInfo(SOCKET fd, char clientinfo[]) {
	int point = 0;
	int tmppoint = 0;
	int byteArrayLength = sizeof(clientinfo);

	byte b[BUFFLEN] = "";
	memcpy(b, clientinfo, BUFFLEN);

	int first = b[point] & 0xFF;
	byte opCode = (byte)(first & 0xFF);
	
	Websocket_Request wbsq;
	wbsq.reset();
	wbsq.fetch_websocket_info(clientinfo);
	if (wbsq.opcode_ == 8) {
		//关闭信号
		cout << "client close!" << endl;
		closesocket(fd);
	}
	cout << "request :" << wbsq.payload_ << endl;
	char outMessage[BUFFLEN];
	memcpy(outMessage, wbsq.payload_, wbsq.payload_length_);
	outMessage[wbsq.payload_length_] = 0x00;
	return outMessage;

}

void handleHeader(char *request, string clientKey) {
	strcat(request, "HTTP/1.1 101 Switching Protocols\r\n");
	strcat(request, "Connection: upgrade\r\n");
	strcat(request, "Sec-WebSocket-Accept: ");
	std::string server_key = clientKey;
	server_key += MAGIC_KEY;

	SHA1 sha;
	unsigned int message_digest[5];
	sha.Reset();
	sha << server_key.c_str();

	sha.Result(message_digest);
	for (int i = 0; i < 5; i++) {
		message_digest[i] = htonl(message_digest[i]);
	}
	server_key = base64_encode(reinterpret_cast<const unsigned char*>(message_digest), 20);
	server_key += "\r\n";
	strcat(request, server_key.c_str());
	strcat(request, "Upgrade: websocket\r\n\r\n");
}
int handle(SOCKET fd) {
	char request[1024] = "";
	int result = -1;
	char revData[1024];
	int ret = recv(fd, revData, 1024, 0);
	string revDataString = revData;
	cout << "revDataString:" << revDataString << endl;
	string::size_type idx;
	idx = revDataString.find("GET");
	if (idx == string::npos) {
		cout << "不是握手协议" << endl;
	}
	else {
		cout << "start handle" << endl;
		int index = revDataString.find("Sec-WebSocket-Key");
		if (index > 0) {
			string secWebSocketKeyString = revDataString.substr(index + 19, 24);
			handleHeader(request, secWebSocketKeyString);
			//响应握手
			send(fd, request, strlen(request), 0);
			result = 1;
		}
		else {
			cout << "握手失败，没有Sec-WebSocket-Key" << endl;
		}
	}
	return result;
}

void run(SOCKET fd) {
	
	char clinetInfo[BUFFLEN] = "";
	int hre = handle(fd);
	if (hre > 0) {
		//握手成功
		while (true) {
		//	循环响应数据
			//清空数据
			memset(clinetInfo, '\0', sizeof(clinetInfo));
			int ret = recv(fd, clinetInfo, BUFFLEN, 0);
			if (ret > 0) {
				char* requestMsg = getClinetInfo(fd, clinetInfo);

				respond(fd, requestMsg);
			}
			else {
				//延迟一下，避免客户端突然断开造成cpu爆炸
				//todo 以后加入心跳
				Sleep(10);
			}

		}

	}


}

/****
websocket握手头信息
****/


int init() {
	WORD socketVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (WSAStartup(socketVersion, &wsaData) != 0) {
		cout << "WSAStartup失败!" << endl;
		return -1;
		
	}

	SOCKET listenfd_ = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd_ == -1) {
		cout << "创建套接字失败!" << endl;
		return -1;
	}
	sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(sockaddr_in));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(PORT);
	if (-1 == bind(listenfd_, (struct sockaddr*)(&server_addr), sizeof(server_addr))) {
		cout << ("绑定套接字失败!") << endl;
		return -1;
	}
	if (-1 == listen(listenfd_, 5)) {
		cout << ("监听失败!") << endl;
		return -1;
	}
	while (true) {
		struct sockaddr_in client_addr;
		int clilen = sizeof(client_addr);
		SOCKET fd = accept(listenfd_, (struct sockaddr*) & client_addr, &clilen);
		if (fd == INVALID_SOCKET) {
			cout << ("accept error!") << endl;

		}
		else {
			cout << ("ready handle!") << endl;
			HANDLE hT = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)run, (LPVOID)fd, 0, 0);
			if (hT != NULL) {
				CloseHandle(hT);
			}

		}
	}
}

int main(int argc, char** argv) {
	system("chcp 65001"); //控制台中文乱码问题
	init();
}