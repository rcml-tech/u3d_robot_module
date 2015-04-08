


#define _WINSOCK_DEPRECATED_NO_WARNINGS //����� �������� �� ���������� �� ������� inet_addr
#define _CRT_SECURE_NO_WARNINGS // ����� �������� �� wcsncpy
#define _SCL_SECURE_NO_WARNINGS


#include <WinSock2.h>
#include <iostream>
#include <windows.h>
#include <time.h>
#include <vector>
#include <map>
#include <string>

#pragma comment(lib, "ws2_32") // ��� ����������� ����� dll WinSock2.dll �� ����� � ���������� ���������

#include "SimpleIni.h"
#include "module.h"
#include "robot_module.h"
#include "function_module.h"
#include "virtual_robot.h"

////////// ������ ���������� ��������� � �������
const unsigned int COUNT_u3dRobot_FUNCTIONS = 5;
const unsigned int COUNT_AXIS = 3;
variable_value G_UNIQ_ID = 1; // �������� ������ ���� ���������� ���������� ����� ���� ����������  ID ��������
SOCKET VR_socket;

#define ADD_u3dRobot_FUNCTION(FUNCTION_NAME, COUNT_PARAMS, GIVE_EXCEPTION) \
u3drobot_functions[function_id] = new FunctionData; \
u3drobot_functions[function_id]->command_index = function_id + 1; \
u3drobot_functions[function_id]->count_params = COUNT_PARAMS; \
u3drobot_functions[function_id]->give_exception = GIVE_EXCEPTION; \
u3drobot_functions[function_id]->name = FUNCTION_NAME; \
function_id++;
// ����� �������

// ������ ������� ������� ��� ���� ������� ��������/ ������� ������� - ���������� �� ����� COUNT_u3dRobot_FUNCTIONS. � �� ������� ����� �����-���� ������� � ������.
#define DEFINE_ALL_FUNCTIONS \
ADD_u3dRobot_FUNCTION("spawn", 5, false)\
ADD_u3dRobot_FUNCTION("move", 3, false)\
ADD_u3dRobot_FUNCTION("changeColor", 1, false)\
ADD_u3dRobot_FUNCTION("getX", 0, false)\
ADD_u3dRobot_FUNCTION("getY", 0, false);

#define ADD_ROBOT_AXIS(AXIS_NAME, UPPER_VALUE, LOWER_VALUE) \
robot_axis[axis_id] = new AxisData; \
robot_axis[axis_id]->axis_index = axis_id + 1; \
robot_axis[axis_id]->upper_value = UPPER_VALUE; \
robot_axis[axis_id]->lower_value = LOWER_VALUE; \
robot_axis[axis_id]->name = AXIS_NAME; \
axis_id++;

#define DEFINE_ALL_AXIS \
ADD_ROBOT_AXIS("locked", 1, 0)\
ADD_ROBOT_AXIS("straight", 100, -100)\
ADD_ROBOT_AXIS("rotation", 100, -100);


std::string createWorldMessage(variable_value uniq_id, variable_value l, variable_value w, variable_value h){
	std::string fir = "%%";
	std::string plusr = "+init:";
	std::string rezult;

	// �������� ���� ���������
	rezult.assign(fir);
	rezult.append(std::to_string(uniq_id));
	rezult.append(plusr);
	rezult.append(std::to_string(l));
	rezult.append(",");
	rezult.append(std::to_string(w));
	rezult.append(",");
	rezult.append(std::to_string(h));
	rezult.append("&");

	return rezult;
};
std::string destroyWorldMessage(variable_value uniq_id){
	std::string fir = "%%";
	std::string plusr = "+destroy&";
	std::string rezult;

	// �������� ���� ���������
	rezult.assign(fir);
	rezult.append(std::to_string(uniq_id));
	rezult.append(plusr);

	return rezult;
};

