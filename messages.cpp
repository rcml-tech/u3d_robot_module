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
#pragma comment(lib, "ws2_32") //link to dll

SOCKET SaR;

CRITICAL_SECTION G_CS_MES;

bool Postman_Thread_Exist = true; // Global Variable to Colose Postman Thread

//std::map<HANDLE*, std::string> tempMap;
std::map<int, std::map<HANDLE*, std::string>>  MutualMap;

int getUniqId(){
	static int uniq_id = 0;
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

	//char *tempRec;

	//char *twoMessInOneRec;
	//bool thereAreAnotherMessage =false;
	std::string tempString("");

	while (true)
	{
		EnterCriticalSection(&G_CS_MES);
		if (!Postman_Thread_Exist) { return 0; } // Close Thread
		LeaveCriticalSection(&G_CS_MES);

		is_recv = false;
		
		Sleep(10);

		if (tempString.find(perc) == -1){
			EnterCriticalSection(&G_CS_MES);
			recv(SaR, rec, 60, 0);
			LeaveCriticalSection(&G_CS_MES);
			tempString.assign(rec);
		}
		// Проверяем есть ли информация в сообщении
		if (tempString.find(perc) != -1) {
			while (true) {
				if ( tempString.find(amper) != -1) { // Полногстью ссчитали сообщение
					is_recv = true;
					break;
				}
				else {
					// ТОгда ссчитываем и прибавляем к строке пока не получим полностью сообщение
					EnterCriticalSection(&G_CS_MES);
					recv(SaR, rec, 60, 0);
					LeaveCriticalSection(&G_CS_MES);
					tempString.append(rec);
				}
			}// EndWhile
		}

		if (is_recv){
			// Теперь делаем подстроку - вырезаем то что у нас было и отправим это в наш обработчик
			std::string strToProcess = tempString.substr(tempString.find(perc), tempString.find(amper) - tempString.find(perc) + 1);
			tempString.assign(tempString.substr(tempString.find(amper) + 1));

			// Блок того что мы делаем а именно вырезаем наше сообщение
			int uniq_id = extractUniq_Id(strToProcess);
			//
			EnterCriticalSection(&G_CS_MES);
			for (std::map<int, std::map<HANDLE*, std::string>>::iterator i = MutualMap.begin(); i != MutualMap.end(); i++){
				if (i->first == uniq_id){
					i->second.begin()->second.assign(extractMessage(strToProcess)); // Записываем в map все что между % и &
					SetEvent( *(i->second.begin()->first) );
					break;
				} 
			} 
			LeaveCriticalSection(&G_CS_MES);
		} // End If
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

void testSuccess(char *str){
	if (strstr(str, "fail")) {
		throw std::exception();
	}
};
void testStringSuccess(std::string str){
	if (str.find("fail") != std::string::npos) {
		throw std::exception();
	}
};

std::string createMessage(std::string params){
	EnterCriticalSection(&G_CS_MES); //CRITICAL_SECTION
	static int UNIQ_ID = 0;
	UNIQ_ID++;
	int temp_ID = UNIQ_ID;

	HANDLE WaitRecivedMessage;
	WaitRecivedMessage = CreateEvent(NULL, true, false, NULL);

	char rec[60];
	std::string temp = "%%" + std::to_string(UNIQ_ID) + "+" + params + "&";

	send(SaR, temp.c_str(), temp.length(), 0);

	std::map<HANDLE*, std::string> tempMap;
	tempMap.insert(std::pair<HANDLE*, std::string>(&WaitRecivedMessage, ""));
	MutualMap.insert(std::pair<int, std::map<HANDLE*, std::string>>(UNIQ_ID, tempMap));
	tempMap.clear();

	LeaveCriticalSection(&G_CS_MES); // Вышли из критической секции

	printf("SendMessage: %d\n", UNIQ_ID);

	if (params.find("destroy") == -1){
		WaitForSingleObject(WaitRecivedMessage, INFINITE); // 
	}

	EnterCriticalSection(&G_CS_MES); // CRITICAL_SECTION

	for (std::map<int, std::map<HANDLE*, std::string>>::iterator i = MutualMap.begin(); i != MutualMap.end(); i++){
		if (i->first == temp_ID){
			temp = i->second.begin()->second; // Записываем в map все что между % и &
			MutualMap.erase(i); // Удаляем чтобы память не забивалась
			break;
		} 
	} 

	LeaveCriticalSection(&G_CS_MES); // Вышли из критической секции

	testStringSuccess(temp);
	return temp;
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

