#include "camera.h"

#include "application.h"
#include "d3d_manager.h"

#include "scene_manager.h"

#include "485.h"

bool camera::s_start = true;

bool camera::init(){

	m_look     = _vec3(0.0f,-1.0f,0.0f);
	m_target   = _vec3(0.0f,0.0f,0.0f);

	m_yaw      = 180.0f;
	m_pitch    = -45.0f;

	m_distance = 10.0f;

	m_x_pos    = 0;
	m_y_pos    = 0;

	return true;
}

void  camera::generate_data(float yaw, float pitch){

	/* spherical  camera */
	_mat4  r   = _yawpitchroll(float(_radians(yaw)),pitch,0.0f);
	_vec4 t    = r*_vec4(0.0f,0,m_distance,0.0f);
	_vec4 up   = r*_vec4( _utility::up.x , _utility::up.y, _utility::up.z,0.0f);
	m_target   = _485_bounding_box.m_body->getposition();
	m_up       = _vec3(up.x,up.y,up.z);
	m_position = m_target + _vec3(t.x,t.y,t.z);
	m_look     = _normalize(m_target-m_position);
	/****************************************************************************/

	_vec3 direction    =_vec3( m_look.x , 0.0f , m_look.z );
	direction.normalise();

	m_last_orientation = _quaternion();
	m_last_orientation.rotatebyvector( direction );

}

bool camera::update(){


	if( !_scene_manager->testflags(_scene_menu) && !camera::s_start) {

		POINT cursor_position;
		float width  = float(GetSystemMetrics(SM_CXSCREEN))/2.0f;
		float height = float(GetSystemMetrics(SM_CYSCREEN))/2.0f;

		if ( GetCursorPos(&cursor_position)== 0 ) {  application_error("cursor pos"); }

		if( SetCursorPos( int(width) , int(height) ) == 0 ) { application_error("cursor pos"); }
			m_pitch += ( height  - float(cursor_position.y ) );
			m_yaw   += ( width  - float(cursor_position.x ) );

		float pitch_min = _scene_manager->testflags(_scene_aim)?-100.0f:-70.0f;
		float pitch_max = _scene_manager->testflags(_scene_aim)? 100.0f: 10.0f;

		m_pitch = (m_pitch > pitch_max ) ? pitch_max  : m_pitch;
		m_pitch = (m_pitch < pitch_min ) ? pitch_min : m_pitch;

		m_yaw   = (m_yaw   >    (FLT_MAX/2) )  ?    (FLT_MAX/2) : m_yaw;
		m_yaw   = (m_yaw   <   -(FLT_MAX/2) )  ?   -(FLT_MAX/2) : m_yaw;

		float pitch = _scene_manager->testflags(_scene_aim)?0.0f:float(_radians(m_pitch));
		generate_data(m_yaw, pitch);

	}else {

		if(camera::s_start){
			/* starting animation */
			static float interpolate =0.0f;
			static float yaw = 0.0f;
			static float pitch = 0.0f;

			float yaw_ =_lerp(yaw,m_yaw,interpolate);
			float pitch_ =_lerp(pitch,-40.0f,interpolate);

			generate_data( yaw_ , float( _radians(pitch_) ) );

			m_orientation = m_last_orientation;
			m_last_view =_lookatrh(m_position,m_target,m_up);
			m_view = m_last_view;

			if(camera::s_start && !_scene_manager->testflags(_scene_menu) ){

				interpolate += application_clock->m_last_frame_seconds*0.5f;
				m_pitch = pitch_;
			}
			if(interpolate>=1.0f){ 
				_485_bounding_box.m_body->setcansleep(false);
				camera::s_start = false;
			}
		}else{
			m_view = m_last_view;
			m_orientation = m_last_orientation;
		}
	}

	if( _scene_manager->testflags(_scene_aim) ) {

		/* spherical  camera */
		m_aim_position     = _485_bounding_box.m_body->gettransform() * _vec3(-2.0f,3.0f,-4.0f);

		_mat4 rp = _yawpitchroll(0.0f,float(_radians(-m_pitch)),0.0f);
		m_aim_target = rp* _vec3(-2.0f,3.0f,4.0f);

		m_aim_target     = _485_bounding_box.m_body->gettransform() * m_aim_target;
		m_view = _lookatrh(m_aim_position,m_aim_target,_utility::up);

		m_aim_look = _normalize(m_aim_target-m_aim_position);
		/****************************************************************************/

	}else{
		if( !_scene_manager->testflags(_scene_menu) ){
			m_view =_lookatrh(m_position,m_target,m_up);
			m_last_view = m_view;
		}
	}
	//***************************************************/

	return true;
}

void camera::onresetdevice() {
	/*resize causes reset, so update projection matrix*/
	float w = (float)_api_manager->m_d3dpp.BackBufferWidth;
	float h = (float)_api_manager->m_d3dpp.BackBufferHeight;
	m_projection = _perspectivefovrh(D3DX_PI * 0.25f, w,h, 1.0f, 1000.0f);
	/***************************************************/
}
