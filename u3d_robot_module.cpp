/*
* File: u3drobot_module.cpp
* Author: m79lol, iskinmike
*
*/

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdlib.h>
#include <string>
#include <vector>
#include <stdarg.h>

#include <boost/thread.hpp>

#include "module.h"
#include "robot_module.h"
#include "u3d_robot_module.h"

#include "messages_functions.h"

/////////
const unsigned int COUNT_u3dRobot_FUNCTIONS = 10;
const unsigned int COUNT_AXIS = 0;
//From messages_functions.cpp
extern bool *is_world_initialized;
extern bool is_read_from_shared_memory;
extern boost::mutex *box_mutex;

void testHold(int _hold) {
  switch (_hold) {
    case 0:
    case 1: {
      break;
    }
    default: { throw std::exception(); }
  }
}

u3dRobotModule::u3dRobotModule() {
  u3drobot_functions = new FunctionData *[COUNT_u3dRobot_FUNCTIONS];
  system_value function_id = 0;

  FunctionData::ParamTypes *Params = new FunctionData::ParamTypes[9];
  Params[0] = FunctionData::ParamTypes::FLOAT;
  Params[1] = FunctionData::ParamTypes::FLOAT;
  Params[2] = FunctionData::ParamTypes::FLOAT;
  Params[3] = FunctionData::ParamTypes::FLOAT;
  Params[4] = FunctionData::ParamTypes::FLOAT;
  Params[5] = FunctionData::ParamTypes::FLOAT;
  Params[6] = FunctionData::ParamTypes::FLOAT;
  Params[7] = FunctionData::ParamTypes::FLOAT;
  Params[8] = FunctionData::ParamTypes::STRING;

  u3drobot_functions[function_id] =
      new FunctionData(function_id + 1, 9, Params, "createCubeRobot");
  function_id++;

  Params = new FunctionData::ParamTypes[6];
  Params[0] = FunctionData::ParamTypes::FLOAT;
  Params[1] = FunctionData::ParamTypes::FLOAT;
  Params[2] = FunctionData::ParamTypes::FLOAT;
  Params[3] = FunctionData::ParamTypes::FLOAT;
  Params[4] = FunctionData::ParamTypes::FLOAT;
  Params[5] = FunctionData::ParamTypes::STRING;

  u3drobot_functions[function_id] =
      new FunctionData(function_id + 1, 6, Params, "createSphereRobot");
  function_id++;

  Params = new FunctionData::ParamTypes[10];
  Params[0] = FunctionData::ParamTypes::FLOAT;
  Params[1] = FunctionData::ParamTypes::FLOAT;
  Params[2] = FunctionData::ParamTypes::FLOAT;
  Params[3] = FunctionData::ParamTypes::FLOAT;
  Params[4] = FunctionData::ParamTypes::FLOAT;
  Params[5] = FunctionData::ParamTypes::FLOAT;
  Params[6] = FunctionData::ParamTypes::FLOAT;
  Params[7] = FunctionData::ParamTypes::FLOAT;
  Params[8] = FunctionData::ParamTypes::STRING;
  Params[9] = FunctionData::ParamTypes::STRING;

  u3drobot_functions[function_id] =
      new FunctionData(function_id + 1, 10, Params, "createModelRobot");
  function_id++;

  Params = new FunctionData::ParamTypes[1];
  Params[0] = FunctionData::ParamTypes::STRING;

  u3drobot_functions[function_id] =
      new FunctionData(function_id + 1, 1, Params, "changeColor");
  function_id++;

  Params = new FunctionData::ParamTypes[6];
  Params[0] = FunctionData::ParamTypes::FLOAT;
  Params[1] = FunctionData::ParamTypes::FLOAT;
  Params[2] = FunctionData::ParamTypes::FLOAT;
  Params[3] = FunctionData::ParamTypes::FLOAT;
  Params[4] = FunctionData::ParamTypes::FLOAT;
  Params[5] = FunctionData::ParamTypes::FLOAT;

  u3drobot_functions[function_id] =
      new FunctionData(function_id + 1, 6, Params, "move");
  function_id++;

  u3drobot_functions[function_id] =
      new FunctionData(function_id + 1, 0, NULL, "getX");
  function_id++;

  u3drobot_functions[function_id] =
      new FunctionData(function_id + 1, 0, NULL, "getY");
  function_id++;

  u3drobot_functions[function_id] =
      new FunctionData(function_id + 1, 0, NULL, "getZ");
  function_id++;

  u3drobot_functions[function_id] =
      new FunctionData(function_id + 1, 0, NULL, "getAngle");
  function_id++;

  Params = new FunctionData::ParamTypes[1];
  Params[0] = FunctionData::ParamTypes::FLOAT;

  u3drobot_functions[function_id] =
      new FunctionData(function_id + 1, 1, Params, "changeStatus");
};

