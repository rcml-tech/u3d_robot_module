/*
* File: messages.cpp
* Author: m79lol, iskinmike
*
*/

#include <stdlib.h>
#include <string>
#include <time.h>
#include <map>
#include <vector>

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>

#include "messages.h"
#include "define_section.h"
#include "stringC11.h"

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

	return strtod(temp.c_str(), NULL);
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
	char perc = '%';
	char amper = '&';

	std::string tempString("");
	std::string temp("");
	std::map<int, std::pair<PTR_EVENT_HANDLE, std::string>* > PostmansMap;

	fd_set ArrOfSockets;

	unsigned int Postmans_UNIQ_ID = 0;

	while (true)
	{
		ATOM_LOCK(G_CS_MES);
		if (!Postman_Thread_Exist) { 
			return 0; 
		} // Close Thread
		ATOM_UNLOCK(G_CS_MES);
		timeval tVal; // For select function
		tVal.tv_sec = 1;
		tVal.tv_usec = 0;

		FD_ZERO(&ArrOfSockets);
		FD_SET(SaR,&ArrOfSockets);

		ATOM_LOCK(G_CS_MES);
		for (std::vector<std::pair<PTR_EVENT_HANDLE, std::string> *>::iterator i = BoxOfMessages.begin(); i != BoxOfMessages.end(); ++i){
			Postmans_UNIQ_ID++;
			PostmansMap[Postmans_UNIQ_ID] = (*i);
		}
		BoxOfMessages.clear();
		ATOM_UNLOCK(G_CS_MES);

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
				EVENT_SEND(*(PostmansMap[uniq_id]->first));
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

	if (error) { printf("ERROR WSAStartup: %lu", GetLastError()); };

	if (w.wVersion != 0x0202)
	{
		WSACleanup();
		printf("ERROR Wrong Version of WSADATA: %lu", GetLastError());
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
	
	SaR = socket(PF_INET, SOCK_STREAM, 0);

	SOCKET_NON_BLOCK(SaR,"Try to make socket nonblocking: %d");

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
	hPostman = START_THREAD_DEMON(PostmanThread,NULL,unThreadID);
};

// Close Connection
void closeSocketConnection(){
	ATOM_LOCK(G_CS_MES);
	Postman_Thread_Exist = false;
	ATOM_UNLOCK(G_CS_MES);
#ifdef _WIN32
	EVENT_WAIT(hPostman,G_CS_MES);
#else
	pthread_join(hPostman,NULL);
#endif
	boost::interprocess::shared_memory_object::remove("PostmansSharedMemory");
	SOCKET_CLOSE(SaR,"Can't close socket: %d");
	DESTROY_ATOM(G_CS_MES);
	
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
	DEFINE_EVENT(WaitRecivedMessage);
	DEFINE_ATOM(WaitMessageMutex);
	std::pair<PTR_EVENT_HANDLE, std::string> pairParams(&WaitRecivedMessage, params);
	ATOM_LOCK(G_CS_MES);
	BoxOfMessages.push_back(&pairParams);
	ATOM_UNLOCK(G_CS_MES); 
	if (params != "destroy") {
		EVENT_WAIT(WaitRecivedMessage,WaitMessageMutex);
	}
	DESTROY_EVENT(WaitRecivedMessage);
	DESTROY_ATOM(WaitMessageMutex);
	testStringSuccess(pairParams.second);

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

