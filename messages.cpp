#define _WINSOCK_DEPRECATED_NO_WARNINGS //Иначе ругается на устаревшую но удобную inet_addr
#define _CRT_SECURE_NO_WARNINGS // Иначе рушается на wcsncpy
#define _SCL_SECURE_NO_WARNINGS

#include "messages.h"
// В этом файле будем описывать универсальные функции с которыми в дальнейшем смогут работать другие программы


SOCKET SaR;


void initConnection(int Port){
	sockaddr_in addr;

	addr.sin_family = AF_INET; // семейство адресов interrnet
	addr.sin_port = htons(Port); // Назначаем порт сокету
	addr.sin_addr.S_un.S_addr = inet_addr("192.168.1.128"); // Задаем конкретный адрес //Ругается на Deprecated inet_addr. Поэтому  использую _WINSOCK_DEPRECATED_NO_WARNINGS

	// теперь делаем Сокет котрый будет подключаться к нашему reciver'у
	SaR = socket(PF_INET, SOCK_STREAM, 0);

	u_long iMode = 1;
	int iResult;
	// Делаем сокеты неблокирующими
	iResult = ioctlsocket(SaR, FIONBIO, &iMode);
	if (iResult != NO_ERROR) {
		printf("ERROR_NONBLOCK: %d", iResult);
	}

	if (connect(SaR, (SOCKADDR *)&addr, sizeof(addr)) != 0) {
		//std::cout << "can't create connection" << WSAGetLastError() << "\n"; // Тут надо будет заменить на colorPrintF
	};


};


// Вырезает число из char строки
int extractor(char *str, char first, char second){
	char *fail = "fail";

	if (!strstr(str, fail)){
		char *temp = strchr(str, first);
		temp++;
		temp = _strrev(temp);
		temp = strchr(temp, second);
		temp++;
		temp = _strrev(temp);

		return atoi(temp);
	}
	else { return 0; }

};

int extractObj_id(char *str){
	return extractor(str, ':', '&');
};
int extractX(char *str){
	return extractor(str, ':', ',');
};
int extractY(char *str){
	return extractor(str, ',', '&');
};

// Выбирает цвет из нескольких вариантов
char* chooseColor(double arg){
	switch ((int)arg){
	case 1:  {return "00FF00"; }
	case 2:  {return "FFFF00"; }
	case 3:  {return "00FFFF"; }
	case 4:  {return "FF00FF"; }
	default: {return "F0F0F0"; }
	}
};


void testSuccess(char *str){
	if (strstr(str, "fail")) {
		throw std::exception();
	}
};

char *message(std::string name, std::string params){

		static int UNIC_ID=0;
		UNIC_ID++;
		char *rec = new char[60];
		std::string temp = "%%" + std::to_string(UNIC_ID) + "+" + name + params + "&";

		send(SaR, temp.c_str(), temp.length(), 0);
		Sleep(1500);
		recv(SaR,rec, 60, 0);

		testSuccess(rec);

		return rec;
};


// for DELETE
std::string deleteRobot( int obj_id){
	std::string params("");

	params.append(":");
	params.append(std::to_string(obj_id));

	char *temp;
	temp = message("delete", params);
	std::string st(temp);
	return st;
};

// for DESTROY
std::string destroyWorld(){
	char *temp;
	temp = message("destroy", "");
	std::string st(temp);
	return st;
};

// for INIT
std::string initWorld( int x, int y, int z){
	std::string params("");

	params.append(":");
	params.append(std::to_string(x));
	params.append(",");
	params.append(std::to_string(y));
	params.append(",");
	params.append(std::to_string(z));

	char *temp;
	temp = message("init", params);
	std::string st(temp);
	return st;
};

// for CREATE
std::string createRobot( int x, int y, int d_x, int d_y, int d_z, int color){
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

	char *temp;
	temp = message("robot", params);

	int i = extractObj_id(temp);

	std::string st(std::to_string(i));
	return st;
};

// for COLOR
std::string colorRobot(int obj_id, int color){
	std::string params("");

	params.append(":");
	params.append("color");
	params.append(",");
	params.append(std::to_string(obj_id));
	params.append(",");
	params.append(chooseColor(color));

	char *temp;
	temp = message("robot", params);
	std::string st(temp);
	return st;
};

// for MOVE
std::string moveRobot( int obj_id, int x, int y, int speed){
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

	char *temp;
	temp = message("robot", params);
	std::string st(temp);
	return st;
};

// for COORDS
std::string coordsRobotX(int obj_id){
	std::string params("");

	params.append(":");
	params.append("coords");
	params.append(",");
	params.append(std::to_string(obj_id));

	char *temp;
	temp = message("robot", params);

	int i = extractY(temp);
	std::string st(std::to_string(i));
	return st;
};
// for COORDS
std::string coordsRobotY(int obj_id){
	std::string params("");

	params.append(":");
	params.append("coords");
	params.append(",");
	params.append(std::to_string(obj_id));

	char *temp;
	temp = message("robot", params);

	int i = extractY(temp);
	std::string st(std::to_string(i));
	return st;
};