void u3dRobotModule::prepare(colorPrintfModule_t *colorPrintf_p,
                             colorPrintfModuleVA_t *colorPrintfVA_p) {
  this->colorPrintf_p = colorPrintfVA_p;
}

void u3dRobot::prepare(colorPrintfRobot_t *colorPrintf_p,
                       colorPrintfRobotVA_t *colorPrintfVA_p) {
  this->colorPrintf_p = colorPrintfVA_p;
}

const char *u3dRobotModule::getUID() { return "u3dRobot_module_dll"; };

FunctionData **u3dRobotModule::getFunctions(unsigned int *count_functions) {
  *count_functions = COUNT_u3dRobot_FUNCTIONS;
  return u3drobot_functions;
}

int u3dRobotModule::init() { return 0; };

Robot *u3dRobotModule::robotRequire() {
  module_mutex.lock();
  u3dRobot *u3d_robot = new u3dRobot();
  aviable_connections.push_back(u3d_robot);

  Robot *robot = u3d_robot;
  module_mutex.unlock();
  return robot;
};

void u3dRobotModule::robotFree(Robot *robot) {
  module_mutex.lock();
  u3dRobot *u3d_robot = reinterpret_cast<u3dRobot *>(robot);
  for (m_connections::iterator i = aviable_connections.begin();
       i != aviable_connections.end(); ++i) {
    if (u3d_robot == *i) {
      if ((*i)->robot_index) {
        deleteRobot((*i)->robot_index);
      };
      delete (*i);
      aviable_connections.erase(i);
      break;
    };
  }
  module_mutex.unlock();
};

void u3dRobotModule::final() { aviable_connections.clear(); };

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

AxisData **u3dRobotModule::getAxis(unsigned int *count_axis) {
  (*count_axis) = COUNT_AXIS;
  return NULL;
};

void u3dRobot::axisControl(system_value axis_index, variable_value value){};

void *u3dRobotModule::writePC(unsigned int *buffer_length) {
  *buffer_length = 0;
  return NULL;
}

