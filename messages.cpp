/*
* File: messages.cpp
* Author: m79lol, iskinmike
*
*/

#define _WINSOCK_DEPRECATED_NO_WARNINGS 
#define _CRT_SECURE_NO_WARNINGS 
#define _SCL_SECURE_NO_WARNINGS

#include <string>
#include "messages.h"
#include <WinSock2.h>
#include <windows.h>
#include <process.h>
#include <time.h>
#include <map>
#include <vector>
#pragma comment(lib, "ws2_32") //link to dll

SOCKET SaR;

CRITICAL_SECTION G_CS_MES;

HANDLE hPostman;
bool Postman_Thread_Exist = true; // Global Variable to Colose Postman Thread

std::map<int, std::map<HANDLE*, std::string>>  MutualMap;

std::vector<std::pair<HANDLE*, std::string> *> BoxOfMessages;

double extractString(std::string str, char first, char second){

	std::string temp("");

	int beg = 0;
	int end = 0;
	int i = 1;
	if (first == '%') { i++; }
	beg = str.find(first) + i;
	end = str.find(second);

	temp.assign(str, beg, end - beg);

	return std::stod(temp);
};
int extractObj_id(std::string str){
	return extractString(str, ':', '&');
};
double extractX(std::string str){
	return extractString(str, ':', ',');
};
double extractY(std::string str){
	return extractString(str, ',', '&');
};
std::string extractMessage(std::string str){
	std::string temp("");

	char first = '+';
	char second = '&';

	unsigned int beg = 0;
	unsigned int end = 0;

	beg = str.find(first)+1;
	end = str.find(second)+1; // чтобы сохранить амперсанд и не надо было снова функции переписывать

	temp.assign(str, beg, end - beg);
	return temp;
};

int extractUniq_Id(std::string str){
	return extractString(str,'%','+');
};

unsigned int PostmanThread(){
	char rec[60] = {0};
	bool is_recv = false;
	char perc = '%';
	char amper = '&';

	std::string tempString("");
	std::string temp("");
	std::map<int, std::pair<HANDLE*, std::string>* > PostmansMap;

	fd_set ArrOfSockets;

	unsigned int Postmans_UNIQ_ID = 0;

	while (true)
	{
		EnterCriticalSection(&G_CS_MES);
		if (!Postman_Thread_Exist) { return 0; } // Close Thread
		LeaveCriticalSection(&G_CS_MES);

		timeval tVal; // For select function
		tVal.tv_sec = 1;
		tVal.tv_usec = 0;

		FD_ZERO(&ArrOfSockets);
		FD_SET(SaR,&ArrOfSockets);

		EnterCriticalSection(&G_CS_MES);
		for (std::vector<std::pair<HANDLE*, std::string> *>::iterator i = BoxOfMessages.begin(); i != BoxOfMessages.end(); ++i){
			Postmans_UNIQ_ID++;
			PostmansMap[Postmans_UNIQ_ID] = (*i);
		}
		BoxOfMessages.clear();
		LeaveCriticalSection(&G_CS_MES);

		// Now we work with own map
		for (auto i = PostmansMap.begin(); i != PostmansMap.end(); ++i){
			temp = "%%" + std::to_string(i->first) + "+" + i->second->second + "&";  // construct message
			send(SaR, temp.c_str(), temp.length(), 0);
			i->second->second.assign("");
		}
		
		if (select(SaR + 1, &ArrOfSockets, NULL, NULL, &tVal)) {
			// Without testing for errors
			int NumberOfRecivedBytes = 0;
			do
			{
				NumberOfRecivedBytes = recv(SaR, rec, 55, 0);
				if (NumberOfRecivedBytes == -1) { break; }
				tempString.append(rec, NumberOfRecivedBytes); // recive until -1 byte
			} while (NumberOfRecivedBytes !=-1);


			// Now we cut string
			while (tempString.find(amper) != std::string::npos) {
				unsigned int PosAmper = tempString.find(amper);
				unsigned int PosPerc = tempString.find(perc);

				std::string strToProcess = tempString.substr(PosPerc, PosAmper - PosPerc + 1);
				tempString.assign(tempString.substr(PosAmper + 1));
				
				int uniq_id = extractUniq_Id(strToProcess);
				PostmansMap[uniq_id]->second = strToProcess;
				SetEvent( *(PostmansMap[uniq_id]->first));
				PostmansMap.erase(uniq_id);
			};
		}
	}// EndWhile
};