std::string deleteMessage(variable_value uniq_id, variable_value obj_id){
	std::string fir = "%%";
	std::string plusr = "+delete:";
	std::string rezult;

	// �������� ���� ���������
	rezult.assign(fir);
	rezult.append(std::to_string(uniq_id));
	rezult.append(plusr);
	rezult.append(std::to_string(obj_id));
	rezult.append("&");

	return rezult;
};
std::string createMessage(variable_value uniq_id, std::string word, variable_value x, variable_value y, variable_value d_x, variable_value d_y, variable_value d_z, variable_value color){
	std::string fir = "%%";
	std::string plusr = "+robot:";
	std::string col;
	std::string rezult;

	switch ((int)color)
	{
	case 1:{
		col = "00FF00";
		break;
	}
	case 2:{
		col = "BB0000";
		break;
	}
	case 3:{
		col = "0000AC";
		break;
	}
	};
	// �������� ���� ���������
	rezult.assign(fir);
	rezult.append(std::to_string(uniq_id));
	rezult.append(plusr);
	rezult.append(word);
	rezult.append(",");
	rezult.append(std::to_string(x));
	rezult.append(",");
	rezult.append(std::to_string(y));
	rezult.append(",");
	rezult.append(std::to_string(d_x));
	rezult.append(",");
	rezult.append(std::to_string(d_y));
	rezult.append(",");
	rezult.append(std::to_string(d_z));
	rezult.append(",");
	rezult.append(col);
	rezult.append("&");

	return rezult;
};

std::string moveMessage(variable_value uniq_id, std::string word, variable_value obj_id, variable_value x, variable_value y, variable_value speed){
	std::string fir = "%%";
	std::string plusr = "+robot:";

	std::string rezult;

	// �������� ���� ���������
	rezult.assign(fir);
	rezult.append(std::to_string(uniq_id));
	rezult.append(plusr);
	rezult.append(word);
	rezult.append(",");
	rezult.append(std::to_string(obj_id));
	rezult.append(",");
	rezult.append(std::to_string(x));
	rezult.append(",");
	rezult.append(std::to_string(y));
	rezult.append(",");
	rezult.append(std::to_string(speed));
	rezult.append("&");

	return rezult;
};
std::string changecolorMessage(variable_value uniq_id, std::string word, variable_value obj_id, variable_value color){
	std::string fir = "%%";
	std::string plusr = "+robot:";
	std::string col;

	std::string rezult;

	switch ((int)color)
	{
	case 1:{
		col = "00FF00";
		break;
	}
	case 2:{
		col = "BB0000";
		break;
	}
	case 3:{
		col = "0000AC";
		break;
	}
	};
	// �������� ���� ���������
	rezult.assign(fir);
	rezult.append(std::to_string(uniq_id));
	rezult.append(plusr);
	rezult.append(word);
	rezult.append(",");
	rezult.append(std::to_string(obj_id));
	rezult.append(",");
	rezult.append(col);
	rezult.append("&");

	return rezult;
};

std::string reqXMessage(variable_value uniq_id, std::string word, variable_value obj_id){
	std::string fir = "%%";
	std::string plusr = "+robot:";
	std::string rezult;

	// �������� ���� ���������
	rezult.assign(fir);
	rezult.append(std::to_string(uniq_id));
	rezult.append(plusr);
	rezult.append(word);
	rezult.append(",");
	rezult.append(std::to_string(obj_id));
	rezult.append("&");

	return rezult;
};
std::string reqYMessage(variable_value uniq_id, std::string word, variable_value obj_id){
	std::string fir = "%%";
	std::string plusr = "+robot:";
	std::string rezult;

	// �������� ���� ���������
	rezult.assign(fir);
	rezult.append(std::to_string(uniq_id));
	rezult.append(plusr);
	rezult.append(word);
	rezult.append(",");
	rezult.append(std::to_string(obj_id));
	rezult.append("&");

	return rezult;
};
// ������� ������������� �� ����������� ��������� ID �������
variable_value extractObj_id(char *str){
	std::string temp(str);
	char ctmp[10];

	variable_value beg = temp.find(':') + 1;
	variable_value end = temp.find('&');
	variable_value len = end - beg;

	temp.copy(ctmp, len, beg);
	std::string tmp(ctmp);
	return std::stoi(tmp);
};
variable_value extractX(char *str){
	printf("- %s\n", "fex1");
	std::string temp(str);
	char chtmp[10];
	printf("- %s\n", "fex2");
	variable_value beg = temp.find(':') + 1;
	variable_value end = temp.find(',');
	variable_value len = end - beg;
	printf("- %s\n",temp);
	temp.copy(chtmp, len, beg);
	printf("- %s\n", "fex3");
	std::string tmp(chtmp);
	printf("- %s\n", "fex4");
	int inn = std::stoi(tmp);
	printf("- %s\n", "fex5");
	//printf("- %d\n", inn);
	return std::stoi(tmp);
};
variable_value extractY(char *str){
	std::string temp(str);
	char chtmp[10];

	variable_value beg = temp.find(',') + 1;
	variable_value end = temp.find('&');
	variable_value len = end - beg;

	temp.copy(chtmp, len, beg);
	std::string tmp(chtmp);
	return std::stoi(tmp);
};



