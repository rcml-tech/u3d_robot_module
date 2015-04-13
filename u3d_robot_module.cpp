

#define _WINSOCK_DEPRECATED_NO_WARNINGS //Иначе ругается на устаревшую но удобную inet_addr
#define _CRT_SECURE_NO_WARNINGS // Иначе рушается на wcsncpy
#define _SCL_SECURE_NO_WARNINGS


#include <WinSock2.h>
#include <iostream>
#include <windows.h>
#include <time.h>
#include <vector>
#include <string>

#pragma comment(lib, "ws2_32") // Это подключение самой dll WinSock2.dll Не нужно в настройках указывать

#include "SimpleIni.h"
#include "module.h"
#include "robot_module.h"
#include "u3d_robot_module.h"
#include "messages.h"


////////// Опишем глобальные константы и макросы
const unsigned int COUNT_u3dRobot_FUNCTIONS = 5;
const unsigned int COUNT_AXIS = 3;
//int G_UNIQ_ID = 1; // Пришлось ввести пока Глобальную переменную чтобы были уникальные  ID процесса
//SOCKET VR_socket;

#define ADD_u3dRobot_FUNCTION(FUNCTION_NAME, COUNT_PARAMS, GIVE_EXCEPTION) \
u3drobot_functions[function_id] = new FunctionData; \
u3drobot_functions[function_id]->command_index = function_id + 1; \
u3drobot_functions[function_id]->count_params = COUNT_PARAMS; \
u3drobot_functions[function_id]->give_exception = GIVE_EXCEPTION; \
u3drobot_functions[function_id]->name = FUNCTION_NAME; \
function_id++;
// Конец макроса

// Опишем макросс который все наши функции заполнит/ Добавил Функцию - Увеличивай их число COUNT_u3dRobot_FUNCTIONS. А то удалишь нафиг какой-нить процесс в памяти.
#define DEFINE_ALL_FUNCTIONS \
ADD_u3dRobot_FUNCTION("spawn", 6, false)\
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


// сделано!
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

// Сделано!
void u3dRobotModule::prepare(colorPrintf_t *colorPrintf_p, colorPrintfVA_t *colorPrintfVA_p) {
	colorPrintf = colorPrintf_p;
}

// сделано!
const char* u3dRobotModule::getUID() {
	return "u3dRobot_functions_dll";
};

// сделано!
FunctionData** u3dRobotModule::getFunctions(unsigned int *count_functions) {
	*count_functions = COUNT_u3dRobot_FUNCTIONS;
	return u3drobot_functions;
}


int u3dRobotModule::init(){
	InitializeCriticalSection(&VRM_cs); // Инициализируем критическую секцию
	CSimpleIniA ini;
	ini.SetMultiKey(true);

	HMODULE lr_handle;

	lr_handle = GetModuleHandleW(L"u3drobot_module.dll");

	WCHAR DllPath[MAX_PATH] = { 0 };

	//GetModuleFileNameW((HINSTANCE)&__ImageBase, DllPath, _countof(DllPath));
	GetModuleFileNameW(lr_handle, DllPath, _countof(DllPath));
	WCHAR *tmp = wcsrchr(DllPath, L'\\'); // Ищет последнее вхождение слэша
	WCHAR ConfigPath[MAX_PATH] = { 0 };
	size_t path_len = tmp - DllPath; // определяет длину пути до папки где лежит длл
	wcsncpy(ConfigPath, DllPath, path_len); // VStudio Reccomends wcsncpy_s() instead Копирует из DllPath первую часть пути
	wcscat(ConfigPath, L"\\config.ini"); // Подставляет в путь имя конфиг файла чтобы Simple.ini смогла его найти
	// пытаемся загрузить наш файл


	//string str;


	if (ini.LoadFile(ConfigPath) < 0) {  
		printf("Can't load '%s' file!\n", ConfigPath);
		return 1;
	}

	CSimpleIniA::TNamesDepend values; // ввели переменную чтобы записать в нее значения из конфига. в нашем случае порт
	ini.GetAllValues("ports", "port", values);
	CSimpleIniA::TNamesDepend::const_iterator ini_value;
	for (ini_value = values.begin(); ini_value != values.end(); ++ini_value) {
		printf("- %s\n", ini_value->pItem);
		//std::string connection(ini_value->pItem);

		// теперь начнем создавать наши подключения
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


		initConnection(port);
		Sleep(20);

		initWorld(100,100,100);

	}
	return 0;
};


Robot* u3dRobotModule::robotRequire(){
	EnterCriticalSection(&VRM_cs);
	u3dRobot *u3d_robot = new u3dRobot(robot_id);
	aviable_connections.push_back(u3d_robot);// = u3d_robot;
	robot_id++;

	Robot *robot = u3d_robot;
	LeaveCriticalSection(&VRM_cs);
	return robot;
};


void u3dRobotModule::robotFree(Robot *robot){
	EnterCriticalSection(&VRM_cs);
	for (int i = 0; i < aviable_connections.size(); i++){
		if (aviable_connections[i]->is_Created){
			deleteRobot(aviable_connections[i]->robot_index);
			delete aviable_connections[i];
		}
	}
	//destroyWorld();
	LeaveCriticalSection(&VRM_cs);
};


void u3dRobotModule::final(){

	aviable_connections.clear();

};

// сделано!
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


// Сделано !
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



FunctionResult* u3dRobot::executeFunction(system_value functionId, variable_value *args) {
	if ((functionId < 1) || (functionId > COUNT_u3dRobot_FUNCTIONS)) {
		return NULL;
	}
	variable_value rez = 0;
	bool throw_exception = false;
	try {
		switch (functionId) {
		case 1: {
			robot_index = std::stoi( createRobot(*args, *(args + 1), *(args + 2), *(args + 3), *(args + 4), *(args + 5)) );
			is_Created = true;
			break;
		}
		case 2: {
			if (is_Created){
				// execute fucntion move
				moveRobot(robot_index, *args, *(args + 1), *(args + 2));
			}
			else { throw std::exception(); };
			break;
		}
		case 3: {
			if (is_Created){
				// execute fucntion change color
				colorRobot(robot_index, *args);
			}
			else { throw std::exception(); };
			break;
		}
		case 4: {
			if (is_Created){
				// execute fucntion getX
				rez = std::stoi( coordsRobotX(robot_index) );
			}
			else { throw std::exception(); };
			break;
		}
		case 5: {
			if (is_Created){
				// execute fucntion getY
				rez = std::stoi(coordsRobotY(robot_index));
			}
			else { throw std::exception(); };
			break;
		}
		};
		return new FunctionResult(1, rez);
	}
	catch (...){
		return new FunctionResult(0);
	};
};

__declspec(dllexport) RobotModule* getRobotModuleObject() {
	return new u3dRobotModule();
};