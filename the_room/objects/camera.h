#pragma once

#include "application_header.h"

struct camera : public application_object {

	virtual bool init();
	virtual void clear(){}
	virtual bool update();

	virtual void onlostdevice(){}
	virtual void onresetdevice();
	virtual void msgproc(UINT msg, WPARAM wParam, LPARAM lParam){}

	void generate_data(float pitch, float roll);
	
    _vec3       m_up;
    _vec3       m_look;
    _vec3       m_target;
    _vec3       m_position;

	_vec3       m_aim_look;
	_vec3       m_aim_target;
	_vec3       m_aim_position;

	_mat4       m_last_view;
	_vec3       m_last_direction;
	_quaternion m_last_orientation;

	
	_quaternion m_orientation;

    _mat4       m_view;
    _mat4       m_projection;

    float       m_y_pos;
    float       m_x_pos;

	float       m_yaw;
    float       m_pitch;
    float       m_distance;

	static bool s_start;
};

