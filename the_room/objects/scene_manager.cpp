#include "scene_manager.h"


#include "application.h"
#include "d3d_window.h"
#include "d3d_manager.h"

#include "ui.h"
#include "ui_text.h"
#include "ui_button.h"
#include "ui_static.h"

#include "485.h"
#include "the_room.h"

#include "camera.h"

#include "time.h"

_vec4 * box::s_box_colors = NULL;

void ammo_round::setstate() {

	m_type = FIREING;

	
	m_body->setmass(40.0f); 


	_vec3 v = _scene_manager->m_camera->m_aim_look;

	m_body->setvelocity(v*100.0f); 
	m_body->setacceleration(0.0f, 0.0f, 0.0f); // no gravity
	m_body->setdamping(0.99f, 0.8f);
	m_radius = 0.2f;

	m_body->setcansleep(false);
	m_body->setawake();

	_mat3 tensor;
	float coeff = 0.4f*m_body->getmass()*m_radius*m_radius;
	tensor.setinertiatensorcoeffs(coeff,coeff,coeff);
	m_body->setinertiatensor(tensor);

	m_body->setposition( _scene_manager->m_camera->m_aim_position + (v*5.5f) );
	m_update_time = 0;/* count in seconds*/

	// clear the force accumulators
	m_body->calculatederiveddata();
	calculateinternals();
}

scene_manager::scene_manager() : m_resolver(max_contacts*8,0.02f,0.02f) {

	m_485           = NULL;
	m_the_room      = NULL;

	m_ui            = NULL;
	m_cross_hair_1  = NULL;
	m_cross_hair_2  = NULL;
	m_fps_control   = NULL;
	m_directions    = NULL;
	m_continue      = NULL;
	m_fullscreen    = NULL;
	m_vsync         = NULL;
	m_exit          = NULL;

	m_camera        = NULL;
}

bool scene_manager::loadmesh( _mesh * mesh, int id){

	/* load mesh data*/
	LPVOID data = application::getresourcedata( id );
	if(!application::loadmeshfile(data,mesh,false) ){ application_throw("readmesh"); }
	application::freeresourcedata();

	/* get vertex buffer pointer */
	application_throw_hr(_api_manager->m_d3ddevice->CreateVertexBuffer(
		mesh->m_submeshes[0].m_vertices.m_count * sizeof(_vertex),
		D3DUSAGE_WRITEONLY,0, D3DPOOL_MANAGED,	
		&(mesh->m_submeshes[0].m_vertex_buffer), 0));

	if(!mesh->m_submeshes[0].m_vertex_buffer){ application_throw("vertex buffer"); }

	_vertex * v = 0;
	/* lock vertex buffer and write */
	application_throw_hr(mesh->m_submeshes[0].m_vertex_buffer->Lock(0, 0, (void**)&v, 0));
	for(uint32_t ii=0;ii<mesh->m_submeshes[0].m_vertices.m_count;ii++){
		v[ii] = mesh->m_submeshes[0].m_vertices[ii];
		//st to uv************************
		v[ii].m_uv.y = 1.0f-v[ii].m_uv.y;
		//********************************
	}
	application_throw_hr(mesh->m_submeshes[0].m_vertex_buffer->Unlock());

	application_throw_hr(_api_manager->m_d3ddevice->CreateIndexBuffer(
		mesh->m_submeshes[0].m_indices.m_count * sizeof(WORD),
		D3DUSAGE_WRITEONLY,D3DFMT_INDEX16,
		D3DPOOL_MANAGED, &(mesh->m_submeshes[0].m_index_buffer), 0));
	if(!mesh->m_submeshes[0].m_index_buffer){ application_throw("index_buffer"); }

	WORD* indices = 0;
	application_throw_hr(mesh->m_submeshes[0].m_index_buffer->Lock(0, 0, (void**)&(indices), 0));
	for(uint32_t ii=0;ii<mesh->m_submeshes[0].m_indices.m_count/3;ii++){

		uint32_t pos = ii*3;
		//* conversion from right hand( opengl ) to left hand( direct3d ) Coordinate Systems
		//* requires clockwise rotation of triangles
		/*https://learn.microsoft.com/en-us/windows/win32/direct3d9/coordinate-systems*/
		indices[pos  ] = WORD(mesh->m_submeshes[0].m_indices[pos]);
		indices[pos+1] = WORD(mesh->m_submeshes[0].m_indices[pos+2]);
		indices[pos+2] = WORD(mesh->m_submeshes[0].m_indices[pos+1]);
	}
	application_throw_hr(mesh->m_submeshes[0].m_index_buffer->Unlock());

	return true;
}

