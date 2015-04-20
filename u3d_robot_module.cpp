/*
* File: u3drobot_module.cpp
* Author: m79lol, iskinmike
*
*/

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS 
#define _SCL_SECURE_NO_WARNINGS


#include <iostream>
#include <windows.h>
#include <vector>

#include "SimpleIni.h"
#include "module.h"
#include "robot_module.h"
#include "u3d_robot_module.h"
#include "messages.h"


EXTERN_C IMAGE_DOS_HEADER __ImageBase;

/////////
const unsigned int COUNT_u3dRobot_FUNCTIONS = 5;
const unsigned int COUNT_AXIS = 0;

#define ADD_SPAWN_FUNCTION(FUNCTION_NAME) \
u3drobot_functions[function_id] = new FunctionData; \
u3drobot_functions[function_id]->command_index = function_id + 1; \
u3drobot_functions[function_id]->count_params = 6; \
u3drobot_functions[function_id]->params = new FunctionData::ParamTypes[6]; \
u3drobot_functions[function_id]->params[0] = FunctionData::FLOAT; \
u3drobot_functions[function_id]->params[1] = FunctionData::FLOAT; \
u3drobot_functions[function_id]->params[2] = FunctionData::FLOAT; \
u3drobot_functions[function_id]->params[3] = FunctionData::FLOAT; \
u3drobot_functions[function_id]->params[4] = FunctionData::FLOAT; \
u3drobot_functions[function_id]->params[5] = FunctionData::FLOAT; \
u3drobot_functions[function_id]->name = FUNCTION_NAME; \
function_id++;
//////

#define ADD_MOVE_FUNCTION(FUNCTION_NAME) \
u3drobot_functions[function_id] = new FunctionData; \
u3drobot_functions[function_id]->command_index = function_id + 1; \
u3drobot_functions[function_id]->count_params = 3; \
u3drobot_functions[function_id]->params = new FunctionData::ParamTypes[3]; \
u3drobot_functions[function_id]->params[0] = FunctionData::FLOAT; \
u3drobot_functions[function_id]->params[1] = FunctionData::FLOAT; \
u3drobot_functions[function_id]->params[2] = FunctionData::FLOAT; \
u3drobot_functions[function_id]->name = FUNCTION_NAME; \
function_id++;
//////

#define ADD_CHANGECOLOR_FUNCTION(FUNCTION_NAME) \
u3drobot_functions[function_id] = new FunctionData; \
u3drobot_functions[function_id]->command_index = function_id + 1; \
u3drobot_functions[function_id]->count_params = 1; \
u3drobot_functions[function_id]->params = new FunctionData::ParamTypes[1]; \
u3drobot_functions[function_id]->params[0] = FunctionData::FLOAT; \
u3drobot_functions[function_id]->name = FUNCTION_NAME; \
function_id++;
//////

#define ADD_GET_FUNCTION(FUNCTION_NAME) \
u3drobot_functions[function_id] = new FunctionData; \
u3drobot_functions[function_id]->command_index = function_id + 1; \
u3drobot_functions[function_id]->count_params = 0; \
u3drobot_functions[function_id]->name = FUNCTION_NAME; \
function_id++;
//////

#define DEFINE_ALL_FUNCTIONS \
ADD_SPAWN_FUNCTION("spawn")\
ADD_MOVE_FUNCTION("move")\
ADD_CHANGECOLOR_FUNCTION("changeColor")\
ADD_GET_FUNCTION("getX")\
ADD_GET_FUNCTION("getY");
//////

u3dRobotModule::u3dRobotModule() {
	u3drobot_functions = new FunctionData*[COUNT_u3dRobot_FUNCTIONS];
	system_value function_id = 0;
	DEFINE_ALL_FUNCTIONS
};

void u3dRobotModule::prepare(colorPrintf_t *colorPrintf_p, colorPrintfVA_t *colorPrintfVA_p) {
	colorPrintf = colorPrintf_p;
}

const char* u3dRobotModule::getUID() {
	return "u3dRobot_functions_dll";
};

FunctionData** u3dRobotModule::getFunctions(unsigned int *count_functions) {
	*count_functions = COUNT_u3dRobot_FUNCTIONS;
	return u3drobot_functions;
}

