#include "ui.h"

#include "application.h"
#include "d3d_window.h"
#include "d3d_manager.h"

#include "scene_manager.h"

#include "resource.h"

_vec2 * ui::s_font_vectors = NULL;

#define bmp_throw(x) { fclose(file); application_throw(x); }

 bool ui_control::chartest(WPARAM wParam){
	if( _application->testflags(application_controldown)||(wParam == VK_ESCAPE)||( wParam == VK_BACK )||( wParam == VK_RETURN ) ){ return false; }
	return true;
}

ui_control::ui_control(){

	m_x = m_y = 0;
	m_width = m_height =10.0f;
	m_font_width  = 8;
	m_font_height = 16;

	m_font_texture = NULL;
	m_foreground_vertex_buffer = NULL;
	m_background_vertex_buffer = NULL;
}

bool ui_control::intersection_test(){
	WINDOWINFO pwi;
	GetWindowInfo( _window->m_hwnd,&pwi);
	
	if( (_application->m_x_cursor_pos) < float(pwi.rcClient.left+m_x) ){ return false; }
	if( (_application->m_x_cursor_pos) > float(pwi.rcClient.left+m_x+m_width) ){ return false; }

	if( (_application->m_y_cursor_pos) < float(pwi.rcClient.top+m_y) ){ return false; }
	if( (_application->m_y_cursor_pos) > float(pwi.rcClient.top+m_y+m_height) ){ return false; }

	return true;
}

ui::ui() {
	/* "s_font_vectors" holds font texture positions in uvs of the font texture */
	if(!s_font_vectors) {
		s_font_vectors = new _vec2[256];
		for(uint16_t i=0;i<256;i++){
			s_font_vectors[i].x = (i%16)*0.0624f;
			s_font_vectors[i].y = floor(i/16.0f)*0.0624f;
		}
	}
	m_main_font_texture = NULL;
	m_current_control = -1;
}

bool ui::init(){

	float w = (float)_api_manager->m_d3dpp.BackBufferWidth;
	float h = (float)_api_manager->m_d3dpp.BackBufferHeight;

	m_projection = _ortho(0.0f,w,h,0.0f,-1000.0f,1000.0f);

	if(!loadfont() ){ return false; }
	application_throw_hr( D3DXCreateTexture(_api_manager->m_d3ddevice,m_image_width,m_image_height,D3DX_DEFAULT,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&m_main_font_texture) );

	D3DLOCKED_RECT rect;
	application_throw_hr( m_main_font_texture->LockRect(0,&rect,0,D3DLOCK_DISCARD) );

	D3DCOLOR * pixel = NULL;
	uint8_t  * pixel_pointer = (uint8_t*)rect.pBits;
	int count = m_image_size;
	/*
	* the font file used "courier.bmp" has RGBA format
	* the font data is stored so that all non character pixels have a zero alpha channel
	* by checking the alpha channel, this loop iterates and sets  the color of the font to ARGB(0xFF,0xFF,0xFF,0xFF)
	* which then is tested in each font using control to set all such values to the desired color
	*/

	for( uint32_t i = 0; i < m_image_height; i++) {
		pixel_pointer += rect.Pitch;
		pixel = (D3DCOLOR*)pixel_pointer;
		for( int32_t j = m_image_width-1; j >=0 ; j--, count-=4){
			if( (m_image_data[count+3])  < 254 ){
				pixel[j] = D3DCOLOR_ARGB(0,0,0,0);
			}else{
				pixel[j] = D3DCOLOR_ARGB(0xFF,0xFF,0xFF,0xFF);
			}
		}
	}
	m_main_font_texture->UnlockRect(0);

	for(uint32_t i=0;i<m_controls.m_count;i++){ m_controls[i]->init(); }

	return true;
}

void ui::clear(){

	for(uint32_t i=0;i<m_controls.m_count;i++){ m_controls[i]->clear(); }
	if(s_font_vectors) { delete [] s_font_vectors; }
	s_font_vectors = NULL;
	application_releasecom(m_main_font_texture);

}

