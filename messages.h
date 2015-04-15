/*
* File: messages.h
* Author: m79lol, iskinmike
*
*/

void initConnection(int Port, std::string IP);
void closeSocketConnection();

double extractString(std::string str, char first, char second);


double extractObj_id(std::string str);
double extractX(std::string str);
double extractY(std::string str);

void testSuccess(char *str);
std::string createMessage(std::string params);

void initWorld(int x, int y, int z);
void destroyWorld();

double createRobot(int x, int y, int d_x, int d_y, int d_z, int color);
void deleteRobot(int obj_id);

void moveRobot(int obj_id, int x, int y, int speed);
void colorRobot(int obj_id, int color);

double coordsRobotX(int obj_id);
double coordsRobotY(int obj_id);