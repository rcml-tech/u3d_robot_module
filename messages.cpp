#define _WINSOCK_DEPRECATED_NO_WARNINGS //»наче ругаетс€ на устаревшую но удобную inet_addr
#define _CRT_SECURE_NO_WARNINGS // »наче рушаетс€ на wcsncpy
#define _SCL_SECURE_NO_WARNINGS

#include "messages.h"
// ¬ этом файле будем описывать универсальные функции с которыми в дальнейшем смогут работать другие программы



// ¬ырезает число из char строки
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


// 
char *sendAndRec(SOCKET SR, char *mes, int chrec){
	char *rec = new char[chrec];
	*rec = { 0 };
	//Send message to socket
	std::string temp(mes);

	//send(SR, mes, strlen(mes), 0);
	send(SR, temp.c_str(), temp.length() , 0);
	int tmp = strlen(mes); // дл€ отладки
	Sleep(1500);
	// recive message from SOCKET
	recv(SR, rec, chrec, 0);
	return rec;
};

// ¬ыбирает цвет из нескольких вариантов
char* chooseColor(double arg){
	switch ((int)arg){
	case 1:  {return "00FF00"; }
	case 2:  {return "FFFF00"; }
	case 3:  {return "00FFFF"; }
	case 4:  {return "FF00FF"; }
	default: {return "F0F0F0"; }
	}
};

// создает строку из аргументов
char *crtParamSrting(double *args, int numArgs){
	char temp[20] = {0};
	char arg[20] = {0};

	for (int i = 0; i < numArgs; i++){
		_itoa((int)*(args+i), arg, 10);
		strcat(temp, ",");
		strcat(temp, arg);
	}
	return temp;
};

char *objIdToString(int id){
	char temp[20] = { 0 };
	char arg[20] = { 0 };

	if (id !=0) {
		_itoa(id, arg, 10);
		strcat(temp, ",");
		strcat(temp, arg);
	}
	return temp;
};



// —оздает сообщение
char *crtSendSrting(int uniq_id, char *name, char *word, int obj_id, double *args, int numArgs){
	char perc[3] = "%%";
	char plus[2] = "+";
	char dots[2] = ":";
	char amper[2] = "&";


	char uniq[10];
	_itoa(uniq_id,uniq, 10);

	char *temp = new char[60];
	*temp = {0};
	//char *tp="";

	strcat(temp, perc);
	strcat(temp, uniq);
	strcat(temp, plus);
	strcat(temp, name);
	if (!strcmp(name, "destroy")) {
		strcat(temp, amper);
		return temp;
	}
	strcat(temp, dots);
	strcat(temp, word);
	if (!strcmp(name, "delete")) {
		strcat(temp, objIdToString(obj_id)+1);
		strcat(temp, amper);
		return temp;
	}
	strcat(temp, objIdToString(obj_id));
	// ¬ этом меесте надо свитч который решит в зависимости от какого word что делать с цветом
	if (!strcmp(word, "color") || !strcmp(word, "create")){
		strcat(temp, crtParamSrting(args, numArgs-1));
		char *color = chooseColor(*(args+numArgs-1));
		strcat(temp, ",");
		strcat(temp, color);
	}
	else
	{   // Ёто дл€ того чтобы от зап€той избавитьс€ в определенных случа€х
		if (!strcmp(name ,"delete") || !strcmp(name, "init")){
			char *t = crtParamSrting(args, numArgs);
			strcat(temp, t+1);
		}
		else{
			strcat(temp, crtParamSrting(args, numArgs));
		}

	}
	strcat(temp, amper);

	return temp;
};

int messagePart(SOCKET SR, int uniq_id, int obj_id, double *args, int numArgs, int rcBytes, char *fname, char *fword){
	
	char *sndstr = crtSendSrting(uniq_id, fname, fword, obj_id, args, numArgs);
	char *rec;// = new char[rcBytes];
	rec = NULL;
	//char rec[60] = {0};
	rec = sendAndRec(SR, sndstr, rcBytes); // получили сообщение
	int tmp;
	if (!strcmp(fname, "init") || !strcmp(fname, "destroy") || !strcmp(fword, "move") || !strcmp(fword, "color") || !strcmp(fword, "move")) {
		tmp = 0;
	}
	else {
		tmp = extractObj_id(rec);
	};
	//delete[] rec;
	return tmp;
};


void createWorld(SOCKET SR,int uniq_id, double *args,int numArgs, int rcBytes){
	char fname[5] = "init";
	char fword[1] = "";
	int tmp = messagePart(SR, uniq_id, 0, args, numArgs, rcBytes, fname, fword);
};
void destroyWorld(SOCKET SR, int uniq_id, int rcBytes){
	char *fname = "destroy";
	char *fword = "";
	double *tmp = 0;
	int temp = messagePart(SR, uniq_id,0, tmp, 0, rcBytes, fname, fword);
};

// return robot's ID
int createRobot(SOCKET SR, int uniq_id, int obj_id, double *args, int numArgs, int rcBytes){
	char *fname = "robot";
	char *fword = "create";
	int tmp = messagePart(SR, uniq_id, 0, args, numArgs, rcBytes, fname, fword);
	return tmp;
};
void deleteRobot(SOCKET SR, int uniq_id, int obj_id, int rcBytes){
	char *fname = "delete";
	char *fword = "";
	double *tmp = 0;
	int temp = messagePart(SR, uniq_id, obj_id, tmp, 0, rcBytes, fname, fword);
};


int moveRobot(SOCKET SR, int uniq_id, int obj_id, double *args, int numArgs, int rcBytes){
	char *fname = "robot";
	char *fword = "move";
	int tmp = messagePart(SR, uniq_id, obj_id, args, numArgs, rcBytes, fname, fword);
	return tmp;
};
int changeRobotColor(SOCKET SR, int obj_id, int uniq_id, double *args, int numArgs, int rcBytes){
	char *fname = "robot";
	char *fword = "color";
	int tmp = messagePart(SR, uniq_id, obj_id, args, numArgs, rcBytes, fname, fword);
	return tmp;
};

int reqRobotX(SOCKET SR, int uniq_id, int obj_id, double *args, int numArgs, int rcBytes){
	char *fname = "robot";
	char *fword = "coords";

	// создадим сообщение
	char *sndstr = crtSendSrting(uniq_id, fname, fword, obj_id, args, numArgs);
	char *rec;// = new char[rcBytes];
	rec = NULL;
	rec = sendAndRec(SR, sndstr, rcBytes); // получили сообщение
	int tmp = extractX(rec);
	//delete[] rec;
	return tmp;
};
int reqRobotY(SOCKET SR, int uniq_id, int obj_id, double *args, int numArgs, int rcBytes){
	char *fname = "robot";
	char *fword = "coords";

	// создадим сообщение
	char *sndstr = crtSendSrting(uniq_id, fname, fword, obj_id, args, numArgs);
	char *rec;// = new char[rcBytes];
	rec = NULL;
	rec = sendAndRec(SR, sndstr, rcBytes); // получили сообщение
	int tmp = extractY(rec);
	//delete[] rec;
	return tmp;
};


// дл€ create,move,ch_color , createWorld, destroy, delete_obj
int universalRobotMessage(SOCKET SR, int uniq_id, int obj_id, double *args, int numArgs, int rcBytes, char *fname, char *fword)
{
	int tmp = messagePart(SR, uniq_id, obj_id, args, numArgs, rcBytes, fname, fword);
	return tmp;
}