FunctionResult *u3dRobot::executeFunction(CommandMode mode,
                                          system_value functionId,
                                          void **args) {
  if ((functionId < 1) || (functionId > (int)COUNT_u3dRobot_FUNCTIONS)) {
    return NULL;
  }

  if (!is_read_from_shared_memory) {
    readSharedMemory();
  }

  try {
	  (*box_mutex).lock();
	  if (!(*is_world_initialized)) {
		  (*box_mutex).unlock();
		  throw std::exception();
	  }
	  (*box_mutex).unlock();
	  if (robot_index && (functionId == 1 || functionId == 2 || functionId == 3)) {
		  throw std::exception();
	  }
	  else if (!robot_index && (functionId != 1 && functionId != 2 && functionId != 3)) {
		  throw std::exception();
	  }

	  variable_value rez = 0;
	  switch (functionId) {
	  case 1: {  // createCubeRobot

		  variable_value *input1 = (variable_value *)args[0];
		  variable_value *input2 = (variable_value *)args[1];
		  variable_value *input3 = (variable_value *)args[2];
		  variable_value *input4 = (variable_value *)args[3];
		  variable_value *input5 = (variable_value *)args[4];
		  variable_value *input6 = (variable_value *)args[5];
		  variable_value *input7 = (variable_value *)args[6];
		  variable_value *input8 = (variable_value *)args[7];
		  testHold(*input8);
		  std::string input9((const char *)args[8]);
		  robot_index = createCube((int)*input1, (int)*input2, (int)*input3,
			  (int)*input4, (int)*input5, (int)*input6,
			  (int)*input7, (int)*input8, input9);
		  uniq_name = new char[40];
		  sprintf(uniq_name, "robot-%u", robot_index);
		  break;
	  }
	  case 2: {  // createSphereRobot

		  variable_value *input1 = (variable_value *)args[0];
		  variable_value *input2 = (variable_value *)args[1];
		  variable_value *input3 = (variable_value *)args[2];
		  variable_value *input4 = (variable_value *)args[3];
		  variable_value *input5 = (variable_value *)args[4];
		  testHold(*input5);
		  std::string input6((const char *)args[5]);
		  robot_index = createSphere((int)*input1, (int)*input2, (int)*input3,
			  (int)*input4, (int)*input5, input6);
		  uniq_name = new char[40];
		  sprintf(uniq_name, "robot-%u", robot_index);
		  break;
	  }
	  case 3: {  // createModelRobot

		  variable_value *input1 = (variable_value *)args[0];
		  variable_value *input2 = (variable_value *)args[1];
		  variable_value *input3 = (variable_value *)args[2];
		  variable_value *input4 = (variable_value *)args[3];
		  variable_value *input5 = (variable_value *)args[4];
		  variable_value *input6 = (variable_value *)args[5];
		  variable_value *input7 = (variable_value *)args[6];
		  variable_value *input8 = (variable_value *)args[7];
		  testHold(*input8);
		  std::string input9((const char *)args[8]);
		  std::string input10((const char *)args[9]);
		  robot_index = createModel((int)*input1, (int)*input2, (int)*input3,
			  (int)*input4, (int)*input5, (int)*input6,
			  (int)*input7, (int)*input8, input9, input10);
		  uniq_name = new char[40];
		  sprintf(uniq_name, "robot-%u", robot_index);
		  break;
	  }
	  case 4: {  // change Color
		  if (!robot_index) {
			  throw std::exception();
		  }
		  std::string input1((const char *)args[0]);
		  changeColor(robot_index, input1);
		  break;
	  }
	  case 5: {  // moveObject
		  if (!robot_index) {
			  throw std::exception();
		  }
		  variable_value *input1 = (variable_value *)args[0];
		  variable_value *input2 = (variable_value *)args[1];
		  variable_value *input3 = (variable_value *)args[2];
		  variable_value *input4 = (variable_value *)args[3];
		  variable_value *input5 = (variable_value *)args[4];
		  variable_value *input6 = (variable_value *)args[5];
		  moveObject(robot_index, (int)*input1, (int)*input2, (int)*input3,
			  (int)*input4, (int)*input5, (int)*input6);
		  break;
	  }
	  case 6: {  // getX
		  if (!robot_index) {
			  throw std::exception();
		  }
		  rez = getX(robot_index);
		  break;
	  }
	  case 7: {  // getY
		  if (!robot_index) {
			  throw std::exception();
		  }
		  rez = getY(robot_index);
		  break;
	  }
	  case 8: {  // getZ
		  if (!robot_index) {
			  throw std::exception();
		  }
		  rez = getZ(robot_index);
		  break;
	  }
	  case 9: {  // getAngle
		  if (!robot_index) {
			  throw std::exception();
		  }
		  rez = getAngle(robot_index);
		  break;
	  }
	  case 10: {  // changeStatus
		  if (!robot_index) {
			  throw std::exception();
		  }
		  variable_value *input1 = (variable_value *)args[0];
		  testHold(*input1);
		  changeStatus(robot_index, (int)*input1);
		  break;
	  }
	  };
	  return new FunctionResult(1, rez);
  }
  catch (...) {
	  return new FunctionResult(0);
  };
};

int u3dRobotModule::startProgram(int uniq_index) { return 0; }

void u3dRobotModule::readPC(void *buffer, unsigned int buffer_length) {}

int u3dRobotModule::endProgram(int uniq_index) { return 0; }

void u3dRobotModule::colorPrintf(ConsoleColor colors, const char *mask, ...) {
  va_list args;
  va_start(args, mask);
  (*colorPrintf_p)(this, colors, mask, args);
  va_end(args);
}

void u3dRobot::colorPrintf(ConsoleColor colors, const char *mask, ...) {
  va_list args;
  va_start(args, mask);
  (*colorPrintf_p)(this, NULL, colors, mask, args);
  va_end(args);
}

u3dRobot::~u3dRobot() {
  if (!uniq_name) {
    delete[] uniq_name;
  }
}

u3dRobot::u3dRobot() : robot_index(0) { uniq_name = NULL; };

PREFIX_FUNC_DLL RobotModule *getRobotModuleObject() {
  return new u3dRobotModule();
};