bool ui::update(){


	application_throw_hr(_fx->SetMatrix(_api_manager->m_hmvp,(D3DXMATRIX*)&m_projection ));
	for(uint32_t i=0;i<m_controls.m_count;i++){ 

		bool control_switch = true;
		if(	!_scene_manager->testflags(_scene_menu) ){
			control_switch = ( /* when menu is not showing , don't render */
				( m_controls[i] != (ui_control*)_scene_manager->m_directions )  && 
				( m_controls[i] != (ui_control*)_scene_manager->m_exit  )       && 
				( m_controls[i] != (ui_control*)_scene_manager->m_continue )    &&    
				( m_controls[i] != (ui_control*)_scene_manager->m_vsync  )      && 
				( m_controls[i] != (ui_control*)_scene_manager->m_fullscreen  )       
				);

			if(!_scene_manager->testflags(_scene_aim) ){

				if( !_scene_manager->testflags( _scene_aim ) ) {
					control_switch = control_switch &&  (/* when menu is not showing , don't render */
						( m_controls[i] != (ui_control*)_scene_manager->m_cross_hair_1) &&
						( m_controls[i] != (ui_control*)_scene_manager->m_cross_hair_2)
						);
				}
			}

		}else{
			if( !_scene_manager->testflags( _scene_aim ) ) {
				control_switch =  (/* when not in aim mode, don't render */
					( m_controls[i] != (ui_control*)_scene_manager->m_cross_hair_1) &&
					( m_controls[i] != (ui_control*)_scene_manager->m_cross_hair_2)
					);
			}
		}
		if( control_switch ) { m_controls[i]->update(); }
	}
	return true;
}

void ui::onresetdevice(){
	/*resize causes reset, so update projection matrix*/
	float w = (float)_api_manager->m_d3dpp.BackBufferWidth;
	float h = (float)_api_manager->m_d3dpp.BackBufferHeight;
	m_projection = _ortho(0.0f,w,h,0.0f,-1000.0f,1000.0f);
	/******************************************************/
}

bool ui::addcontrol(ui_control * control){

	if(!control){ application_throw("control pointer"); }

	control->m_id = m_controls.m_count; 
	m_controls.pushback(control);

	return true;
}

bool ui::loadfont(){

	/* load resource data */
	LPVOID data = application::getresourcedata( IDR_FONT );
	if( data == (LPVOID)NULL ){ application_throw("font data"); }

	const uint8_t * all_data = (uint8_t *)data;

	uint32_t  channel_count;

	if ( all_data[0]!='B' || all_data[1]!='M' ) {  application_throw(" font "); }

	/* 40 = BITMAPINFOHEADER 52 = BITMAPV2INFOHEADER 108 =BITMAPV4HEADER 124 = BITMAPV5HEADER */
	uint32_t headertype = *(uint32_t*)&(all_data[0x0E]);
	if( (headertype != 40) && (headertype != 52) && (headertype != 108) && (headertype != 124)  ){ application_throw("compression"); }

	/*  0 = BI_RGB  3 = BI_BITFIELDS  6 = BI_ALPHABITFIELDS */ 
	uint32_t compression = *(int*)&(all_data[0x1E]);
	if( compression== 0 )     { channel_count = 3; }
	else if( compression==3 ) { channel_count = headertype<108?3:4; }
	else if( compression==6 ) { channel_count = 4; }
	else { application_throw("compression"); }

	uint16_t bitcount = *(uint16_t*)&(all_data[0x1C]);
	if ( bitcount!=24 && bitcount!=32 )       { application_throw("bitcount"); }

	m_image_width          = *(uint32_t*)&(all_data[0x12]);
	m_image_height         = *(uint32_t*)&(all_data[0x16]);
	if(m_image_width==0 || m_image_height==0)   { application_throw("image dimensions"); }

	uint32_t data_position = *(uint32_t*)&(all_data[0x0A]);
	if(data_position==0)   { application_throw("data position "); }

	/*the image size. This is the size of the raw bitmap data; a dummy 0 can be given for BI_RGB bitmaps.*/
	m_image_size      = *(uint32_t*)&(all_data[0x22]);
	if(m_image_size==0) { m_image_size=m_image_width*m_image_height*channel_count; }
	else{ 
		if( (m_image_size/channel_count) != (m_image_width*m_image_height) ){ application_throw("imagesize"); }
	}

	/* read pixel array*/
	uint8_t * image_data = (uint8_t*)&(all_data[data_position]);

	m_image_size = m_image_width*m_image_height*4;
	m_image_data = new uint8_t[m_image_size];
	uint32_t data_index = 0;

	for(uint32_t i =0; i<m_image_height ;i++){
		for(uint32_t ii =0; ii<(m_image_width*4) ;ii+=4,data_index+=channel_count ){
			m_image_data[ (i*m_image_width*4)+ii   ] = image_data[ data_index    ];
			m_image_data[ (i*m_image_width*4)+ii+1 ] = image_data[ data_index+1  ];
			m_image_data[ (i*m_image_width*4)+ii+2 ] = image_data[ data_index+2  ];
			if(channel_count==4){
				m_image_data[ (i*m_image_width*4)+ii+3 ] = image_data[ data_index+3  ];
			}else{
				m_image_data[ (i*m_image_width*4)+ii+3 ] = 0xFF;
			}
		}
	}

	application::freeresourcedata();

	return true;
}
