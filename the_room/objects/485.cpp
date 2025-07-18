#include "485.h"

#include "application.h"

#include "camera.h"
#include "physics.h"

#include "d3d_window.h"
#include "d3d_manager.h"
#include "scene_manager.h"

#include "resource.h"

object_485::object_485(){

	m_end_keyframe     = 0;
	m_start_keyframe   = 0;
	m_current_keyframe = 1;

	m_speed = 20.0f;
	m_animation_second = 0.0f;
	m_animation_length = 0.0f;

	m_texture = NULL;

}

bool object_485::init(){

	/* load mesh from rc resource */
	LPVOID data = application::getresourcedata( IDR_485 );
	if(!application::loadmeshfile(data,&m_mesh) ){ application_throw("loadmesh"); }
	application::freeresourcedata();

	if(m_mesh.m_bones.m_count){ m_keyframe_buffer.allocate(m_mesh.m_bones.m_count); }
	/* allocate vertex buffer */
	application_throw_hr(_api_manager->m_d3ddevice->CreateVertexBuffer(
		m_mesh.m_submeshes[0].m_vertices.m_count * sizeof(_vertex),
		D3DUSAGE_WRITEONLY,0, D3DPOOL_MANAGED,	&(m_mesh.m_submeshes[0].m_vertex_buffer), 0));
	if(!m_mesh.m_submeshes[0].m_vertex_buffer){ application_throw("vertex buffer"); }



	/* load the mesh data to buffers */
	_vertex * v = 0;
	application_throw_hr(m_mesh.m_submeshes[0].m_vertex_buffer->Lock(0, 0, (void**)&v, 0));
	for(uint32_t ii=0;ii<m_mesh.m_submeshes[0].m_vertices.m_count;ii++){
		v[ii] = m_mesh.m_submeshes[0].m_vertices[ii];
		//st to uv************************
		v[ii].m_uv.y = 1.0f-v[ii].m_uv.y;
		//********************************
	}
	application_throw_hr(m_mesh.m_submeshes[0].m_vertex_buffer->Unlock());

	application_throw_hr(_api_manager->m_d3ddevice->CreateIndexBuffer(
		m_mesh.m_submeshes[0].m_indices.m_count * sizeof(WORD),
		D3DUSAGE_WRITEONLY,D3DFMT_INDEX16,
		D3DPOOL_MANAGED, &(m_mesh.m_submeshes[0].m_index_buffer), 0));
	if(!m_mesh.m_submeshes[0].m_index_buffer){ application_throw("index_buffer"); }

	WORD* indices = 0;
	application_throw_hr(m_mesh.m_submeshes[0].m_index_buffer->Lock(0, 0, (void**)&(indices), 0));
	for(uint32_t ii=0;ii<m_mesh.m_submeshes[0].m_indices.m_count/3;ii++){

		//* conversion from right hand( opengl ) to left hand( direct3d ) Coordinate Systems
		//* requires clockwise rotation of triangles
		/*https://learn.microsoft.com/en-us/windows/win32/direct3d9/coordinate-systems*/
		uint32_t pos = ii*3;
		indices[pos  ] = WORD(m_mesh.m_submeshes[0].m_indices[pos]);
		indices[pos+1] = WORD(m_mesh.m_submeshes[0].m_indices[pos+2]);
		indices[pos+2] = WORD(m_mesh.m_submeshes[0].m_indices[pos+1]);

	}

	application_throw_hr(m_mesh.m_submeshes[0].m_index_buffer->Unlock());
	/***********************************************************************************/

	application_throw_hr( D3DXCreateTextureFromResource( _api_manager->m_d3ddevice, NULL, MAKEINTRESOURCE(IDB_485_UV), &m_texture) );

	addflags(object_485_fast);

	return true;
}

void object_485::clear(){
	application_releasecom(m_texture);
	application_releasecom(m_mesh.m_submeshes[0].m_index_buffer);
	application_releasecom(m_mesh.m_submeshes[0].m_vertex_buffer);
}

