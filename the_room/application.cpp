
#include "application.h"

#include "d3d_window.h"
#include "d3d_manager.h"
#include "scene_manager.h"

#include "ui.h"
#include "ui_static.h"
#include "ui_text.h"
#include "ui_button.h"


application*  application::_instance       = NULL;
HINSTANCE     application::_win32_instance = NULL;

LPVOID  application::s_lpdata    = (LPVOID) NULL;
HGLOBAL application::s_hglobal   = (HGLOBAL)NULL;
HRSRC   application::s_hresource = (HRSRC)  NULL;


ui_static * fps_control          = NULL;
ui_static * mspf_control         = NULL;

const _vec3 _utility::up         = _vec3(0, 1, 0);
float _utility::sleepepsilon     = 0.33f;

application::application(){ m_scene_manager = NULL; }

bool application::init(){

	/*win32 HINSTANCE */
	_win32_instance = GetModuleHandle(NULL);

	_window         = new d3d_window();
	_api_manager    = new d3d_manager();

	if(!_window->init())      { return false;}
	if(!_api_manager->init()) { return false; }


	application_clock = new clock();
	application_clock->init();

	//*mouse pointer update*************************************/
	POINT cursor_position;
	if (GetCursorPos(&cursor_position)) {
		m_x_cursor_pos = float(cursor_position.x);
		m_y_cursor_pos = float(cursor_position.y);
	}else{ application_throw("cursor pos");}
	//***************************************************/

	m_scene_manager = new scene_manager();

	if(!m_scene_manager->init()){ return false; }

	return true;
}

void application::run(){

	/* when this flag is removed, the application exits */
	addflags(application_running); 
	/***************************************/

	/* to initialize the projection matrix */
	onresetdevice();

	/* clear message queue */
	_window->update();

	float coursor_second = 0.5f;

	/* main loop */
	while( testflags(application_running) ) {

		/**d3d device test.  error exits application**/
		removeflags(application_lostdev);

		// get the state of the graphics device.
		HRESULT hr = _api_manager->m_d3ddevice->TestCooperativeLevel();

		if( hr == D3DERR_DEVICELOST ) {  

			// if the device is lost and cannot be reset yet then
			// sleep for a bit and we'll try again on the next 
			// message loop cycle.

			Sleep(20);
			addflags( application_lostdev );  

		}else if( hr == D3DERR_DRIVERINTERNALERROR ) {

			// internal driver error...exit
			addflags( application_deverror );
			application_error("d3d device error");

		}else if( hr == D3DERR_DEVICENOTRESET ) {

			// The device is lost but we can reset and restore it.
			_api_manager->reset();
			addflags( application_lostdev );
		}
		/*********************************************/

		/* clock update *******************************************/
		application_clock->update();
		/**********************************************************/

		/* coursor display timer **************************************************/
		coursor_second += application_clock->m_last_frame_seconds;
		if(coursor_second >= 0.5f){
			if(testflags(application_coursor_on)){ removeflags(application_coursor_on); }
			else{ addflags(application_coursor_on); }
			coursor_second = 0.0f;
		}
		/**************************************************************************/

		if( !(testflags(application_lostdev))  && !(testflags(application_deverror))  ) {

			/*mouse pointer update*************************************/
			POINT cursor_position;
			if (GetCursorPos(&cursor_position)) {
				m_x_cursor_pos = float(cursor_position.x);
				m_y_cursor_pos = float(cursor_position.y);
			}else{ application_error("cursor pos");}
			/***************************************************/

			/* application update (render) ******************************************/
			application_error_hr(_api_manager->m_d3ddevice->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(255, 255, 255), 1.0f, 0));
			application_error_hr(_api_manager->m_d3ddevice->BeginScene());

			m_scene_manager->update(); 

			application_error_hr(_api_manager->m_d3ddevice->EndScene());
			application_error_hr(_api_manager->m_d3ddevice->Present(0, 0, 0, 0));
			/************************************************************************/
		}

		/* pause when menu shows*/
		if(_scene_manager->testflags(_scene_menu)){ _application->addflags(application_paused); }

		else { _application->removeflags(application_paused); }

		/* input update */
		if( testflags(application_deverror) ) { removeflags(application_running); }
		else { _window->update(); }/* winpoc (input) */
	}

	/* deallocate .. exiting */

	DestroyWindow(_window->m_hwnd);
	m_scene_manager->clear();
	clear();
	/**********************/
}

void application::clear(){
	if(_api_manager ) { _api_manager->clear();}
}

