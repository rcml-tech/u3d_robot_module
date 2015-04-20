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
#include <map>
#include <vector>
#pragma comment(lib, "ws2_32") //link to dll

SOCKET SaR;

CRITICAL_SECTION G_CS_MES;

bool Postman_Thread_Exist = true; // Global Variable to Colose Postman Thread

std::map<int, std::map<HANDLE*, std::string>>  MutualMap;

std::vector<std::pair<HANDLE*, std::string> *> BoxOfMessages;

int getUniqId(){
	static int uniq_id = 1;
	return uniq_id++;
};

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
double extractObj_id(std::string str){
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

	int beg = 0;
	int end = 0;

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

	timeval tVal; // For select function
	tVal.tv_sec = 1;
	tVal.tv_usec = 0;

	while (true)
	{
		EnterCriticalSection(&G_CS_MES);
		if (!Postman_Thread_Exist) { return 0; } // Close Thread
		LeaveCriticalSection(&G_CS_MES);

		fd_set ArrOfSockets;
		ArrOfSockets.fd_count = 1;
		ArrOfSockets.fd_array[0] = SaR;

		EnterCriticalSection(&G_CS_MES);
		for (std::vector<std::pair<HANDLE*, std::string> *>::iterator i = BoxOfMessages.begin(); i != BoxOfMessages.end(); ++i){
			PostmansMap[getUniqId()] = (*i); 
		}
		BoxOfMessages.clear();
		LeaveCriticalSection(&G_CS_MES);

		// Now we work with own map
		for (std::map<int, std::pair<HANDLE*, std::string> *>::iterator i = PostmansMap.begin(); i != PostmansMap.end(); ++i){
			temp = "%%" + std::to_string(i->first) + "+" + i->second->second + "&";  // construct message
			select(1, NULL, &ArrOfSockets, NULL, &tVal); // To be able to write Corretly
			send(SaR, temp.c_str(), temp.length(), 0);
			i->second->second.assign("");
		}


		if (select(1, &ArrOfSockets, NULL, NULL, &tVal)) {
			// Without testing for errors
			while (recv(SaR, rec, 60, 0) != -1){
				tempString.append(rec); // recive until -1 byte
			}

			// Now we cut string
			while (tempString != "") {
				
				std::string strToProcess = tempString.substr(tempString.find(perc), tempString.find(amper) - tempString.find(perc) + 1);
				tempString.assign(tempString.substr(tempString.find(amper) + 1));
				
				int uniq_id = extractUniq_Id(strToProcess);
				for (std::map<int, std::pair<HANDLE*, std::string> *>::iterator i = PostmansMap.begin(); i != PostmansMap.end(); ++i){
					if (uniq_id == i->first){
						i->second->second.assign(strToProcess);
						SetEvent(*(i->second->first));
						PostmansMap.erase(uniq_id);
						break;
					}
				}
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

	Sleep(1000);

	if (connect(SaR, (SOCKADDR *)&addr, sizeof(addr)) != 0) {
		//printf("ERROR can't connect: %d", GetLastError());
	};
	Sleep(1000);

	// Start Thread
	unsigned int unThreadID;

	HANDLE hPostman = (HANDLE)_beginthreadex(NULL, 0, (unsigned(__stdcall *)(void*)) &PostmanThread, NULL, 0, (unsigned *)&unThreadID);
};

// Close Connection
void closeSocketConnection(){
	EnterCriticalSection(&G_CS_MES);
	Postman_Thread_Exist = false;
	LeaveCriticalSection(&G_CS_MES);
	Sleep(20);
	closesocket(SaR);
	WSACleanup();
};

char* chooseColor(double arg){
	switch ((int)arg){
	case 1:  {return "00FF00"; }
	case 2:  {return "FFFF00"; }
	case 3:  {return "00FFFF"; }
	case 4:  {return "FF00FF"; }
	case 5:  {return "000000"; }
	case 6:  {return "0000FF"; }
	case 7:  {return "FFFFFF"; }
	case 8:  {return "008080"; }
	case 9:  {return "808000"; }
	case 10:  {return "000080"; }
	case 11:  {return "808080"; }
	case 12:  {return "800000"; }
	case 13:  {return "FF0000"; }
	case 14:  {return "C0C0C0"; }
	case 15:  {return "800080"; }
	case 16:  {return "008000"; }
	default: {return "4682B4"; }
	}
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
	BoxOfMessages.insert(BoxOfMessages.end(), &pairParams); //insert
	LeaveCriticalSection(&G_CS_MES); 

	WaitForSingleObject(WaitRecivedMessage, INFINITE);

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
double createRobot(int x, int y, int d_x, int d_y, int d_z, int color){
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
	params.append(chooseColor(color));

	std::string temp;
	temp = createMessage(params);

	double d = extractObj_id(temp);
	return d;
};
// for COLOR
void colorRobot(int obj_id, int color){
	std::string params("robot");

	params.append(":");
	params.append("color");
	params.append(",");
	params.append(std::to_string(obj_id));
	params.append(",");
	params.append(chooseColor(color));

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

