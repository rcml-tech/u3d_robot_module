/*
* File: lego_module.h
* Author: m79lol, iskinmike
*
*/
#ifndef VIRTUAL_ROBOT_MODULE_H
#define	VIRTUAL_ROBOT_MODULE_H
class u3dRobot : public Robot {
public:
	variable_value robot_index;
	bool isAviable;
	bool is_locked;
	bool is_Created;

	std::vector<variable_value> axis_state;
	u3dRobot(int robot_index) : robot_index(robot_index), isAviable(true), is_Created(false), is_locked(false){}
	FunctionResult* executeFunction(system_value command_index, variable_value *args);
	void axisControl(system_value axis_index, variable_value value);
	~u3dRobot() {};
};

typedef std::vector<u3dRobot*> m_connections;

class u3dRobotModule : public RobotModule{
public:
	CRITICAL_SECTION VRM_cs;
	variable_value robot_id;
	m_connections aviable_connections;
	FunctionData **u3drobot_functions;
	AxisData **robot_axis;
	colorPrintf_t *colorPrintf;
	u3dRobotModule();
	const char *getUID();
	void prepare(colorPrintf_t *colorPrintf_p, colorPrintfVA_t *colorPrintfVA_p);
	int init();
	FunctionData** getFunctions(unsigned int *count_functions);
	AxisData** getAxis(unsigned int *count_axis);
	Robot* robotRequire();
	void robotFree(Robot *robot);
	void final();
	void destroy();
	~u3dRobotModule() {};
};
#endif	/* VIRTUAL_ROBOT_MODULE_H */