#pragma once

#include "application_header.h"

struct scene_manager;

struct application : public application_flags {

    application();

	bool init();

	void run();
    
	void clear();

    void onlostdevice();
    void onresetdevice();

	scene_manager * m_scene_manager;

	/*cursor***************/
	float      m_x_cursor_pos;
    float      m_y_cursor_pos;
	/**********************/

	/*application global static variables *********************/
    static application *  _instance;
    static HINSTANCE      _win32_instance;
	/**********************************************************/

	/***generates a _mesh from a ._mesh resource file **************/
    static bool loadmeshfile(const LPVOID data,_mesh* submeshes,bool bones=true);
	/**********************************************************/

	static LPVOID  s_lpdata;
	static HGLOBAL s_hglobal;
	static HRSRC   s_hresource;
	static LPVOID  getresourcedata(int id);
	static void    freeresourcedata();

};