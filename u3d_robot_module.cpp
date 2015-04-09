


#define _WINSOCK_DEPRECATED_NO_WARNINGS //Иначе ругается на устаревшую но удобную inet_addr
#define _CRT_SECURE_NO_WARNINGS // Иначе рушается на wcsncpy
#define _SCL_SECURE_NO_WARNINGS


#include <WinSock2.h>
#include <iostream>
#include <windows.h>
#include <time.h>
#include <vector>
#include <map>
#include <string>

#pragma comment(lib, "ws2_32") // Это подключение самой dll WinSock2.dll Не нужно в настройках указывать

#include "SimpleIni.h"
#include "module.h"
#include "robot_module.h"
#include "function_module.h"
#include "u3d_robot.h"

////////// Опишем глобальные константы и макросы
const unsigned int COUNT_u3dRobot_FUNCTIONS = 5;
const unsigned int COUNT_AXIS = 3;
int G_UNIQ_ID = 1; // Пришлось ввести пока Глобальную переменную чтобы были уникальные  ID процесса
SOCKET VR_socket;

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


std::string createWorldMessage(int uniq_id, int l, int w, int h){
	std::string fir = "%%";
	std::string plusr = "+init:";
	std::string rezult;

	// Собираем наше сообщение
	rezult.assign(fir);
	rezult.append(std::to_string( (int) uniq_id));
	rezult.append(plusr);
	rezult.append(std::to_string(l));
	rezult.append(",");
	rezult.append(std::to_string(w));
	rezult.append(",");
	rezult.append(std::to_string(h));
	rezult.append("&");

	return rezult;
};
std::string destroyWorldMessage(int uniq_id){
	std::string fir = "%%";
	std::string plusr = "+destroy&";
	std::string rezult;

	// Собираем наше сообщение
	rezult.assign(fir);
	rezult.append(std::to_string( (int) uniq_id));
	rezult.append(plusr);

	return rezult;
};