bool scene_manager::init(){

	time_t rand_time;
	time( &rand_time );

	box::s_box_colors = new _vec4[box_count];
	random random_;
	random_.seed( ((int64_t)rand_time) % 10 );
	for(int i=0; i<box_count; i++){ 
		box::s_box_colors[i] = _vec4(
			random_.randomreal(0.5f,1.0f),
			random_.randomreal(0.3f,0.7f),
			random_.randomreal(0.1f,0.6f),1.0f);
	}


	/*skeletan animation object (character)*/
	m_485 = new object_485();
	m_object_array.pushback( (application_object*)m_485);

	/* the rest of the objects */
	m_the_room = new the_room();
	m_object_array.pushback( (application_object*)m_the_room);

	m_485->setbindpose();


	m_ui = new ui();

	m_cross_hair_1 = new ui_static();
	m_cross_hair_1->m_background_color = _vec4(0.0f,0.0f,1.0f,0.8f);
	m_ui->addcontrol(m_cross_hair_1);
	m_cross_hair_2 = new ui_static();
	m_cross_hair_2->m_background_color = _vec4(0.0f,0.0f,1.0f,0.8f);
	m_ui->addcontrol(m_cross_hair_2);

	m_fps_control = new ui_static();
	m_fps_control->m_background_color = _vec4(0.0f,0.0f,0.0f,0.0f);
	m_fps_control->m_foreground_color = _vec4(0.0f,0.0f,1.0f,1.0f);

	m_fps_control->m_width  = _api_manager->m_d3dpp.BackBufferWidth/2.0f;
	m_fps_control->m_height = _api_manager->m_d3dpp.BackBufferHeight/10.0f;
	m_ui->addcontrol(m_fps_control);

	m_directions = new ui_text();
	m_directions->m_background_color = _vec4(0.1f,0.1f,0.1f,0.8f);
	m_directions->m_foreground_color = _vec4(1.0f,1.0f,1.0f,1.0f);
	m_ui->addcontrol(m_directions);		
	m_directions->settext(
		"\n"
		" W,A,S,D         : movement keys \n"
		" MouseMove       : camera \n"
		" Q               : toggle aim mode \n"
		" LeftMouseButton : fire (only in aim mode)\n                   hold down for repeat fire\n"
		" ESC             : menu \n"
		);
	m_directions->addflags(ui_disable);

	m_continue = new ui_button();
	m_continue->m_background_color = _vec4(0.0f,0.0f,1.0f,0.5f);
	m_continue->m_foreground_color = _vec4(1.0f,1.0f,1.0f,1.0f);
	m_continue->m_alt_color = _vec4(0.0f,0.0f,0.4f,0.2f);

	m_ui->addcontrol(m_continue);
	m_continue->settext("start");


	m_vsync = new ui_button();
	m_vsync->m_background_color = _vec4(0.0f,0.0f,0.2f,0.5f);
	m_vsync->m_foreground_color = _vec4(1.0f,1.0f,1.0f,1.0f);
	m_vsync->m_alt_color = _vec4(0.0f,0.0f,0.4f,0.2f);

	m_ui->addcontrol(m_vsync);
	m_vsync->settext("vsync");

	m_fullscreen = new ui_button();
	m_fullscreen->m_background_color = _vec4(0.0f,0.0f,0.2f,0.5f);
	m_fullscreen->m_foreground_color = _vec4(1.0f,1.0f,1.0f,1.0f);
	m_fullscreen->m_alt_color = _vec4(0.0f,0.0f,0.4f,0.2f);

	m_ui->addcontrol(m_fullscreen);
	m_fullscreen->settext("fullscreen");

	m_exit = new ui_button();
	m_exit->m_background_color = _vec4(1.0f,0.0f,0.0f,0.5f);
	m_exit->m_foreground_color = _vec4(1.0f,1.0f,1.0f,1.0f);
	m_exit->m_alt_color = _vec4(0.0f,0.0f,0.4f,0.2f);

	m_ui->addcontrol(m_exit);
	m_exit->settext("Exit");

	if(!m_ui->init()){ return false; }

	for( uint32_t i=0;i<m_object_array.m_count;i++){ 
		if(!m_object_array[i]->init()){ return false; } 
	}	

	m_camera = new camera();
	if(!m_camera->init()){ return false; }

	m_cdata.m_contact_array = m_contacts;

	/* setup character bounding box */
	_485_bounding_box.setstate( _vec3(0.0f,0.0f,0.0f), _quaternion(), _vec3(1.5f,3.5f,1.5f), _vec3(0.0f,0.0f,0.0f) );
	_485_bounding_box.m_body->setposition(_vec3(0.0f,0.0f,0.0f));
	_485_bounding_box.m_body->setawake(false);
	/********************************/

	/* setup  boxes  */
	float y_pos = 0.0f;
	int   current_box = 1;
	m_box_data[ current_box++ ].setstate( _vec3(75.0f,0.0f,0.0f), _quaternion(), _vec3(5.0f,2.0f,10.0f), _vec3(0.0f,1.0f,0.0f) );
	while( current_box < (box_count/2) ){
		m_box_data[ current_box++ ].setstate( _vec3(75.0f,5.0f,y_pos), _quaternion(), _vec3(1.0f,2.0f,1.0f), _vec3(0.0f,1.0f,0.0f) );
		y_pos += 5.0f;
	}

	m_box_data[ current_box++ ].setstate( _vec3(-75.0f,0.0f,0.0f), _quaternion(), _vec3(5.0f,2.0f,10.0f), _vec3(0.0f,1.0f,0.0f) );
	y_pos = 0.0f;
	while( current_box < box_count ){
		m_box_data[ current_box++ ].setstate( _vec3(-75.0f,5.0f,y_pos), _quaternion(), _vec3(1.0f,2.0f,1.0f), _vec3(0.0f,1.0f,0.0f) );
		y_pos += 5.0f;
	}
	/******************************************/


	// reset the contacts
	m_cdata.m_contact_count = 0;

	/* set all rounds to unused*/
	for (ammo_round *shot = m_ammo; shot < m_ammo+m_ammo_rounds; shot++) { shot->m_type = UNUSED; }

	/* show menu */
	addflags(_scene_menu);

	return true;
}
void scene_manager::clear(){

	for( uint32_t i=0;i<m_object_array.m_count;i++){ 
		m_object_array[i]->clear();
		delete m_object_array[i];
	}	
	m_485 = NULL;
	m_the_room = NULL;

	m_ui->clear();

	delete m_ui;           m_ui=NULL;          

	delete m_cross_hair_1; m_cross_hair_1=NULL;
	delete m_cross_hair_2; m_cross_hair_2=NULL;
	delete m_fps_control;  m_fps_control=NULL; 
	delete m_directions;   m_directions=NULL;  
	delete m_continue;     m_continue=NULL;    
	delete m_vsync;        m_vsync=NULL;       
	delete m_fullscreen;   m_fullscreen=NULL;  
	delete m_exit;         m_exit=NULL;        

	delete m_camera;       m_camera = NULL;


	delete[] box::s_box_colors; box::s_box_colors = NULL;

}
bool scene_manager::update(){

	static float second = 1.0f;
	static float round_time = 0.0f;

	/* test to see if rounds are to be fired */
	if ( GetAsyncKeyState(VK_LBUTTON) & 0x800C  && (round_time<=0) ) { 

		if( _scene_manager->testflags(_scene_aim) && ! _scene_manager->testflags(_scene_menu) ){
			ammo_round *shot;
			for (shot = _scene_manager->m_ammo; shot < _scene_manager->m_ammo+_scene_manager->m_ammo_rounds; shot++) {
				if (shot->m_type == UNUSED) { break; }
			}

			// if we didn't find a round, then exit - we can't fire.
			if (shot < _scene_manager->m_ammo+_scene_manager->m_ammo_rounds) { 
				// set the shot
				shot->setstate();
				round_time =0.1f;/* in seconds */
			}
		}
	}
	if(round_time>0.0f){ round_time-= application_clock->m_last_frame_seconds; }


	float duration = application_clock->m_last_frame_seconds;

	/* increment second */
	second += duration;

	/*stat string generation **************************************************/
	static _string state;
	if(second >= 1.0f) {
		/* update per second*/
		_string  stats =_string(" fps : ")+ _utility::floattostring(application_clock->m_fps);
		stats  = stats +_string(" mspf: ");
		stats  = stats +_utility::floattostring( application_clock->m_last_frame_milliseconds ,true);
		m_fps_control->settext(stats.m_data);
		second = 0.0f;
	}
	/**************************************************************************/

	if ( !_application->testflags(application_paused) && (duration > 0.0f)) {

		if (duration > 0.05f) { duration = 0.05f; }

		// update the objects
		updateobjects(duration);

		// perform the contact generation
		generatecontacts();

		// resolve detected contacts
		m_resolver.resolvecontacts(
			m_cdata.m_contact_array,
			m_cdata.m_contact_count,
			duration
			);
	}


	/* update ui controls */
	float height = float( _api_manager->m_d3dpp.BackBufferHeight );
	float width = float( _api_manager->m_d3dpp.BackBufferWidth );


	m_fps_control->m_width  = width/2.0f;
	m_fps_control->m_height = height/10.0f;

	m_cross_hair_1->m_width  = 10.0f;
	m_cross_hair_1->m_height = 1.0f;
	m_cross_hair_1->m_x      = (width/2.0f)-5.0f;
	m_cross_hair_1->m_y      = (height/2.0f);
	m_cross_hair_1->reset();

	m_cross_hair_2->m_width  =  1.0f;
	m_cross_hair_2->m_height =  10.0f;
	m_cross_hair_2->m_x      = (width/2.0f);
	m_cross_hair_2->m_y      = (height/2.0f)-5.0f;
	m_cross_hair_2->reset();


	m_directions->m_width  = (width/2.0f);
	m_directions->m_height = (height/8.0f)*3;
	m_directions->m_x = width/4.0f;
	m_directions->m_y = (height/4.0f);
	m_directions->reset();

	float pad = (width/8.0f);
	float button_height = (height/8.0f);

	m_continue->m_width  = (width/8.0f);
	m_continue->m_height = button_height;
	m_continue->m_x = pad*2;
	m_continue->m_y = (height/2.0f) + button_height;
	m_continue->reset();

	m_fullscreen->m_width  = (width/8.0f);
	m_fullscreen->m_height = button_height;
	m_fullscreen->m_x = pad*3;
	m_fullscreen->m_y = (height/2.0f) + button_height;
	m_fullscreen->reset();

	m_vsync->m_width  = (width/8.0f);
	m_vsync->m_height = button_height;
	m_vsync->m_x = pad*4;
	m_vsync->m_y = (height/2.0f) + button_height;
	m_vsync->reset();

	m_exit->m_width  = (width/8.0f);
	m_exit->m_height = button_height;
	m_exit->m_x = pad*5;
	m_exit->m_y = (height/2.0f) + button_height;
	m_exit->reset();
	/*************************************************/



	m_camera->update();
	for( uint32_t i=0;i<m_object_array.m_count;i++){ m_object_array[i]->update(); }	
	m_ui->update();


	/* remove motion from character bounding box */
	_485_bounding_box.m_body->setvelocity( _vec3( 0.0f,0.0f,0.0f) );
	_485_bounding_box.m_body->setrotation( _vec3( 0.0f,0.0f,0.0f) );

	_vec3 pos = _485_bounding_box.m_body->getposition();
	_485_bounding_box.m_body->setposition(_vec3(pos.x,3.5f,pos.z));
	/*******************************************************************/


	return true;
}
void scene_manager::onlostdevice() {
	m_camera->onlostdevice();
	for( uint32_t i=0;i<m_object_array.m_count;i++){ 
		if( m_object_array[i] ) { m_object_array[i]->onlostdevice(); }
	}	
	m_ui->onlostdevice();
}

