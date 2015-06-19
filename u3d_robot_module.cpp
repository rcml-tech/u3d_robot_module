/*
* File: u3drobot_module.cpp
* Author: m79lol, iskinmike
*
*/
#ifdef _WIN32
	#define _CRT_SECURE_NO_WARNINGS 
	#define _SCL_SECURE_NO_WARNINGS
#endif

#include <string>
#include <vector>

#ifdef _WIN32
	#include <windows.h>
	#include <stdlib.h> 
#else
	#include <fcntl.h>
	#include <dlfcn.h>
	#include <pthread.h>
#endif

#include "SimpleIni.h"
#include "module.h"
#include "robot_module.h"
#include "u3d_robot_module.h"
#include "messages.h"

#ifdef _WIN32
	EXTERN_C IMAGE_DOS_HEADER __ImageBase;
	// Critical Atom_section
	#define DEFINE_ATOM(ATOM_NAME) CRITICAL_SECTION ATOM_NAME;
	#define ATOM_LOCK(ATOM_NAME) EnterCriticalSection( &ATOM_NAME );
	#define ATOM_UNLOCK(ATOM_NAME) LeaveCriticalSection( &ATOM_NAME );
#else
	// Critical Atom_section
	#define DEFINE_ATOM(ATOM_NAME) pthread_mutex_t ATOM_NAME = PTHREAD_MUTEX_INITIALIZER;
	#define ATOM_LOCK(ATOM_NAME) pthread_mutex_lock( &ATOM_NAME );
	#define ATOM_UNLOCK(ATOM_NAME) pthread_mutex_unlock( &ATOM_NAME );
#endif

/////////
const unsigned int COUNT_u3dRobot_FUNCTIONS = 5;
const unsigned int COUNT_AXIS = 0;

u3dRobotModule::u3dRobotModule() {
	u3drobot_functions = new FunctionData*[COUNT_u3dRobot_FUNCTIONS];
	system_value function_id = 0;

	FunctionData::ParamTypes *Params = new FunctionData::ParamTypes[6];
	Params[0] = FunctionData::ParamTypes::FLOAT;
	Params[1] = FunctionData::ParamTypes::FLOAT;
	Params[2] = FunctionData::ParamTypes::FLOAT;
	Params[3] = FunctionData::ParamTypes::FLOAT;
	Params[4] = FunctionData::ParamTypes::FLOAT;
	Params[5] = FunctionData::ParamTypes::STRING;

	u3drobot_functions[function_id] = new FunctionData(function_id+1, 6, Params, "spawn");
	function_id++;


	Params = new FunctionData::ParamTypes[3];
	Params[0] = FunctionData::ParamTypes::FLOAT;
	Params[1] = FunctionData::ParamTypes::FLOAT;
	Params[2] = FunctionData::ParamTypes::FLOAT;

	u3drobot_functions[function_id] = new FunctionData(function_id+1, 3, Params, "move");
	function_id++;


	Params = new FunctionData::ParamTypes[1];
	Params[0] = FunctionData::ParamTypes::STRING;

	u3drobot_functions[function_id] = new FunctionData(function_id+1, 1, Params, "changeColor");
	function_id++;


	u3drobot_functions[function_id] = new FunctionData(function_id+1, 0, NULL, "getX");
	function_id++;
	u3drobot_functions[function_id] = new FunctionData(function_id+1, 0, NULL, "getY");
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
	CSimpleIniA ini;
#ifdef _WIN32
	InitializeCriticalSection(&VRM_cs);
	ini.SetMultiKey(true);

	WCHAR DllPath[MAX_PATH] = { 0 };

	GetModuleFileNameW((HINSTANCE)&__ImageBase, DllPath, (DWORD) MAX_PATH);

	WCHAR *tmp = wcsrchr(DllPath, L'\\');
	WCHAR wConfigPath[MAX_PATH] = { 0 };
	size_t path_len = tmp - DllPath;
	wcsncpy(wConfigPath, DllPath, path_len);
	wcscat(wConfigPath, L"\\config.ini");

	char ConfigPath[MAX_PATH] = {0};
	wcstombs(ConfigPath,wConfigPath,sizeof(ConfigPath));
#else
	pthread_mutex_init(&VRM_cs, NULL);

	Dl_info PathToSharedObject;
	void * pointer = reinterpret_cast<void*> (getRobotModuleObject) ;
	dladdr(pointer,&PathToSharedObject);
	std::string dltemp(PathToSharedObject.dli_fname);

	int dlfound = dltemp.find_last_of("/");

	dltemp = dltemp.substr(0,dlfound);
	dltemp += "/config.ini";

	const char* ConfigPath = dltemp.c_str();
#endif
	if (ini.LoadFile(ConfigPath) < 0) {
		colorPrintf(this, ConsoleColor(ConsoleColor::red), "Can't load '%s' file!\n", ConfigPath);
		return 1;
	}

	CSimpleIniA::TNamesDepend values;
	CSimpleIniA::TNamesDepend IP;
	CSimpleIniA::TNamesDepend x, y, z;
	ini.GetAllValues("connection", "port", values);
	ini.GetAllValues("connection", "ip", IP);
	ini.GetAllValues("world", "x", x);
	ini.GetAllValues("world", "y", y);
	ini.GetAllValues("world", "z", z);

	CSimpleIniA::TNamesDepend::const_iterator ini_value;

	for (ini_value = values.begin(); ini_value != values.end(); ++ini_value) {
		colorPrintf(this, ConsoleColor(ConsoleColor::white), "Attemp to connect: %s\n", ini_value->pItem);
		int port = atoi(ini_value->pItem);

		std::string temp(IP.begin()->pItem);
		initConnection(port, temp);
		initWorld(atoi(x.begin()->pItem), atoi(y.begin()->pItem), atoi(z.begin()->pItem));
	}
	return 0;
};