// �������!
const char* u3dRobotModule::getUID() {
	return "u3dRobot_functions_dll";
};

// �������!
FunctionData** u3dRobotModule::getFunctions(unsigned int *count_functions) {
	*count_functions = COUNT_u3dRobot_FUNCTIONS;
	return u3drobot_functions;
}

// �������!
u3dRobotModule::u3dRobotModule() {
	srand(time(NULL));
	u3drobot_functions = new FunctionData*[COUNT_u3dRobot_FUNCTIONS];
	system_value function_id = 0;
	DEFINE_ALL_FUNCTIONS
		robot_axis = new AxisData*[COUNT_AXIS];
	system_value axis_id = 0;
	DEFINE_ALL_AXIS
		robot_id = 1;
};

// �������!
void u3dRobotModule::destroy() {
	for (unsigned int j = 0; j < COUNT_u3dRobot_FUNCTIONS; ++j) {
		delete u3drobot_functions[j];
	}
	for (unsigned int j = 0; j < COUNT_AXIS; ++j) {
		delete robot_axis[j];
	}
	delete[] u3drobot_functions;
	delete[] robot_axis;
	delete this;
};


int u3dRobotModule::init(){
	InitializeCriticalSection(&VRM_cs); // �������������� ����������� ������
	CSimpleIniA ini;
	ini.SetMultiKey(true);

	HMODULE lr_handle;

	lr_handle = GetModuleHandleW(L"u3d_robot.dll");

	WCHAR DllPath[MAX_PATH] = { 0 };

	//GetModuleFileNameW((HINSTANCE)&__ImageBase, DllPath, _countof(DllPath));
	GetModuleFileNameW(lr_handle, DllPath, _countof(DllPath));
	WCHAR *tmp = wcsrchr(DllPath, L'\\'); // ���� ��������� ��������� �����
	WCHAR ConfigPath[MAX_PATH] = { 0 };
	size_t path_len = tmp - DllPath; // ���������� ����� ���� �� ����� ��� ����� ���
	wcsncpy(ConfigPath, DllPath, path_len); // VStudio Reccomends wcsncpy_s() instead �������� �� DllPath ������ ����� ����
	wcscat(ConfigPath, L"\\config.ini"); // ����������� � ���� ��� ������ ����� ����� Simple.ini ������ ��� �����
	// �������� ��������� ��� ����


	//string str;


	if (ini.LoadFile("g:\\VSProjects\\VR_test_version\\VR_test_version\\config.ini") < 0) {
		printf("Can't load '%s' file!\n", "g:\\VSProjects\\VR_test_version\\VR_test_version\\config.ini");
		return 1;
	}

	CSimpleIniA::TNamesDepend values; // ����� ���������� ����� �������� � ��� �������� �� �������. � ����� ������ ����
	ini.GetAllValues("ports", "port", values);
	CSimpleIniA::TNamesDepend::const_iterator ini_value;
	for (ini_value = values.begin(); ini_value != values.end(); ++ini_value) {
		printf("- %s\n", ini_value->pItem);
		//std::string connection(ini_value->pItem);

		// ������ ������ ��������� ���� �����������
		WSADATA w;
		int error = WSAStartup(0x0202, &w);

		if (error) { return-1; };

		if (w.wVersion != 0x0202)
		{
			WSACleanup();
			return -1;
		}

		int port = std::stoi(ini_value->pItem);
		//colorPrintf(this, ConsoleColor(ConsoleColor::white), "Attemp to connect: %s\n", ini_value->pItem);

		// ������ ���������� ��� tcp ������
		sockaddr_in addr;

		addr.sin_family = AF_INET; // ��������� ������� interrnet
		addr.sin_port = htons(port); // ��������� ���� ������
		addr.sin_addr.S_un.S_addr = inet_addr("192.168.1.128"); // ������ ���������� ����� //�������� �� Deprecated inet_addr. �������  ��������� _WINSOCK_DEPRECATED_NO_WARNINGS

		// ������ ������ ����� ������ ����� ������������ � ������ reciver'�
		VR_socket = socket(PF_INET, SOCK_STREAM, 0);

		u_long iMode = 1;
		int iResult;
		// ������ ������ ��������������
		iResult = ioctlsocket(VR_socket, FIONBIO, &iMode);
		if (iResult != NO_ERROR) {
			printf("ERROR_NONBLOCK: %d", iResult);
		}

		if (connect(VR_socket, (SOCKADDR *)&addr, sizeof(addr)) != 0) {
			//std::cout << "can't create connection" << WSAGetLastError() << "\n"; // ��� ���� ����� �������� �� colorPrintF
		};

		Sleep(20);
		// ������� ���
		std::string mes = createWorldMessage(G_UNIQ_ID, 100, 100, 100);
		// send message to SOCKET
		G_UNIQ_ID++;
		send(VR_socket, mes.c_str(), mes.length(), 0);
		Sleep(500);
		// ������ ��������� ������ ����� ����� ��� ������� �� �������� � ������ ���� �������� ������ ���������
		char rec[22];
		recv(VR_socket, rec, 22, 0);

	}
	return 0;
};


