#include "the_room.h"

#include "application.h"

#include "camera.h"
#include "scene_manager.h"

#include "d3d_window.h"
#include "d3d_manager.h"

#include "resource.h"

the_room::the_room() {
	m_box_texture = NULL;
	m_floor_texture = NULL;
	m_floor_vertex_buffer = NULL;
}
bool the_room::init(){

	/* allocate floor vertex buffer */
	application_throw_hr(_api_manager->m_d3ddevice->CreateVertexBuffer(
		6 * sizeof(_vertex),
		D3DUSAGE_WRITEONLY,0, D3DPOOL_MANAGED,	&(m_floor_vertex_buffer), 0));
	if(!m_floor_vertex_buffer){ application_throw("vertex buffer"); }
	/*************************************************************************/

	/* generate floor vertex buffer */
	_vertex * v = 0;
	application_throw_hr(m_floor_vertex_buffer->Lock(0, 0, (void**)&v, 0));
	m_plane_size = 128.0f;
	v[0].m_vertex.x =  m_plane_size;
	v[0].m_vertex.z =  m_plane_size;
	v[0].m_normal.y = 1.0f;
	v[0].m_uv.x=0.0f;
	v[0].m_uv.y=-1.0f;
	/*1*/
	v[1].m_vertex.x = -m_plane_size;
	v[1].m_vertex.z =  m_plane_size;
	v[1].m_normal.y = 1.0f;
	v[1].m_uv.x=1.0f;
	v[1].m_uv.y=-1.0f;
	/*2*/
	v[2].m_vertex.x =  m_plane_size;
	v[2].m_vertex.z = -m_plane_size;
	v[2].m_normal.y = 1.0f;
	v[2].m_uv.x=0.0f;
	v[2].m_uv.y=0.0f;
	/*3*/
	v[3].m_vertex.x =  m_plane_size;
	v[3].m_vertex.z = -m_plane_size;
	v[3].m_normal.y = 1.0f;
	v[3].m_uv.x=0.0f;
	v[3].m_uv.y=0.0f;
	/*3*/
	v[4].m_vertex.x =  -m_plane_size;
	v[4].m_vertex.z =   m_plane_size;
	v[4].m_normal.y = 1.0f;
	v[4].m_uv.x=1.0f;
	v[4].m_uv.y=-1.0f;
	/*2*/
	v[5].m_vertex.x =  -m_plane_size;
	v[5].m_vertex.z =  -m_plane_size;
	v[5].m_normal.y = 1.0f;
	v[5].m_uv.x=1.0f;
	v[5].m_uv.y=0.0f;
	/*4*/
	application_throw_hr(m_floor_vertex_buffer->Unlock());
	/*************************************************************************/

	/* load data from resource */
	application_throw_hr( D3DXCreateTextureFromResource( _api_manager->m_d3ddevice, NULL, MAKEINTRESOURCE(IDB_FLOOR_UV), &m_floor_texture) );
	application_throw_hr( D3DXCreateTextureFromResource( _api_manager->m_d3ddevice, NULL, MAKEINTRESOURCE(IDB_BOX_UV), &m_box_texture) );
	if(!_scene_manager->loadmesh( &m_cube_mesh   , IDR_CUBE   )){ return false; }
	if(!_scene_manager->loadmesh( &m_sphere_mesh , IDR_SPHERE )){ return false; }
	/*************************************************************************/

	return true;

}
void the_room::clear(){

	application_releasecom(m_box_texture);
	application_releasecom(m_floor_texture);
	application_releasecom(m_floor_vertex_buffer);

	application_releasecom(m_cube_mesh.m_submeshes[0].m_index_buffer);
	application_releasecom(m_cube_mesh.m_submeshes[0].m_vertex_buffer);
	application_releasecom(m_sphere_mesh.m_submeshes[0].m_index_buffer);
	application_releasecom(m_sphere_mesh.m_submeshes[0].m_vertex_buffer);

}
bool the_room::update(){

	m_model_view = _camera_view *_camera_projection;

	application_error_hr(_fx->SetTechnique(_api_manager->m_htech_floor) );

	/* floor plane */
	application_throw_hr(_fx->SetTexture(_api_manager->m_htex, m_floor_texture));

	application_throw_hr(_fx->SetMatrix(_api_manager->m_hworld, (D3DXMATRIX*)&m_model));
	m_model = _camera_view * _camera_projection;
	application_throw_hr(_fx->SetMatrix(_api_manager->m_hmvp,(D3DXMATRIX*)&m_model ));

	application_throw_hr(_api_manager->m_d3ddevice->SetVertexDeclaration(_api_manager->m_object_vertex_declaration));

	application_throw_hr(_api_manager->m_d3ddevice->SetStreamSource(0,m_floor_vertex_buffer, 0, sizeof(_vertex)));

	application_throw_hr( _fx->SetValue(_api_manager->m_hcolor, (D3DXCOLOR*)(&_vec4(1.0f,1.0f,1.0f,1.0f)), sizeof(D3DXCOLOR) ) );

	application_throw_hr(_fx->Begin(NULL, 0));
	application_throw_hr(_fx->BeginPass(0));
	application_throw_hr(_api_manager->m_d3ddevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0,2));
	application_throw_hr(_fx->EndPass());
	application_throw_hr(_fx->End()); 
	/*******************************************************************************************/

	application_error_hr(_fx->SetTechnique(_api_manager->m_htech_object) );

	_mat4 nmodel;
	///*front wall frame*/
	nmodel = _translate(_vec3(0.0f,0.0f,127.5f) );
	m_model  = _scale(_vec3(256.0f,0.5f,1.0f)) *nmodel;
	drawcube( _vec4(1.0f,0.8f,0.4f,1.0f) );
	///*back wall frame*/
	nmodel = _translate(_vec3(0.0f,0.0f,-127.5f) );
	m_model      = _scale(_vec3(256.0f,0.5f,1.0f)) *nmodel;
	drawcube( _vec4(1.0f,0.8f,0.4f,1.0f) );
	///*left wall frame*/
	nmodel = _translate(_vec3(-127.5f,0.0f,0.0f) );
	m_model       = _scale(_vec3(1.0f,0.5f,256.0f)) *nmodel;
	drawcube( _vec4(1.0f,0.8f,0.4f,1.0f) );
	///*right wall frame*/
	nmodel = _translate(_vec3(127.5f,0.0f,0.0f) );
	m_model       = _scale(_vec3(1.0f,0.5f,256.0f)) *nmodel;
	drawcube( _vec4(1.0f,0.8f,0.4f,1.0f) );

	application_error_hr(_fx->SetTechnique(_api_manager->m_htech_object_uv) );
	application_throw_hr(_fx->SetTexture(_api_manager->m_htex, m_box_texture));

	/* draw boxes *****************************************************************************/
	for (box *box_ = _scene_manager->m_box_data; box_ < _scene_manager->m_box_data+box_count; box_++) {

		/* box *****************************************************************************/
		_vec3 scale = _vec3(box_->m_half_size.x*2, box_->m_half_size.y*2, box_->m_half_size.z*2);
		m_nmodel =  box_->gettransform();
		m_model  =  _scale(scale) * box_->gettransform();
		if( (box_!= &(_485_bounding_box))  ){
			drawcube(box::s_box_colors[ uint32_t(box_-_scene_manager->m_box_data) ] );
			/***********************************************************************************/
		}
	}
	/*******************************************************************************************/

	/* draw ammo particles */
	for (ammo_round *shot = _scene_manager->m_ammo; shot < _scene_manager->m_ammo+_scene_manager->m_ammo_rounds; shot++) {
		if (shot->m_type != UNUSED) {

			/*round************************************************************/
			_vec3 scale = _vec3(shot->m_radius*2, shot->m_radius*2, shot->m_radius*2);
			m_model =  _scale( scale ) * _translate( shot->m_body->getposition() );
			m_model = _translate( shot->m_body->getposition() );
			drawsphere( _vec4(1.0f,0.0f,0.0f,0.4f ) );
		}
	}

	application_error_hr(_fx->SetTechnique(_api_manager->m_htech_object) );

	return true;
}