void scene_manager::onresetdevice() {
	m_camera->onresetdevice();
	for( uint32_t i=0;i<m_object_array.m_count;i++){ 
		if( m_object_array[i] ) {m_object_array[i]->onresetdevice();}
	}
	m_ui->onresetdevice();
}

void scene_manager::msgproc(UINT msg, WPARAM wParam, LPARAM lParam){

	if(m_camera){ m_camera->msgproc(msg,wParam,lParam); }
	for( uint32_t i=0;i<m_object_array.m_count;i++){ 
		if( m_object_array[i] ) { m_object_array[i]->msgproc(msg,wParam,lParam); }
	}
	for( uint32_t i=0;i<m_ui->m_controls.m_count;i++){ 
		m_ui->m_controls[i]->msgproc(msg,wParam,lParam);
	}


	switch( msg )
	{
	case WM_KEYDOWN:{

		if( wParam == 0x51 ){
			_scene_manager->m_485->m_start_keyframe=0;
			_scene_manager->m_485->m_end_keyframe=0;
			if( !testflags(_scene_aim) ){ /* toggle between aim mode */
				addflags(_scene_aim);
				_scene_manager->m_485->removeflags(object_485_fast);
			}else { 
				removeflags(_scene_aim); 
				_scene_manager->m_485->addflags(object_485_fast);
			}

		}
					}
	}
}

