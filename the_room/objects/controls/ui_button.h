#pragma once
#include "application_header.h"

#include "ui.h"

struct ui_button : public ui_control {
	ui_button();
	~ui_button(){}

	virtual bool init();
	virtual void clear();
	virtual bool update();

	virtual void onlostdevice(){}
	virtual void onresetdevice(){}

	virtual void msgproc(UINT msg, WPARAM wParam, LPARAM lParam);

	void reset();

	void settext(const char* text);

	_vec4 m_alt_color;

	bool genbackgroundbuffer();
	bool genforegroundbuffer();

	_string m_string;

};