bool object_485::update(){

	application_throw_hr(_fx->SetTechnique(_api_manager->m_htech_blend));

	//* due to blender's up axis being Z
	m_model = _rotate(float(_radians(-90.0f)),_vec3(1.0f,0.0f,0.0f));
	//**********************************

	m_model = m_model * _485_bounding_box.m_body->gettransform();
	m_model = m_model * _translate(_vec3(0.0f,-3.5f,0.0f));
	application_throw_hr(_fx->SetMatrix(_api_manager->m_hworld, (D3DXMATRIX*)&m_model));

	m_model_view = m_model* _camera_view;

	m_model_view_projection = m_model_view *_camera_projection;
	application_throw_hr(_fx->SetMatrix(_api_manager->m_hmv,(D3DXMATRIX*)&m_model_view ));
	application_throw_hr(_fx->SetMatrix( _api_manager->m_hmvp,(D3DXMATRIX*)&m_model_view_projection ));


	application_throw_hr(_fx->SetTexture(_api_manager->m_htex, m_texture));

	application_throw_hr(_api_manager->m_d3ddevice->SetVertexDeclaration(_api_manager->m_bone_vertex_declaration));
	application_throw_hr(_fx->SetMatrixArray(_api_manager->m_hbones,currentkeyframe(), m_mesh.m_bones.m_count));

	application_throw_hr( _fx->SetValue(_api_manager->m_hcolor, (D3DXCOLOR*)(&_vec4(1.0f,1.0f,1.0f,1.0f)), sizeof(D3DXCOLOR) ) );


	application_throw_hr(_api_manager->m_d3ddevice->SetStreamSource(0, m_mesh.m_submeshes[0].m_vertex_buffer, 0, sizeof(_vertex)));
	application_throw_hr(_api_manager->m_d3ddevice->SetIndices(m_mesh.m_submeshes[0].m_index_buffer));

	application_throw_hr(_fx->Begin(NULL, 0));
	application_throw_hr(_fx->BeginPass(0));

	application_throw_hr(_api_manager->m_d3ddevice->DrawIndexedPrimitive(
		D3DPT_TRIANGLELIST, 0, 0,
		m_mesh.m_submeshes[0].m_vertices.m_count, 0,
		m_mesh.m_submeshes[0].m_indices.m_count/3));

	application_throw_hr(_fx->EndPass());
	application_throw_hr(_fx->End());

	/* skip key input when menu is showing */
	if( _scene_manager->testflags(_scene_menu) || camera::s_start ) { return true; }

	removeflags( object_485_up|object_485_down|object_485_left|object_485_right );

	if ( GetAsyncKeyState(0x57) & 0x800C ) { addflags(object_485_up); }
	if ( GetAsyncKeyState(0x53) & 0x800C ) { addflags(object_485_down);  }
	if ( GetAsyncKeyState(0x41) & 0x800C ) { addflags(object_485_left);  }
	if ( GetAsyncKeyState(0x44) & 0x800C ) { addflags(object_485_right); }


	bool keydown = testflags(object_485_up) || testflags(object_485_down) || testflags( object_485_left) || testflags(object_485_right);
	bool animation_switch = false;

	if(_scene_manager->testflags(_scene_aim) && testflags(object_485_fast) ){ animation_switch = true; }
	else{ 
		if( !_scene_manager->testflags(_scene_aim) && !testflags(object_485_fast) ){ animation_switch = true; }
	}

	if( !keydown ){
		if(m_end_keyframe >0  ){
			m_start_keyframe=0;
			m_end_keyframe=0;
		}
	}

	static _quaternion orientation;

	/* input */
	if(keydown) {
		if(m_end_keyframe == 0 ){

			keyframe(0,3);

			if( testflags(object_485_fast) ){
				m_animation_length =0.15f;
				m_speed=14.0f;
			}else{
				m_animation_length =0.2f;
				m_speed=10.0f;
			}
		}

		float added_radians = 0.0f;
		if( testflags( object_485_down ) ) { added_radians =    D3DX_PI; }
		if( testflags( object_485_left ) ) { added_radians =   (D3DX_PI/2); }
		if( testflags( object_485_right ) ){ added_radians =  -(D3DX_PI/2); }

		_vec3 direction;
		if( _scene_manager->testflags( _scene_aim ) ){

			added_radians = 0.0f;
			_vec3 look = _normalize( _vec3( _scene_manager->m_camera->m_look.x , 0.0f , _scene_manager->m_camera->m_look.z ) );

			bool left_right = false;

			if( testflags( object_485_left ) ){

				if( testflags( object_485_up ) ){
					direction = _rotate( (D3DX_PI/4.0f) , _vec3(0.0f,1.0f,0.0f) ) * look;
				}else if(  !testflags( object_485_up ) &&  testflags( object_485_down ) ){
					direction = _rotate( (D3DX_PI/3.0f) , _vec3(0.0f,1.0f,0.0f) ) * look;
				}else{
					direction = _rotate( (D3DX_PI/2.0f) , _vec3(0.0f,1.0f,0.0f) ) * look;
				}
				left_right = true;
			}
			if( testflags( object_485_right ) ){

				if( testflags( object_485_up ) ){
					direction = _rotate( -(D3DX_PI/4.0f) , _vec3(0.0f,1.0f,0.0f) ) * look;
				}else if(  !testflags( object_485_up ) &&  testflags( object_485_down ) ){
					direction = _rotate( -(D3DX_PI/3.0f) , _vec3(0.0f,1.0f,0.0f) ) * look;
				}else{
					direction = _rotate( -(D3DX_PI/2.0f) , _vec3(0.0f,1.0f,0.0f) ) * look;
				}
				left_right = true;
			}

			if( !left_right ){
				if( testflags( object_485_up )){  direction = look; }
				else if ( testflags( object_485_down )){
					direction = look;
					direction.invert();
				}
			}


		}else {
			direction = _vec3( _scene_manager->m_camera->m_look.x , 0.0f , _scene_manager->m_camera->m_look.z );
		}

		if(added_radians!=0.0f){
			direction = _rotate(added_radians,_vec3(0.0f,1.0f,0.0f))*direction;
		}

		direction.normalise();
		direction = (direction*m_speed*application_clock->m_last_frame_seconds );
		_485_bounding_box.m_body->setposition( _485_bounding_box.m_body->getposition() + direction );

		float angle = float( _radians(_scene_manager->m_camera->m_yaw)+(D3DX_PI)+added_radians );
		orientation = _quaternion(cos(angle/2.0f),0,sin(angle/2.0f),0.0f);

		/* (cos(a/2),xsin(a/2),ysin(a/2),z*sin(a/2)) -> quaternion representation */

	}else if(!keydown &&  _scene_manager->testflags(_scene_aim) ){


		float angle = float (_radians(_scene_manager->m_camera->m_yaw)+(D3DX_PI) );
		orientation = _quaternion(cos(angle/2),0,sin(angle/2),0);
	}

	_485_bounding_box.m_body->setorientation( orientation );
	_485_bounding_box.m_body->calculatederiveddata();		
	/****************************************************************************/

	/* proccess keyframes */
	/* animation_second( time elapsed ), animation_length(time between keyframes ) */
	if( ((m_end_keyframe - m_start_keyframe)>0) ){
		if(m_animation_second>=m_animation_length){
			m_animation_second=0;
			if( m_current_keyframe<m_end_keyframe ){ m_current_keyframe++; }
			else{ m_current_keyframe = m_start_keyframe; }
		}
		else { m_animation_second +=application_clock->m_last_frame_seconds; }
	}

	return true;
} 

