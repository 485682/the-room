#pragma once

#include "application_header.h"

struct d3d_manager {
	d3d_manager();
	bool init();
	void clear();

	bool reset();
	bool buildfx();

	ID3DXEffect* m_fx;

	D3DPRESENT_PARAMETERS m_d3dpp;

	IDirect3D9*           m_d3dobject;
	IDirect3DDevice9*     m_d3ddevice;

	IDirect3DVertexDeclaration9* m_bone_vertex_declaration;
	IDirect3DVertexDeclaration9* m_object_vertex_declaration;
	IDirect3DVertexDeclaration9* m_object_vertex_uv_declaration;
	IDirect3DVertexDeclaration9* m_ui_foreground_vertex_declaration;
	IDirect3DVertexDeclaration9* m_ui_background_vertex_declaration;


	/* effect handles*/
	D3DXHANDLE   m_htex;

	D3DXHANDLE   m_hmv;
	D3DXHANDLE   m_hmvp;
	D3DXHANDLE   m_hworld;

	D3DXHANDLE   m_hcolor;
	D3DXHANDLE   m_hbones;

	D3DXHANDLE   m_htech_blend;
	D3DXHANDLE   m_htech_object;
	D3DXHANDLE   m_htech_object_uv;
	D3DXHANDLE   m_htech_floor;
	D3DXHANDLE   m_htech_ui_foreground;
	D3DXHANDLE   m_htech_ui_background;
	/***********************************/

	static d3d_manager* _manager;
};