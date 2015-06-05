/*
* File: messages.cpp
* Author: m79lol, iskinmike
*
*/

#ifdef _WIN32
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
	#define _CRT_SECURE_NO_WARNINGS 
	#define _SCL_SECURE_NO_WARNINGS

	#include <WinSock2.h>
	#include <windows.h>
	#pragma comment(lib, "ws2_32") //link to dll
#else
	#include <cstdarg>
	#include <pthread.h>
	#include <arpa/inet.h>
	#include <fcntl.h>
	#include <stdarg.h>
	#include <errno.h>
	#include <dlfcn.h>
	#include <sys/types.h>
	#include <sys/time.h>
	#include <sys/socket.h>
	#include <unistd.h>
	#include <stdio.h>
#endif

#include <string>
#include <time.h>
#include <map>
#include <vector>
#include <iostream>

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>

#include "messages.h"

#ifdef _WIN32
	//Typedefs section
	typedef HINSTANCE PLUGIN_HANDLE;
	typedef unsigned int THREAD_HANDLE;
	typedef HANDLE* PTR_EVENT_HANDLE;

	//Service section
	#define snprintf _snprintf
	#define sleep(X) Sleep(X*1000)

	//Threads and Atoms section
	#define DEFINE_ATOM(ATOM_NAME) CRITICAL_SECTION ATOM_NAME;
	#define PTR_DEFINE_ATOM(ATOM_NAME) CRITICAL_SECTION* ATOM_NAME;
	#define DEFINE_EVENT(EVENT_NAME) HANDLE EVENT_NAME = CreateEvent(NULL, true, false, NULL);
	//#define CREATE_EVENT(EVENT_NAME) EVENT_NAME = CreateEvent(NULL, true, false, NULL);
	#define DESTROY_EVENT(EVENT_NAME) CloseHandle(EVENT_NAME);
	#define DEFINE_THREAD_PROCEDURE(PROC_NAME) unsigned int WINAPI PROC_NAME( void* arg )
	#define ATOM_LOCK(ATOM_NAME) EnterCriticalSection( &ATOM_NAME );
	#define ATOM_UNLOCK(ATOM_NAME) LeaveCriticalSection( &ATOM_NAME );
	#define EVENT_WAIT(EVENT_NAME,ATOM_NAME) \
		if (WaitForSingleObject(EVENT_NAME, INFINITE) == WAIT_FAILED) \
		{ \
			addLog("beda\n"); \
		}
	#define EVENT_SEND(EVENT_NAME) SetEvent(EVENT_NAME)
	#define START_THREAD(PROC_NAME,PARAM,THREAD_ID,INDEX) \
		threadsHandles[INDEX] = (HANDLE) _beginthreadex(NULL,0,PROC_NAME,PARAM,0,&THREAD_ID)
	#define START_THREAD_DEMON(PROC_NAME,PARAM,THREAD_ID) \
		(HANDLE) _beginthreadex(NULL,0,PROC_NAME,PARAM,0,&THREAD_ID)

	//Sockets section
	#define SOCKET_ERROR_PROCESS(ERROR_DESCRIPTION) \
		{ \
			addLog(ERROR_DESCRIPTION,WSAGetLastError()); \
			WSACleanup(); \
			exit(1); \
		}
	#define SOCKET_CLOSE(SOCKET_NAME,ERROR_DESCRIPTION) \
		if (closesocket(SOCKET_NAME) == SOCKET_ERROR) \
		{ \
			addLog(ERROR_DESCRIPTION,WSAGetLastError()); \
		}
	#define SOCKET_NON_BLOCK(SOCKET_NAME,ERROR_DESCRIPTION) \
		iResult = ioctlsocket(SOCKET_NAME,FIONBIO,&iMode); \
		if (iResult != NO_ERROR) \
		{ \
			printf(ERROR_DESCRIPTION, iResult); \
		}