void scene_manager::generatecontacts() {

	// note that this method makes a lot of use of early returns to avoid
	// processing lots of potential contacts that it hasn't got room to
	// store.

	// create the ground plane data
	collision_plane floor_plane;
	floor_plane.m_direction = _vec3(0,1,0);
	floor_plane.m_offset = 0;

	// create wall plane data
	collision_plane front_plane;
	front_plane.m_direction = _vec3(0,0,-1);
	front_plane.m_offset = -128;

	collision_plane back_plane;
	back_plane.m_direction = _vec3(0,0,1);
	back_plane.m_offset = -128;

	collision_plane left_plane;
	left_plane.m_direction = _vec3(1,0,0);
	left_plane.m_offset = -128;

	collision_plane right_plane;
	right_plane.m_direction = _vec3(-1,0,0);
	right_plane.m_offset = -128;

	// set up the collision data structure
	m_cdata.reset(max_contacts);
	m_cdata.m_friction    = 0.4f;
	m_cdata.m_restitution = 1.0f;

	for (ammo_round *shot = m_ammo; shot < m_ammo+m_ammo_rounds; shot++) {
		if (shot->m_type != UNUSED){

			if (!m_cdata.hasmorecontacts()) { return; }

			if (collision_detector::sphereandhalfspace(*shot, floor_plane, &m_cdata) ||
				collision_detector::sphereandhalfspace(*shot, front_plane, &m_cdata) ||
				collision_detector::sphereandhalfspace(*shot, back_plane, &m_cdata)  ||
				collision_detector::sphereandhalfspace(*shot, left_plane, &m_cdata)  ||
				collision_detector::sphereandhalfspace(*shot, right_plane, &m_cdata) ){
					shot->m_type = UNUSED;
			}
		}
	}


	// perform exhaustive collision detection
	_mat4 transform, othertransform;
	_vec3 position, otherposition;
	for (box *box_ = m_box_data; box_ < m_box_data+box_count; box_++) {

		if (!m_cdata.hasmorecontacts()) { return; }

		// check for collisions with the ground and wall planes
		collision_detector::boxandhalfspace(*box_, floor_plane, &m_cdata);
		collision_detector::boxandhalfspace(*box_, front_plane, &m_cdata);
		collision_detector::boxandhalfspace(*box_, back_plane, &m_cdata);
		collision_detector::boxandhalfspace(*box_, left_plane, &m_cdata);
		collision_detector::boxandhalfspace(*box_, right_plane, &m_cdata);

		// check for collisions with each shot
		for (ammo_round *shot = m_ammo; shot < m_ammo+m_ammo_rounds; shot++) {
			if (shot->m_type != UNUSED){

				if (!m_cdata.hasmorecontacts()) { return; }

				// when we get a collision, remove the shot
				if ( collision_detector::boxandsphere(*box_, *shot, &m_cdata) ){
					shot->m_type = UNUSED;
				}
			}
		}

		// check for collisions with each other box
		for (box *other = box_+1; other < m_box_data+box_count; other++){
			if (!m_cdata.hasmorecontacts()) { return; }

			collision_detector::boxandbox(*box_, *other, &m_cdata);

			if (intersection_tests::boxandbox(*box_, *other)){
				box_->m_is_over_lapping = other->m_is_over_lapping = true;
			}
		}
	}
}

void scene_manager::updateobjects( float duration) {

	// update the physics of each particle in turn
	for (ammo_round *shot = m_ammo; shot < m_ammo+m_ammo_rounds; shot++) {
		if (shot->m_type != UNUSED) {
			// run the physics
			shot->m_body->integrate(duration);
			shot->calculateinternals();

			shot->m_update_time += application_clock->m_last_frame_seconds;

			// check if the particle is now invalid
			if ( shot->m_update_time>5.0f ) {
				// we simply set the shot type to be unused, so the
				// memory it occupies can be reused by another shot.
				shot->m_type = UNUSED;
			}
		}
	}

	// update the physics of each box in turn
	for (box *box_ = m_box_data; box_ < m_box_data + box_count; box_++) {
		// run the physics
		box_->m_body->integrate(duration);
		box_->calculateinternals();
		box_->m_is_over_lapping = false;
	}

}

