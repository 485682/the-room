#pragma once

#include "application_header.h"
#include "d3d_manager.h"

/* object 485 flags */
#define object_485_up              0x1
#define object_485_down            0x2
#define object_485_left            0x4
#define object_485_right           0x8
#define object_485_fast            0x10

struct  object_485 : public application_object {

    object_485();

    virtual bool init();
    virtual void clear();
    virtual bool update();

	virtual void onlostdevice(){}
	virtual void onresetdevice(){}
	virtual void msgproc(UINT msg, WPARAM wParam, LPARAM lParam){}

    void setbindpose(){ m_current_keyframe=m_start_keyframe=m_end_keyframe=0;}
    void keyframe(const uint32_t &start, const uint32_t &end);
    D3DXMATRIX* currentkeyframe();

    _matrix_array m_keyframe_buffer;


	/* holds speed of walk */
	float   m_speed;

	/* holds seconds accumulated */
	float   m_animation_second;

	/* holds  how many seconds each keyframe lasts */
	float   m_animation_length;


    int32_t m_end_keyframe;
    int32_t m_start_keyframe;
    int32_t m_current_keyframe;

    _mat4 m_model;
	_mat4 m_model_view;
	_mat4 m_model_view_projection;

	IDirect3DTexture9* m_texture;

    _mesh m_mesh;

};
