/*
* File: u3drobot_module.h
* Author: m79lol, iskinmike
*
*/
#ifndef VIRTUAL_ROBOT_MODULE_H
#define	VIRTUAL_ROBOT_MODULE_H

class u3dRobot : public Robot {
	char *uniq_name;
	colorPrintfRobotVA_t *colorPrintf_p;

	public:
		int robot_index;
	
		std::vector<variable_value> axis_state;
		u3dRobot(unsigned int uniq_index);
		void prepare(colorPrintfRobot_t *colorPrintf_p, colorPrintfRobotVA_t *colorPrintfVA_p);
		FunctionResult* executeFunction(system_value command_index, void **args);
		void axisControl(system_value axis_index, variable_value value);
		~u3dRobot();

		void colorPrintf(ConsoleColor colors, const char *mask, ...);
};

typedef std::vector<u3dRobot*> m_connections;

class u3dRobotModule : public RobotModule{
	DEFINE_ATOM(VRM_cs);

	m_connections aviable_connections;
	FunctionData **u3drobot_functions;
	AxisData **robot_axis;
	colorPrintfModuleVA_t *colorPrintf_p;

	public:
		u3dRobotModule();

		//init
		const char *getUID();
		void prepare(colorPrintfModule_t *colorPrintf_p, colorPrintfModuleVA_t *colorPrintfVA_p);

		//compiler only
		FunctionData** getFunctions(unsigned int *count_functions);
		AxisData** getAxis(unsigned int *count_axis);
		void *writePC(unsigned int *buffer_length);

		//intepreter - devices
		int init();
		Robot* robotRequire();
		void robotFree(Robot *robot);
		void final();

		//intepreter - program & lib
		void readPC(void *buffer, unsigned int buffer_length);
	
		//intepreter - program
		int startProgram(int uniq_index);
		int endProgram(int uniq_index);

		//destructor
		void destroy();
		~u3dRobotModule(){};

		void colorPrintf(ConsoleColor colors, const char *mask, ...);
};
#endif	/* VIRTUAL_ROBOT_MODULE_H */