Robot* u3dRobotModule::robotRequire(){
	ATOM_LOCK(VRM_cs);
	u3dRobot *u3d_robot = new u3dRobot(0);
	aviable_connections.push_back(u3d_robot);

	Robot *robot = u3d_robot;
	ATOM_UNLOCK(VRM_cs);
	return robot;
};


void u3dRobotModule::robotFree(Robot *robot){
	ATOM_LOCK(VRM_cs);
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
	ATOM_UNLOCK(VRM_cs);
};


void u3dRobotModule::final(){
	destroyWorld();
	aviable_connections.clear();
	closeSocketConnection();
};

void u3dRobotModule::destroy() {
	for (unsigned int j = 0; j < COUNT_u3dRobot_FUNCTIONS; ++j) {
		if (u3drobot_functions[j]->count_params) {
			delete[] u3drobot_functions[j]->params;
		}
		delete u3drobot_functions[j];
	}
	delete[] u3drobot_functions;
	delete this;
};

AxisData **u3dRobotModule::getAxis(unsigned int *count_axis){
	(*count_axis) = COUNT_AXIS;
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
		case 1: { // spawn
			variable_value *input1 = (variable_value *) args[0];
			variable_value *input2 = (variable_value *) args[1];
			variable_value *input3 = (variable_value *) args[2];
			variable_value *input4 = (variable_value *) args[3];
			variable_value *input5 = (variable_value *) args[4];
			std::string input6( (const char *) args[5]);
			robot_index = createRobot((int) *input1, (int) *input2, (int) *input3, (int) *input4, (int) *input5, input6);
			break;
		}
		case 2: { // move 
			if (!robot_index){ throw std::exception(); }
			variable_value *input1 = (variable_value *) args[0];
			variable_value *input2 = (variable_value *) args[1];
			variable_value *input3 = (variable_value *) args[2];
			moveRobot(robot_index, (int)*input1, (int)*input2, (int)*input3);
			break;
		}
		case 3: { // change Color
			if (!robot_index){ throw std::exception(); }
			std::string input1( (const char *) args[0] );
			colorRobot(robot_index, input1);
			break;
		}
		case 4: { // getX
			if (!robot_index){ throw std::exception(); }
			rez = coordsRobotX(robot_index);
			break;
		}
		case 5: { // getY
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

int u3dRobotModule::startProgram(int uniq_index) {
	return 0;
}

void u3dRobotModule::readPC(void *buffer, unsigned int buffer_length) {
}

int u3dRobotModule::endProgram(int uniq_index) {
	return 0;
}


PREFIX_FUNC_DLL RobotModule* getRobotModuleObject() {
	return new u3dRobotModule();
};