Robot* u3dRobotModule::robotRequire(){
	EnterCriticalSection(&VRM_cs);
	u3dRobot *u3d_robot = new u3dRobot(robot_id);
	aviable_connections[robot_id] = u3d_robot;
	robot_id++;

	u3d_robot->isAviable = false;
	Robot *robot = u3d_robot;
	LeaveCriticalSection(&VRM_cs);
	return robot;
};


void u3dRobotModule::robotFree(Robot *robot){
	EnterCriticalSection(&VRM_cs);
	u3dRobot *u3d_robot = reinterpret_cast<u3dRobot*>(robot);
	for (m_connections::iterator i = aviable_connections.begin(); i != aviable_connections.end(); ++i) {
		if (i->second == u3d_robot) {
			if (u3d_robot->is_Created){
				u3d_robot->isAviable = true;
				std::string mes = deleteMessage(G_UNIQ_ID, u3d_robot->robot_index);
				// send message to SOCKET
				G_UNIQ_ID++;
				send(VR_socket, mes.c_str(), mes.length(), 0);
				// ������ ��������� ������ ����� ����� ��� ������� �� �������� � ������ ���� �������� ������ ���������
				char rec[22];
				recv(VR_socket, rec, 22, 0);
			}
			break;
		}
	}
	LeaveCriticalSection(&VRM_cs);
};


void u3dRobotModule::final(){
	//lego_communication_library::lego_brick^ singletoneBrick = lego_communication_library::lego_brick::getInstance();
	for (m_connections::iterator i = aviable_connections.begin(); i != aviable_connections.end(); ++i) {
		//singletoneBrick->disconnectBrick(i->second->robot_index);
		delete i->second;
	}
	aviable_connections.clear();
	std::string mes = destroyWorldMessage(G_UNIQ_ID);
	G_UNIQ_ID++;
	// send message to SOCKET
	send(VR_socket, mes.c_str(), mes.length(), 0);
	char rec[22];
	recv(VR_socket, rec, 22, 0);
};

// ������� !
AxisData **u3dRobotModule::getAxis(unsigned int *count_axis){
	(*count_axis) = COUNT_AXIS;
	return robot_axis;
};


void u3dRobot::axisControl(system_value axis_index, variable_value value){
	switch (axis_index){
	case 1:{
		is_locked = !!value;
		break;
	};
	case 2:
	case 3:
	case 4:
	case 5:{ // speedMotor A.B.C.D
		if (!is_locked){
			//lego_communication_library::lego_brick::getInstance()->motorSetSpeed(robot_index, (wchar_t)(63 + axis_index), (int)value);
		}
		break;
	};
	case 6:
	case 7:
	case 8:
	case 9:{ // moveMotor A,B,C,D
		if (!is_locked){
			//lego_communication_library::lego_brick::getInstance()->motorMoveTo(robot_index, (wchar_t)(59 + axis_index), 50, (int)value, false); //
		}
		break;
	};
	case 10:{
		if (!is_locked){
			//lego_communication_library::lego_brick::getInstance()->trackVehicleForward(robot_index, (int)value);
		}
		break;
	};
	case 11:{
		if (!is_locked){
			//lego_communication_library::lego_brick::getInstance()->trackVehicleSpinRight(robot_index, (int)value);
		}
		break;
	};
	};
};