std::string deleteMessage(int uniq_id, int obj_id){
	std::string fir = "%%";
	std::string plusr = "+delete:";
	std::string rezult;

	// Собираем наше сообщение
	rezult.assign(fir);
	rezult.append(std::to_string( (int) uniq_id));
	rezult.append(plusr);
	rezult.append(std::to_string(obj_id));
	rezult.append("&");

	return rezult;
};
std::string createMessage(int uniq_id, std::string word, int x, int y, int d_x, int d_y, int d_z, int color){
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
	// Собираем наше сообщение
	rezult.assign(fir);
	rezult.append(std::to_string( (int) uniq_id));
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

std::string moveMessage(int uniq_id, std::string word, int obj_id, int x, int y, int speed){
	std::string fir = "%%";
	std::string plusr = "+robot:";

	std::string rezult;

	// Собираем наше сообщение
	rezult.assign(fir);
	rezult.append(std::to_string( (int) uniq_id));
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
std::string changecolorMessage(int uniq_id, std::string word, int obj_id, int color){
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
	// Собираем наше сообщение
	rezult.assign(fir);
	rezult.append(std::to_string( (int) uniq_id));
	rezult.append(plusr);
	rezult.append(word);
	rezult.append(",");
	rezult.append(std::to_string(obj_id));
	rezult.append(",");
	rezult.append(col);
	rezult.append("&");

	return rezult;
};

std::string reqXMessage(int uniq_id, std::string word, int obj_id){
	std::string fir = "%%";
	std::string plusr = "+robot:";
	std::string rezult = " 1";
	// Собираем наше сообщение
	rezult.assign(fir);
	rezult.append(std::to_string( (int) uniq_id));
	rezult.append(plusr);
	rezult.append(word);
	rezult.append(",");
	rezult.append(std::to_string((int)obj_id));
	rezult.append("&");

	return rezult;
};
std::string reqYMessage(int uniq_id, std::string word, int obj_id){
	std::string fir = "%%";
	std::string plusr = "+robot:";
	std::string rezult;

	// Собираем наше сообщение
	rezult.assign(fir);
	rezult.append(std::to_string( (int) uniq_id));
	rezult.append(plusr);
	rezult.append(word);
	rezult.append(",");
	rezult.append(std::to_string((int)obj_id));
	rezult.append("&");

	return rezult;
};
// Функция вытаскивающая из полученного сообщения ID объекта
int extractObj_id(char *str){

	int beg = 0;
	int end = 0;
	for (int i = 0; i < 30; i++){
		if (*(str + i) == ':'){ beg = i+1; };
		if (*(str + i) == '&'){ end = i; break; };
	};
	printf("- %s\n", " extract  works");

	printf("- %d\n", beg);
	printf("- %d\n", end);

	char *cht = new char[(end - beg)];
	printf("- %s\n", " extract works 2");

	for (int i = 0; i <(end-beg); i++){
		cht[i] =  *(str + i + beg) ;
	};
	//cht[j] = char(0);
	printf("- %d\n", atoi(cht));

	return atoi(cht);
};
int extractX(char *str){

	int beg = 0;
	int end = 0;
	for (int i = 0; i < 30; i++){
		printf("- %c\n", *(str + i));
		if (*(str + i) == ':'){ beg = i + 1; };
		if (*(str + i) == ','){ end = i; break; };
	};
	printf("- %s\n", " extract X works");

	printf("- %d\n", beg);
	printf("- %d\n", end);

	char *cht = new char[(end - beg)];
	printf("- %s\n", " extract X works 2");

	for (int i = 0; i <(end - beg); i++){
		cht[i] = *(str + i + beg);
	};
	//cht[j] = char(0);
	printf("- %d\n", atoi(cht));

	return atoi(cht);
};
int extractY(char *str){

	int beg = 0;
	int end = 0;
	for (int i = 0; i < 30; i++){
		printf("- %c\n", *(str + i));
		if (*(str + i) == ','){ beg = i + 1; };
		if (*(str + i) == '&'){ end = i; break; };
	};
	printf("- %s\n", " extract X works");

	printf("- %d\n", beg);
	printf("- %d\n", end);

	char *cht = new char[(end - beg)];
	printf("- %s\n", " extract X works 2");

	for (int i = 0; i <(end - beg); i++){
		cht[i] = *(str + i + beg);
	};
	//cht[j] = char(0);
	printf("- %d\n", atoi(cht));

	return atoi(cht);
};



// сделано!
const char* u3dRobotModule::getUID() {
	return "u3dRobot_functions_dll";
};

// сделано!
FunctionData** u3dRobotModule::getFunctions(unsigned int *count_functions) {
	*count_functions = COUNT_u3dRobot_FUNCTIONS;
	return u3drobot_functions;
}

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


int u3dRobotModule::init(){
	InitializeCriticalSection(&VRM_cs); // Инициализируем критическую секцию
	CSimpleIniA ini;
	ini.SetMultiKey(true);

	HMODULE lr_handle;

	lr_handle = GetModuleHandleW(L"u3d_robot.dll");

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


	if (ini.LoadFile("g:\\VSProjects\\VR_test_version\\VR_test_version\\config.ini") < 0) {
		printf("Can't load '%s' file!\n", "g:\\VSProjects\\VR_test_version\\VR_test_version\\config.ini");
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

		// Введем переменную для tcp сокета
		sockaddr_in addr;

		addr.sin_family = AF_INET; // семейство адресов interrnet
		addr.sin_port = htons(port); // Назначаем порт сокету
		addr.sin_addr.S_un.S_addr = inet_addr("192.168.1.128"); // Задаем конкретный адрес //Ругается на Deprecated inet_addr. Поэтому  использую _WINSOCK_DEPRECATED_NO_WARNINGS

		// теперь делаем Сокет котрый будет подключаться к нашему reciver'у
		VR_socket = socket(PF_INET, SOCK_STREAM, 0);

		u_long iMode = 1;
		int iResult;
		// Делаем сокеты неблокирующими
		iResult = ioctlsocket(VR_socket, FIONBIO, &iMode);
		if (iResult != NO_ERROR) {
			printf("ERROR_NONBLOCK: %d", iResult);
		}

		if (connect(VR_socket, (SOCKADDR *)&addr, sizeof(addr)) != 0) {
			//std::cout << "can't create connection" << WSAGetLastError() << "\n"; // Тут надо будет заменить на colorPrintF
		};

		Sleep(20);
		// создаем мир
		std::string mes = createWorldMessage(G_UNIQ_ID, 100, 100, 100);
		// send message to SOCKET
		G_UNIQ_ID++;
		send(VR_socket, mes.c_str(), mes.length(), 0);
		Sleep(500);
		// Сделал получение ответа после чтобы оно очередь не заьивало и всегда было доступно верное сообщение
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
				// Сделал получение ответа после чтобы оно очередь не заьивало и всегда было доступно верное сообщение
				char rec[30];
				recv(VR_socket, rec, 30, 0);
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

// Сделано!
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
			int pos_x = *args;
			int pos_y = *(args + 1);
			int d_x = *(args + 2);
			int d_y = *(args + 3);
			int d_z = *(args + 4);

			std::string mes;
			char rec[30];

			mes = createMessage(G_UNIQ_ID, "create", pos_x, pos_y, d_x, d_y, d_z, 1);
			// send message to SOCKET
			printf("- %s\n", "ex1");
			send(VR_socket, mes.c_str(), mes.length(), 0);
			printf("- %s\n", "ex2");
			Sleep(1500);
			printf("- %s\n", "ex3");
			// recive message from SOCKET
			recv(VR_socket, rec,30, 0);
			printf("- %s\n", rec);
			robot_index = extractObj_id(rec);
			printf("- %s\n", "ex5");
			is_Created = true;
			printf("- %s\n", "ex6");
			break;
		}
		case 2: {
			if (is_Created){
				// execute fucntion move
				int pos_x = *args;
				int pos_y = *(args + 1);
				int speed = *(args + 2);

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
				int color = *args;

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
				//int color = *args;

				std::string mes;
				char rec[30];
				// create our message
				printf("- %s\n", "ex1");
				mes = reqXMessage(G_UNIQ_ID, "coords", robot_index);
				printf("- %s\n", mes);
				// send message to SOCKET
				send(VR_socket, mes.c_str(), mes.length(), 0);
				printf("- %s\n", "ex3");
				Sleep(1500);
				// recive message from SOCKET
				recv(VR_socket, rec, 30, 0);
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
				//int color = *args;

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