bool the_room::drawcube(const _vec4 & color){


	m_model = m_model*m_model_view;

	application_throw_hr(_fx->SetMatrix(_api_manager->m_hmvp,   (D3DXMATRIX*)&m_model  ));
	application_throw_hr(_fx->SetMatrix(_api_manager->m_hworld, (D3DXMATRIX*)&m_nmodel ));

	application_throw_hr(_api_manager->m_d3ddevice->SetVertexDeclaration(_api_manager->m_object_vertex_uv_declaration));

	application_throw_hr(_api_manager->m_d3ddevice->SetStreamSource(0, m_cube_mesh.m_submeshes[0].m_vertex_buffer, 0, sizeof(_vertex)));
	application_throw_hr(_api_manager->m_d3ddevice->SetIndices(m_cube_mesh.m_submeshes[0].m_index_buffer));


	application_throw_hr( _fx->SetValue(_api_manager->m_hcolor, (D3DXCOLOR*)(&color), sizeof(D3DXCOLOR) ) );

	application_throw_hr(_fx->Begin(NULL, 0));
	application_throw_hr(_fx->BeginPass(0));
	application_throw_hr(_api_manager->m_d3ddevice->DrawIndexedPrimitive(
		D3DPT_TRIANGLELIST, 0, 0,
		m_cube_mesh.m_submeshes[0].m_vertices.m_count, 0,
		m_cube_mesh.m_submeshes[0].m_indices.m_count/3));
	application_throw_hr(_fx->EndPass());
	application_throw_hr(_fx->End()); 


	return true; 

}
bool the_room::drawsphere(const _vec4 & color){

	m_model = m_model*m_model_view;

	application_throw_hr(_fx->SetMatrix(_api_manager->m_hmvp,(D3DXMATRIX*)&m_model ));
	application_throw_hr(_fx->SetMatrix(_api_manager->m_hworld, (D3DXMATRIX*)&m_nmodel));

	application_throw_hr(_api_manager->m_d3ddevice->SetVertexDeclaration(_api_manager->m_object_vertex_uv_declaration));


	application_throw_hr(_api_manager->m_d3ddevice->SetStreamSource(0, m_sphere_mesh.m_submeshes[0].m_vertex_buffer, 0, sizeof(_vertex)));
	application_throw_hr(_api_manager->m_d3ddevice->SetIndices(m_sphere_mesh.m_submeshes[0].m_index_buffer));

	application_throw_hr( _fx->SetValue(_api_manager->m_hcolor, (D3DXCOLOR*)(&color), sizeof(D3DXCOLOR) ) );

	application_throw_hr(_fx->Begin(NULL, 0));
	application_throw_hr(_fx->BeginPass(0));
	application_throw_hr(_api_manager->m_d3ddevice->DrawIndexedPrimitive(
		D3DPT_TRIANGLELIST, 0, 0,
		m_sphere_mesh.m_submeshes[0].m_vertices.m_count, 0,
		m_sphere_mesh.m_submeshes[0].m_indices.m_count/3));
	application_throw_hr(_fx->EndPass());
	application_throw_hr(_fx->End());

	return true;

}