#else
	//Typedefs section
	typedef void * PLUGIN_HANDLE;
	typedef pthread_t THREAD_HANDLE;
	typedef int SOCKET;
	typedef sockaddr SOCKADDR;
	typedef pthread_cond_t* PTR_EVENT_HANDLE;

	//Threads and Atoms section
	#define DEFINE_ATOM(ATOM_NAME)   pthread_mutex_t ATOM_NAME = PTHREAD_MUTEX_INITIALIZER;
	#define PTR_DEFINE_ATOM(ATOM_NAME) pthread_mutex_t* ATOM_NAME; 
	#define DEFINE_EVENT(EVENT_NAME) pthread_cond_t  EVENT_NAME = PTHREAD_COND_INITIALIZER;
	//#define CREATE_EVENT(EVENT_NAME) EVENT_NAME = PTHREAD_COND_INITIALIZER;
	#define DESTROY_EVENT(EVENT_NAME) pthread_cond_destroy(&EVENT_NAME);
	#define DEFINE_THREAD_PROCEDURE(PROC_NAME) void * PROC_NAME(void *arg)
	#define ATOM_LOCK(ATOM_NAME) pthread_mutex_lock( &ATOM_NAME )
	#define ATOM_UNLOCK(ATOM_NAME) pthread_mutex_unlock( &ATOM_NAME )
	#define EVENT_WAIT(EVENT_NAME,ATOM_NAME) pthread_cond_wait(&EVENT_NAME,&ATOM_NAME);
	#define EVENT_SEND(EVENT_NAME) pthread_cond_signal(&EVENT_NAME);
	#define START_THREAD(PROC_NAME,PARAM,THREAD_ID,INDEX) \
		pthread_create(&THREAD_ID,NULL,&PROC_NAME,PARAM);
	#define START_THREAD_DEMON(PROC_NAME,PARAM,THREAD_ID) \
		pthread_create(&THREAD_ID,NULL,&PROC_NAME,PARAM);
	
	//Sockets section
	#define SOCKET_ERROR -1
	#define INVALID_SOCKET -1
	#define SOCKET_ERROR_PROCESS(ERROR_DESCRIPTION) \
		{ \
			addLog(ERROR_DESCRIPTION,errno); \
			exit(1); \
		}
	#define SOCKET_CLOSE(SOCKET_NAME,ERROR_DESCRIPTION) \
		close(SOCKET_NAME);
	#define SOCKET_NON_BLOCK(SOCKET_NAME,ERROR_DESCRIPTION) \
		if (fcntl(SOCKET_NAME, F_SETFL, O_NONBLOCK) == SOCKET_ERROR) \
		{ \
			printf("%d", iResult); \
		}
#endif

SOCKET SaR;

DEFINE_ATOM(G_CS_MES);

THREAD_HANDLE hPostman;
bool Postman_Thread_Exist = true;

std::vector<std::pair<PTR_EVENT_HANDLE, std::string> *> BoxOfMessages;

struct VectorAndCS{
	std::vector<std::pair<PTR_EVENT_HANDLE, std::string> *>* Vector;
	PTR_DEFINE_ATOM(CS);
};

VectorAndCS DataForSharedMemory;

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
	return (int) extractString(str, ':', '&');
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
	end = str.find(second)+1;

	temp.assign(str, beg, end - beg);
	return temp;
};

int extractUniq_Id(std::string str){
	return (int) extractString(str,'%','+');
};

// Thread Function
DEFINE_THREAD_PROCEDURE(PostmanThread){
	const int buffer_length = 1024;
	char rec[buffer_length] = { 0 };
	bool is_recv = false;
	char perc = '%';
	char amper = '&';

	std::string tempString("");
	std::string temp("");
	std::map<int, std::pair<PTR_EVENT_HANDLE, std::string>* > PostmansMap;

	fd_set ArrOfSockets;

	unsigned int Postmans_UNIQ_ID = 0;

	while (true)
	{
		std::cout << "Thread_works !!!!!!!!!!!!!!!" << std::endl;
		ATOM_LOCK(G_CS_MES);
		std::cout << "ATOM_LOCK Thread_1 "<< &G_CS_MES << std::endl;
		if (!Postman_Thread_Exist) { 
			std::cout << "Terminated " << std::endl;
			return 0; 
		} // Close Thread
		ATOM_UNLOCK(G_CS_MES);
		std::cout << "ATOM_LOCK Thread_1 Outside" << std::endl;
		timeval tVal; // For select function
		tVal.tv_sec = 1;
		tVal.tv_usec = 0;

		FD_ZERO(&ArrOfSockets);
		FD_SET(SaR,&ArrOfSockets);

		ATOM_LOCK(G_CS_MES);
		std::cout << "ATOM_LOCK Thread_2 "<< &G_CS_MES << std::endl;
		for (std::vector<std::pair<PTR_EVENT_HANDLE, std::string> *>::iterator i = BoxOfMessages.begin(); i != BoxOfMessages.end(); ++i){
			Postmans_UNIQ_ID++;
			PostmansMap[Postmans_UNIQ_ID] = (*i);
		}
		BoxOfMessages.clear();
		ATOM_UNLOCK(G_CS_MES);
		std::cout << "ATOM_LOCK Thread_2 Outside" << std::endl;

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
				NumberOfRecivedBytes = recv(SaR, rec, buffer_length, 0);
				if (NumberOfRecivedBytes == -1) { break; }
				tempString.append(rec, NumberOfRecivedBytes);
			} while (NumberOfRecivedBytes !=-1);


			// Now we cut string
			while (tempString.find(amper) != std::string::npos) {
				unsigned int PosAmper = tempString.find(amper);
				unsigned int PosPerc = tempString.find(perc);

				std::string strToProcess = tempString.substr(PosPerc, PosAmper - PosPerc + 1);
				tempString.assign(tempString.substr(PosAmper + 1));
				
				int uniq_id = extractUniq_Id(strToProcess);
				PostmansMap[uniq_id]->second = strToProcess;
				std::cout << "Postman sends EVENT!" << std::endl;
				EVENT_SEND(*(PostmansMap[uniq_id]->first))
				std::cout << "EVENT Sended" << std::endl;
				//SetEvent( *(PostmansMap[uniq_id]->first));

				PostmansMap.erase(uniq_id);
			};
		}
	}// EndWhile
};