void initConnection(int Port, std::string IP){
	InitializeCriticalSection(&G_CS_MES);

	WSADATA w;
	int error = WSAStartup(0x0202, &w);

	if (error) { printf("ERROR WSAStartup: %d", GetLastError()); };

	if (w.wVersion != 0x0202)
	{
		WSACleanup();
		printf("ERROR Wrong Version of WSADATA: %d", GetLastError());
	}

	sockaddr_in addr;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(Port);
	addr.sin_addr.S_un.S_addr = inet_addr(IP.c_str());

	SaR = socket(PF_INET, SOCK_STREAM, 0);

	u_long iMode = 1;
	int iResult;

	iResult = ioctlsocket(SaR, FIONBIO, &iMode);
	if (iResult != NO_ERROR) {
		printf("ERROR_NONBLOCK: %d", iResult);
	}

	if (connect(SaR, (SOCKADDR *)&addr, sizeof(addr)) != 0) {
		//printf("ERROR can't connect: %d", GetLastError());
	};

	// Start Thread
	unsigned int unThreadID;

	hPostman = (HANDLE)_beginthreadex(NULL, 0, (unsigned(__stdcall *)(void*)) &PostmanThread, NULL, 0, (unsigned *)&unThreadID);
};

// Close Connection
void closeSocketConnection(){
	EnterCriticalSection(&G_CS_MES);
	Postman_Thread_Exist = false;
	LeaveCriticalSection(&G_CS_MES);
	WaitForSingleObject(hPostman, INFINITE);
	closesocket(SaR);
	WSACleanup();
};

void testStringSuccess(std::string str){
	if (str.find("fail") != std::string::npos) {
		throw std::exception();
	}
};

std::string createMessage(std::string params){
	HANDLE WaitRecivedMessage;
	WaitRecivedMessage = CreateEvent(NULL, true, false, NULL);

	std::pair<HANDLE*, std::string> pairParams(&WaitRecivedMessage, params);
	EnterCriticalSection(&G_CS_MES); //CRITICAL_SECTION
	BoxOfMessages.push_back(&pairParams); //push_back
	LeaveCriticalSection(&G_CS_MES); 
	if (params != "destroy") {
		WaitForSingleObject(WaitRecivedMessage, INFINITE);
	}
	CloseHandle(WaitRecivedMessage);

	testStringSuccess(pairParams.second);
	return pairParams.second;
};

// for DELETE
void deleteRobot(int obj_id){
	std::string params("delete");

	params.append(":");
	params.append(std::to_string((int)obj_id));

	createMessage(params);
};
// for DESTROY
void destroyWorld(){
	createMessage("destroy");
};
// for INIT
void initWorld(int x, int y, int z){
	std::string params("init");

	params.append(":");
	params.append(std::to_string(x));
	params.append(",");
	params.append(std::to_string(y));
	params.append(",");
	params.append(std::to_string(z));

	createMessage(params);
};
// for CREATE
int createRobot(int x, int y, int d_x, int d_y, int d_z, std::string color){
	std::string params("robot");

	params.append(":");
	params.append("create");
	params.append(",");
	params.append(std::to_string(x));
	params.append(",");
	params.append(std::to_string(y));
	params.append(",");
	params.append(std::to_string(d_x));
	params.append(",");
	params.append(std::to_string(d_y));
	params.append(",");
	params.append(std::to_string(d_z));
	params.append(",");
	params.append(color);

	std::string temp;
	temp = createMessage(params);

	double d = extractObj_id(temp);
	return d;
};
// for COLOR
void colorRobot(int obj_id, std::string color){
	std::string params("robot");

	params.append(":");
	params.append("color");
	params.append(",");
	params.append(std::to_string(obj_id));
	params.append(",");
	params.append(color);

	createMessage(params);
};
// for MOVE
void moveRobot(int obj_id, int x, int y, int speed){
	std::string params("robot");

	params.append(":");
	params.append("move");
	params.append(",");
	params.append(std::to_string(obj_id));
	params.append(",");
	params.append(std::to_string(x));
	params.append(",");
	params.append(std::to_string(y));
	params.append(",");
	params.append(std::to_string(speed));

	createMessage(params);
};
// for COORDS
double coordsRobotX(int obj_id){
	std::string params("robot");

	params.append(":");
	params.append("coords");
	params.append(",");
	params.append(std::to_string(obj_id));

	std::string temp;
	temp = createMessage(params);

	double d = extractObj_id(temp);
	return d;
};
// for COORDS
double coordsRobotY(int obj_id){
	std::string params("robot");

	params.append(":");
	params.append("coords");
	params.append(",");
	params.append(std::to_string(obj_id));

	std::string temp;
	temp = createMessage(params);

	double d = extractObj_id(temp);
	return d;
};