// �������!
void u3dRobotModule::prepare(colorPrintf_t *colorPrintf_p, colorPrintfVA_t *colorPrintfVA_p) {
	colorPrintf = colorPrintf_p;
}


FunctionResult* u3dRobot::executeFunction(system_value functionId, variable_value *args) {
	if ((functionId < 1) || (functionId > COUNT_u3dRobot_FUNCTIONS)) {
		return NULL;
	}
	variable_value rez = 0;
	bool throw_exception = false;
	try {
		switch (functionId) {
		case 1: {
			//G_UNIQ_ID;
			variable_value pos_x = *args;
			variable_value pos_y = *(args + 1);
			variable_value d_x = *(args + 2);
			variable_value d_y = *(args + 3);
			variable_value d_z = *(args + 4);

			std::string mes;
			char rec[22];

			mes = createMessage(G_UNIQ_ID, "create", pos_x, pos_y, d_x, d_y, d_z, 1);
			// send message to SOCKET
			printf("- %s\n", "ex1");
			send(VR_socket, mes.c_str(), mes.length(), 0);
			printf("- %s\n", "ex2");
			Sleep(1500);
			printf("- %s\n", "ex3");
			// recive message from SOCKET
			recv(VR_socket, rec, 22, 0);
			printf("- %s\n", "ex4");
			robot_index = extractObj_id(rec);
			printf("- %s\n", "ex5");
			is_Created = true;
			printf("- %s\n", "ex6");
			break;
		}
		case 2: {
			if (is_Created){
				// execute fucntion move
				variable_value pos_x = *args;
				variable_value pos_y = *(args + 1);
				variable_value speed = *(args + 2);

				std::string mes;
				char rec[22];
				// create our message
				printf("- %s\n", "ex1");
				mes = moveMessage(G_UNIQ_ID, "move", robot_index, pos_x, pos_y, speed);
				printf("- %s\n", "ex2");
				// send message to SOCKET
				send(VR_socket, mes.c_str(), mes.length(), 0);
				Sleep(10);
				// recive message from SOCKET
				recv(VR_socket, rec, 22, 0);
			}
			else { throw std::exception(); };
			break;
		}
		case 3: {
			if (is_Created){
				// execute fucntion move
				variable_value color = *args;

				std::string mes;
				char rec[22];
				// create our message
				mes = changecolorMessage(G_UNIQ_ID, "color", robot_index, color);

				// send message to SOCKET
				send(VR_socket, mes.c_str(), mes.length(), 0);
				Sleep(10);
				// recive message from SOCKET
				recv(VR_socket, rec, 22, 0);
			}
			else { throw std::exception(); };
			break;
		}
		case 4: {
			if (is_Created){
				// execute fucntion move
				//variable_value color = *args;

				std::string mes;
				char rec[22];
				// create our message
				printf("- %s\n", "ex1");
				mes = reqXMessage(G_UNIQ_ID, "coords", robot_index);
				printf("- %s\n", "ex2");
				// send message to SOCKET
				send(VR_socket, mes.c_str(), mes.length(), 0);
				printf("- %s\n", "ex3");
				Sleep(1500);
				// recive message from SOCKET
				recv(VR_socket, rec, 22, 0);
				Sleep(500);
				printf("- %s\n", "ex4");
				rez = extractX(rec);
				printf("- %s\n", "ex5");
			}
			else { throw std::exception(); };
			break;
		}
		case 5: {
			if (is_Created){
				// execute fucntion move
				//variable_value color = *args;

				std::string mes;
				char rec[22];
				// create our message
				mes = reqYMessage(G_UNIQ_ID, "coords", robot_index);

				// send message to SOCKET
				send(VR_socket, mes.c_str(), mes.length(), 0);
				Sleep(10);
				// recive message from SOCKET
				recv(VR_socket, rec, 22, 0);

				rez = extractY(rec);
			}
			else { throw std::exception(); };
			break;
		}
		};
		G_UNIQ_ID++;
		return new FunctionResult(1, rez);
	}
	catch (...){
		G_UNIQ_ID++;
		return new FunctionResult(0);
	};
};

__declspec(dllexport) RobotModule* getRobotModuleObject() {
	return new u3dRobotModule();
};