void initConnection(int Port, std::string IP){
#ifdef _WIN32
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

#else

	sockaddr_in addr;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(Port);
	addr.sin_addr.s_addr = inet_addr(IP.c_str());

#endif
	
	std::cout << "init_starts" << std::endl;

	SaR = socket(PF_INET, SOCK_STREAM, 0);

	u_long iMode = 1;
	int iResult;

	SOCKET_NON_BLOCK(SaR,"Try to make socket nonblocking: ");
/*
	iResult = ioctlsocket(SaR, FIONBIO, &iMode);
	if (iResult != NO_ERROR) {
		printf("ERROR_NONBLOCK: %d", iResult);
	}
*/

	if (connect(SaR, (SOCKADDR *)&addr, sizeof(addr)) != 0) {
		//printf("ERROR can't connect: %d", GetLastError());
	};
	
	// Create File Mapping
	boost::interprocess::shared_memory_object shm_obj(boost::interprocess::open_or_create, "PostmansSharedMemory", boost::interprocess::read_write);

	shm_obj.truncate(sizeof(VectorAndCS*));
	boost::interprocess::mapped_region region(shm_obj, boost::interprocess::read_write);

	DataForSharedMemory.Vector = &BoxOfMessages;
	DataForSharedMemory.CS = &G_CS_MES;

	VectorAndCS *ptrDataForSharedMemory = &DataForSharedMemory; 
	std::memcpy(region.get_address(), &ptrDataForSharedMemory, region.get_size()); 
	
	// Start Thread
#ifdef _WIN32
	unsigned int unThreadID;
#else
	pthread_t unThreadID;
#endif	
	std::cout << "Thread_starts" << std::endl;
	hPostman = START_THREAD_DEMON(PostmanThread,NULL,unThreadID);
	std::cout << "init_ends" << std::endl;

	//#define START_THREAD_DEMON(&PostmanThread,NULL,&unThreadID)
	//	(HANDLE) _beginthreadex(NULL,0,PROC_NAME,PARAM,0,&THREAD_ID)
	//hPostman = (HANDLE)_beginthreadex(NULL, 0, (unsigned(__stdcall *)(void*)) &PostmanThread, NULL, 0, (unsigned *)&unThreadID);
};

// Close Connection
void closeSocketConnection(){
	ATOM_LOCK(G_CS_MES);
	std::cout << "ATOM_LOCK closeSocketConnection" << std::endl;
	Postman_Thread_Exist = false;
	ATOM_UNLOCK(G_CS_MES);
#ifdef _WIN32
	EVENT_WAIT(hPostman,G_CS_MES);
#else
	pthread_join(hPostman,NULL);
#endif
	//WaitForSingleObject(hPostman, INFINITE);
	boost::interprocess::shared_memory_object::remove("PostmansSharedMemory");
	SOCKET_CLOSE(SaR,"Can't close socket: ");
	//closesocket(SaR);
	
#ifdef _WIN32
	WSACleanup();
#endif
};

void testStringSuccess(std::string str){
	if (str.find("fail") != std::string::npos) {
		throw std::exception();
	}
};

std::string createMessage(std::string params){
	//HANDLE WaitRecivedMessage;
	DEFINE_EVENT(WaitRecivedMessage);
	//WaitRecivedMessage = CreateEvent(NULL, true, false, NULL);
	std::cout << "createMessage_starts:  "<< params.c_str() << std::endl;

	std::pair<PTR_EVENT_HANDLE, std::string> pairParams(&WaitRecivedMessage, params);
	std::cout << "Outside_ATOM_LOCK " << &G_CS_MES << std::endl;
	ATOM_LOCK(G_CS_MES);
	std::cout << "Inside_ATOM_LOCK" << std::endl;
	BoxOfMessages.push_back(&pairParams);
	std::cout << "Push_message_in_BoxOfMessages" << std::endl;
	ATOM_UNLOCK(G_CS_MES); 
	if (params != "destroy") {
		std::cout << "Wait_Postman" << std::endl;
		EVENT_WAIT(WaitRecivedMessage,G_CS_MES);
		std::cout << "Wait_Postman_success" << std::endl;
		//WaitForSingleObject(WaitRecivedMessage, INFINITE);
	}

	//CloseHandle(WaitRecivedMessage);
	DESTROY_EVENT(WaitRecivedMessage);

	testStringSuccess(pairParams.second);


	std::cout << "createMessage_ends" << std::endl;
	return pairParams.second;
};

void deleteRobot(int obj_id){
	std::string params("delete");

	params.append(":");
	params.append(std::to_string((int)obj_id));

	createMessage(params);
};

void destroyWorld(){
	createMessage("destroy");
};

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

	int d = extractObj_id(temp);
	return d;
};

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

