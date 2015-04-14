#define _WINSOCK_DEPRECATED_NO_WARNINGS //Иначе ругается на устаревшую но удобную inet_addr
#define _CRT_SECURE_NO_WARNINGS // Иначе рушается на wcsncpy
#define _SCL_SECURE_NO_WARNINGS

#include "messages.h"
// В этом файле будем описывать универсальные функции с которыми в дальнейшем смогут работать другие программы

SOCKET SaR;

void initConnection(int Port){

	// теперь начнем создавать наши подключения
	WSADATA w;
	int error = WSAStartup(0x0202, &w);

	if (error) { printf("ERROR WSAStartup: %d", GetLastError()); };

	if (w.wVersion != 0x0202)
	{
		WSACleanup();
		printf("ERROR Wrong Version of WSADATA: %d", GetLastError());
	}

	sockaddr_in addr;

	addr.sin_family = AF_INET; // семейство адресов interrnet
	addr.sin_port = htons(Port); // Назначаем порт сокету
	addr.sin_addr.S_un.S_addr = inet_addr("192.168.1.4"); // Задаем конкретный адрес //Ругается на Deprecated inet_addr. Поэтому  использую _WINSOCK_DEPRECATED_NO_WARNINGS

	// теперь делаем Сокет котрый будет подключаться к нашему reciver'у
	SaR = socket(PF_INET, SOCK_STREAM, 0);


	u_long iMode = 1;
	int iResult;
	// Делаем сокеты неблокирующими
	iResult = ioctlsocket(SaR, FIONBIO, &iMode);
	if (iResult != NO_ERROR) {
		printf("ERROR_NONBLOCK: %d", iResult);
	}

	Sleep(1000);

	if (connect(SaR, (SOCKADDR *)&addr, sizeof(addr)) != 0) {
		//std::cout << "can't create connection" << WSAGetLastError() << "\n"; // Тут надо будет заменить на colorPrintF
		//printf("ERROR can't connect: %d", GetLastError());
	};
	Sleep(1000);
};


// Close Connection
void closeSocketConnection(){
	closesocket(SaR);
	WSACleanup();
};

double extractString(std::string str, char first, char second){

	std::string temp("");

	int beg = 0;
	int end = 0;

	beg = str.find(first) + 1;
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

// Выбирает цвет из нескольких вариантов
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

std::string message(std::string name, std::string params){

		static int UNIC_ID=0;
		UNIC_ID++;

		char rec[60];
		std::string temp = "%%" + std::to_string(UNIC_ID) + "+" + name + params + "&";

		send(SaR, temp.c_str(), temp.length(), 0);
		Sleep(2000);
		recv(SaR,rec, 60, 0);

		//testSuccess(rec);
		std::string str(rec);
		return str;
};


// for DELETE
void deleteRobot( int obj_id){
	std::string params("");

	params.append(":");
	params.append(std::to_string((int)obj_id));

	std::string temp;
	temp = message("delete", params);

};

// for DESTROY
void destroyWorld(){
	std::string temp;
	temp = message("destroy", "");
};

// for INIT
void initWorld( int x, int y, int z){
	std::string params("");

	params.append(":");
	params.append(std::to_string(x));
	params.append(",");
	params.append(std::to_string(y));
	params.append(",");
	params.append(std::to_string(z));

	std::string temp;
	temp = message("init", params);
};

// for CREATE
double createRobot( int x, int y, int d_x, int d_y, int d_z, int color){
	std::string params("");

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
	temp = message("robot", params);

	double d = extractObj_id(temp);
	return d;
};

// for COLOR
void colorRobot(int obj_id, int color){
	std::string params("");

	params.append(":");
	params.append("color");
	params.append(",");
	params.append(std::to_string(obj_id));
	params.append(",");
	params.append(chooseColor(color));

	std::string temp;
	temp = message("robot", params);
};

// for MOVE
void moveRobot( int obj_id, int x, int y, int speed){
	std::string params("");

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

	std::string temp;
	temp = message("robot", params);

};

// for COORDS
double coordsRobotX(int obj_id){
	std::string params("");

	params.append(":");
	params.append("coords");
	params.append(",");
	params.append(std::to_string(obj_id));

	std::string temp;
	temp = message("robot", params);

	double d = extractObj_id(temp);
	return d;
};
// for COORDS
double coordsRobotY(int obj_id){
	std::string params("");

	params.append(":");
	params.append("coords");
	params.append(",");
	params.append(std::to_string(obj_id));

	std::string temp;
	temp = message("robot", params);

	double d = extractObj_id(temp);
	return d;
};