void application::onlostdevice() {
	if( _application->testflags(application_running) ){
		application_error_hr(_fx->OnLostDevice());
		_scene_manager->onlostdevice();
	}
}

void application::onresetdevice() {
	if( _application->testflags(application_running) ){
		application_error_hr(_fx->OnResetDevice());
		_scene_manager->onresetdevice();
	}
}

bool application::loadmeshfile(const LPVOID data,_mesh * mesh,bool bones){

	int pos = 6;
	const uint8_t * all_data = (uint8_t *)data;

	/* 6 byte string _mesh file identifier */
	char  header_[7];
	application_zero(header_,7);
	memcpy(header_,all_data,6);
	if(!application_scm(header_,"_mesh_")) { application_throw("not _mesh_ file"); }

	/* 2 byte unsinged int ( submesh count ) */
	uint16_t submesh_count_ =  *( (uint16_t*)(&all_data[pos]) );

	pos+= sizeof(uint16_t);

	/* read submeshes */
	for(uint32_t i=0;i<submesh_count_;i++){

		_submesh submesh_;
		uint32_t index_count_ = *( (uint32_t*)(&all_data[pos]) );

		/* 4 byte unsinged int ( vertex indicies count ) */
		pos+= sizeof(uint32_t);

		/* read indices 4 bytes each  */
		for(uint32_t i = 0; i<index_count_; i++){
			submesh_.m_indices.pushback( *( (uint32_t*)(&all_data[pos]) ) ,true);
			pos+= sizeof(uint32_t);
		}

		/* 4 byte unsinged int ( vertex count ) */
		uint32_t vertex_count_ = *( (uint32_t*)(&all_data[pos]) );
		pos+= sizeof(uint32_t);

		/* verticies */
		_vertex * v = (_vertex*)(&all_data[pos]);
		for(uint32_t ii=0;ii<vertex_count_;ii++) { submesh_.m_vertices.pushback( v[ii] ,true); }

		mesh->m_submeshes.pushback(submesh_,true);

		pos+= sizeof(_vertex)*vertex_count_;
	}

	if(bones) {

		/* 2 byte unsinged int (bone transform count ) */
		uint16_t bone_count = 0;
		if( (&all_data[pos]) ){ bone_count = *( (uint16_t*)(&all_data[pos]) ); }
		pos+= sizeof(uint16_t);
		
		
		if(bone_count){

			mesh->m_bones.allocate(bone_count);

			/* bone transforms  */
			_mat4* bones = ( _mat4* )(&all_data[pos]);
			for(uint16_t i = 0; i<bone_count; i++) { mesh->m_bones[i] = bones[i]; }
			pos+=sizeof(_mat4)*bone_count;

			/* 2 byte unsinged int (animation keyframe count ) */
			uint16_t keyframe_count = *( (uint16_t*)(&all_data[pos])  );
			pos+=sizeof(uint16_t);

			if( keyframe_count){
				_mat4* keyframe_bones = ( _mat4* )(&all_data[pos]);

				/* animation bone transforms  */
				uint32_t keyframe_pos =0;
				for(uint32_t ii=0;ii<keyframe_count;ii++){
					_matrix_array keyframe;
					keyframe.allocate(bone_count);
					for(uint16_t j=0;j<bone_count;j++){
						keyframe[j] = keyframe_bones[keyframe_pos+j];
					}
					keyframe_pos+=bone_count;
					mesh->m_keyframes.pushback(keyframe);
				}
			}
		}
	}
	return true;
}

LPVOID application::getresourcedata(int id){

	s_hresource   = FindResource( _win32_instance , MAKEINTRESOURCE( id ),RT_RCDATA);
	if( s_hresource == (HRSRC)NULL ){ application_error("FindResource"); return (LPVOID)NULL; }

	s_hglobal     = LoadResource(NULL, s_hresource );
	if( s_hglobal == (HGLOBAL)NULL ){ application_error("LoadResource");  return (LPVOID)NULL; }

	s_lpdata = LockResource( s_hglobal );
	if( s_lpdata == (LPVOID)NULL ){ application_error("LockResource");    return (LPVOID)NULL; }

	return s_lpdata;
}

void application::freeresourcedata(){
	if(!s_hglobal){
		UnlockResource(s_hglobal);
		FreeResource(s_hglobal);
	}
	s_lpdata = (LPVOID)NULL;
	s_hglobal = (HGLOBAL)NULL;
	s_hresource = (HRSRC)NULL;
}
