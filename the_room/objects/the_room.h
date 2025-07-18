#pragma once

#include "application_header.h"
#include "d3d_manager.h"


struct the_room : public application_object {
	the_room();

	virtual bool init();
	virtual void clear();
	virtual bool update();

	virtual void onlostdevice(){}
	virtual void onresetdevice(){}
	virtual void msgproc(UINT msg, WPARAM wParam, LPARAM lParam){}

	bool drawcube(const _vec4 & color);
	bool drawsphere(const _vec4 & color);

	_mesh m_cube_mesh;
	_mesh m_sphere_mesh;

	IDirect3DTexture9* m_box_texture;
	IDirect3DTexture9* m_floor_texture;

	IDirect3DVertexBuffer9* m_floor_vertex_buffer;
	_mesh m_cube;

	float m_plane_size;

    _mat4 m_model;
    _mat4 m_nmodel;
	_mat4 m_model_view;
	_mat4 m_model_view_projection;


};

