/*
* File: messages.h
* Author: m79lol, iskinmike
*
*/

void initConnection(int Port, std::string IP);
void closeSocketConnection();

void initWorld(int x, int y, int z);
void destroyWorld();

int createRobot(int x, int y, int d_x, int d_y, int d_z, std::string color);
void deleteRobot(int obj_id);

void moveRobot(int obj_id, int x, int y, int speed);
void colorRobot(int obj_id, std::string color);

double coordsRobotX(int obj_id);
double coordsRobotY(int obj_id);