int u3dRobotModule::init(){
	InitializeCriticalSection(&VRM_cs);
	CSimpleIniA ini;
	ini.SetMultiKey(true);

	WCHAR DllPath[MAX_PATH] = { 0 };

	GetModuleFileNameW((HINSTANCE)&__ImageBase, DllPath, _countof(DllPath));

	WCHAR *tmp = wcsrchr(DllPath, L'\\');
	WCHAR ConfigPath[MAX_PATH] = { 0 };
	size_t path_len = tmp - DllPath;
	wcsncpy(ConfigPath, DllPath, path_len);
	wcscat(ConfigPath, L"\\config.ini");

	if (ini.LoadFile(ConfigPath) < 0) {
		colorPrintf(this, ConsoleColor(ConsoleColor::red), "Can't load '%s' file!\n", ConfigPath);
		return 1;
	}

	CSimpleIniA::TNamesDepend values;
	CSimpleIniA::TNamesDepend IP;
	CSimpleIniA::TNamesDepend x, y, z;
	ini.GetAllValues("ports", "port", values);
	ini.GetAllValues("ips", "ip", IP);
	ini.GetAllValues("world", "x", x);
	ini.GetAllValues("world", "y", y);
	ini.GetAllValues("world", "z", z);
	CSimpleIniA::TNamesDepend::const_iterator ini_value;
	for (ini_value = values.begin(); ini_value != values.end(); ++ini_value) {
		colorPrintf(this, ConsoleColor(ConsoleColor::white), "Attemp to connect: %s\n", ini_value->pItem);
		int port = std::stoi(ini_value->pItem);

		std::string temp(IP.begin()->pItem);

		initConnection(port, temp);
		Sleep(20);

		initWorld(std::stoi(x.begin()->pItem), std::stoi(y.begin()->pItem), std::stoi(z.begin()->pItem));
	}
	return 0;
};


Robot* u3dRobotModule::robotRequire(){
	EnterCriticalSection(&VRM_cs);
	u3dRobot *u3d_robot = new u3dRobot(0);
	aviable_connections.push_back(u3d_robot);

	Robot *robot = u3d_robot;
	LeaveCriticalSection(&VRM_cs);
	return robot;
};


void u3dRobotModule::robotFree(Robot *robot){
	EnterCriticalSection(&VRM_cs);
	u3dRobot *u3d_robot = reinterpret_cast<u3dRobot*>(robot);
	for (m_connections::iterator i = aviable_connections.begin(); i != aviable_connections.end(); ++i) {
		if (u3d_robot == *i){
			if ( (*i)->robot_index ){
				deleteRobot((*i)->robot_index);
			};
			delete (*i);
			aviable_connections.erase(i);
			break;
		};
	}
	LeaveCriticalSection(&VRM_cs);
};


void u3dRobotModule::final(){
	destroyWorld();
	aviable_connections.clear();
	closeSocketConnection();
};

void u3dRobotModule::destroy() {
	for (unsigned int j = 0; j < COUNT_u3dRobot_FUNCTIONS; ++j) {
		delete u3drobot_functions[j];
	}
	delete[] u3drobot_functions;
	delete this;
};

AxisData **u3dRobotModule::getAxis(unsigned int *count_axis){
	count_axis = COUNT_AXIS;
	return NULL;
};

void u3dRobot::axisControl(system_value axis_index, variable_value value){
};

void *u3dRobotModule::writePC(unsigned int *buffer_length) {
	*buffer_length = 0;
	return NULL;
}

FunctionResult* u3dRobot::executeFunction(system_value functionId, void **args) {
	if ((functionId < 1) || (functionId > COUNT_u3dRobot_FUNCTIONS)) {
		return NULL;
	}
	variable_value rez = 0;
	try {
		switch (functionId) {
		case 1: {
			variable_value *input1 = (variable_value *)(*args);
			variable_value *input2 = (variable_value *)(*(args + 1));
			variable_value *input3 = (variable_value *)(*(args + 2));
			variable_value *input4 = (variable_value *)(*(args + 3));
			variable_value *input5 = (variable_value *)(*(args + 4));
			variable_value *input6 = (variable_value *)(*(args + 5));
			robot_index = createRobot(*input1, *input2, *input3, *input4, *input5, *input6);
			break;
		}
		case 2: {
			variable_value *input1 = (variable_value *)(*args);
			variable_value *input2 = (variable_value *)(*(args + 1));
			variable_value *input3 = (variable_value *)(*(args + 2));
			if (!robot_index){ throw std::exception(); }
			moveRobot(robot_index, *input1, *input2, *input3);
			break;
		}
		case 3: {
			variable_value *input1 = (variable_value *)(*args);
			if (!robot_index){ throw std::exception(); }
			colorRobot(robot_index, *input1);
			break;
		}
		case 4: {
			if (!robot_index){ throw std::exception(); }
			rez = coordsRobotX(robot_index);
			break;
		}
		case 5: {
			if (!robot_index){ throw std::exception(); }
			rez = coordsRobotY(robot_index);
			break;
		}
		};
		return new FunctionResult(1, rez);
	}
	catch (...){
		return new FunctionResult(0);
	};
};

int u3dRobotModule::startProgram(int uniq_index, void *buffer, unsigned int buffer_length) {
	return 0;
}

int u3dRobotModule::endProgram(int uniq_index) {
	return 0;
}


__declspec(dllexport) RobotModule* getRobotModuleObject() {
	return new u3dRobotModule();
};