void  object_485::keyframe(const uint32_t&start,const uint32_t&end){
	if( ( end<m_mesh.m_keyframes.m_count ) && (end>start) ){
		m_start_keyframe = m_current_keyframe = start;
		m_end_keyframe   = end;
	}
}

D3DXMATRIX*  object_485::currentkeyframe(){

	if( ((m_end_keyframe - m_start_keyframe)>0) ){

		int32_t current_key;
		int32_t previous_key;

		if(m_current_keyframe == m_end_keyframe ){
			current_key  = m_start_keyframe;
			previous_key = m_current_keyframe;
		}
		else{
			current_key  = m_current_keyframe+1;
			previous_key = m_current_keyframe;
		}

		/* linear interpolation of bone transforms */
		for(uint32_t i=0;i<m_mesh.m_bones.m_count;i++){
			float* buffer   = (float*) &( m_keyframe_buffer[i] );
			float* previous = (float*) &( m_mesh.m_keyframes[previous_key][i] );
			float* current  = (float*) &( m_mesh.m_keyframes[current_key][i] );
			for(uint32_t ii=0;ii<16;ii++){
				buffer[ii] = _lerp(previous[ii],current[ii],  m_animation_second/m_animation_length );
			}
		}
		return (D3DXMATRIX*)&(m_keyframe_buffer[0]);
	}
	return (D3DXMATRIX*)&(m_mesh.m_keyframes[0][0]);
}
