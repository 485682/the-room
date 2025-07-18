#include "d3d_manager.h"
#include "application.h"
#include "d3d_window.h"


d3d_manager* d3d_manager::_manager = NULL;

d3d_manager::d3d_manager(){

	m_fx     = NULL;

	application_zero(&m_d3dpp,sizeof(m_d3dpp));

	m_d3dobject = NULL;
	m_d3ddevice = NULL;

	m_bone_vertex_declaration = NULL;
	m_object_vertex_declaration = NULL;
	m_object_vertex_uv_declaration = NULL;
	m_ui_foreground_vertex_declaration = NULL;
	m_ui_background_vertex_declaration = NULL;

	m_hmv                 = (D3DXHANDLE)NULL;
	m_hmvp                = (D3DXHANDLE)NULL;
	m_htex                = (D3DXHANDLE)NULL;
	m_hworld              = (D3DXHANDLE)NULL;

	m_hcolor              = (D3DXHANDLE)NULL;
	m_hbones              = (D3DXHANDLE)NULL;

	m_htech_blend         = (D3DXHANDLE)NULL;
	m_htech_object        = (D3DXHANDLE)NULL;
	m_htech_object_uv     = (D3DXHANDLE)NULL;
	m_htech_floor         = (D3DXHANDLE)NULL;
	m_htech_ui_foreground = (D3DXHANDLE)NULL;
	m_htech_ui_background = (D3DXHANDLE)NULL;


}

bool d3d_manager::init(){

	m_d3dobject = Direct3DCreate9(D3D_SDK_VERSION);
	if( !m_d3dobject ) { application_throw("d3dobject"); }

	// Step 2: Verify hardware support for specified formats in windowed and full screen modes.
	D3DDISPLAYMODE mode;
	HRESULT HR = m_d3dobject->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &mode);
	if(HR != D3D_OK){ application_throw("hr"); }

	application_throw_hr(m_d3dobject->CheckDeviceType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, mode.Format, mode.Format, true));
	application_throw_hr(m_d3dobject->CheckDeviceType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, D3DFMT_X8R8G8B8, false));

	// Step 3: Check for requested vertex processing and pure device.
	D3DCAPS9 caps;
	application_throw_hr(m_d3dobject->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps));

	DWORD devbehaviorflags = 0;
	if( caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT ) { devbehaviorflags |= D3DCREATE_HARDWARE_VERTEXPROCESSING; }
	else { devbehaviorflags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING; }

	// If pure device and HW T&L supported
	if( caps.DevCaps & D3DDEVCAPS_PUREDEVICE && devbehaviorflags & D3DCREATE_HARDWARE_VERTEXPROCESSING) { devbehaviorflags |= D3DCREATE_PUREDEVICE; }

	// Step 4: Fill out the D3DPRESENT_PARAMETERS structure.
	m_d3dpp.BackBufferWidth            = 0;
	m_d3dpp.BackBufferHeight           = 0;
	m_d3dpp.BackBufferFormat           = D3DFMT_UNKNOWN;
	m_d3dpp.BackBufferCount            = 1;
	m_d3dpp.MultiSampleType            = D3DMULTISAMPLE_NONE;
	m_d3dpp.MultiSampleQuality         = 0;
	m_d3dpp.SwapEffect                 = D3DSWAPEFFECT_DISCARD;
	m_d3dpp.hDeviceWindow              = _window->m_hwnd;
	m_d3dpp.Windowed                   = true;
	m_d3dpp.EnableAutoDepthStencil     = true;
	m_d3dpp.AutoDepthStencilFormat     = D3DFMT_D24S8;
	m_d3dpp.Flags                      = 0;
	m_d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
	m_d3dpp.PresentationInterval       = D3DPRESENT_INTERVAL_IMMEDIATE;

	// Create the device.
	application_throw_hr(m_d3dobject->CreateDevice(

		D3DADAPTER_DEFAULT,// primary adapter
		D3DDEVTYPE_HAL,    // device type
		_window->m_hwnd,   // window associated with device
		devbehaviorflags,  // vertex processing
		&m_d3dpp,          // present parameters
		&m_d3ddevice       // return created device

		));
	//  shader version 2.1 or greater required
	application_zero( &caps , sizeof(D3DCAPS9) );
	application_throw_hr(m_d3ddevice->GetDeviceCaps(&caps));
	if( caps.VertexShaderVersion < D3DVS_VERSION(2, 0) ) { application_throw("dev caps"); }
	if( caps.PixelShaderVersion  < D3DPS_VERSION(2, 0) ) { application_throw("dev caps"); }

	D3DVERTEXELEMENT9 vertexelements_ui_foreground[] = {
		{0, 0,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
		{0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
		D3DDECL_END()
	};
	application_throw_hr(_api_manager->m_d3ddevice->CreateVertexDeclaration(vertexelements_ui_foreground, &m_ui_foreground_vertex_declaration));

	D3DVERTEXELEMENT9 vertexelements_ui_background[] = {
		{0, 0,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
		D3DDECL_END()
	};
	application_throw_hr(_api_manager->m_d3ddevice->CreateVertexDeclaration(vertexelements_ui_background, &m_ui_background_vertex_declaration));


	/* bone_vertex_declaration ********************************************************/
	m_bone_vertex_declaration = NULL;
	D3DVERTEXELEMENT9 bone_vertexelements[] = {
		{0, 0,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
		{0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
		{0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
		{0, 32, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0},
		{0, 48, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDWEIGHT, 0},
		D3DDECL_END()
	};
	application_throw_hr(_api_manager->m_d3ddevice->CreateVertexDeclaration(bone_vertexelements, &m_bone_vertex_declaration));
	/*****************************************************************************/

	/* object_vertex_uv_declaration ********************************************************/
	m_object_vertex_uv_declaration = NULL;
	D3DVERTEXELEMENT9 vertexelements_uv[] = {
		{0, 0,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
		{0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
		{0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
		D3DDECL_END()
	};
	application_throw_hr(_api_manager->m_d3ddevice->CreateVertexDeclaration(vertexelements_uv, &m_object_vertex_uv_declaration));
	/*****************************************************************************/

	/* object_vertex_declaration ********************************************************/
	m_object_vertex_declaration = NULL;
	D3DVERTEXELEMENT9 vertexelements[] = {
		{0, 0,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
		{0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
		{0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
		D3DDECL_END()
	};
	application_throw_hr(_api_manager->m_d3ddevice->CreateVertexDeclaration(vertexelements, &m_object_vertex_declaration));
	/*****************************************************************************/

	return buildfx();
}

void d3d_manager::clear(){

	application_releasecom(m_fx);
	application_releasecom(m_d3ddevice);
	application_releasecom(m_d3dobject);
	application_releasecom(m_bone_vertex_declaration);
	application_releasecom(m_object_vertex_declaration);
	application_releasecom(m_object_vertex_uv_declaration);
	application_releasecom(m_ui_foreground_vertex_declaration);
	application_releasecom(m_ui_background_vertex_declaration);

}

bool d3d_manager::reset(){
	_application->onlostdevice();
	application_throw_hr(m_d3ddevice->Reset( &m_d3dpp) );
	_application->onresetdevice();
	return true;
}

bool d3d_manager::buildfx() {

	_string shader =
		" uniform extern float4x4 g_mv; "
		" uniform extern float4x4 g_mvp; "
		" uniform extern float4x4 g_world;" 
		" uniform extern float4x4 g_bones[57];"

		" uniform float4          g_color;"

		" uniform float3 lightposition_0 = float3(0.0f,5.0f, 5.0f);"
		" uniform float3 lightposition_1 = float3(0.0f,5.0f,-5.0f);"

		" uniform extern texture g_tex;"
		" sampler tex_s = sampler_state { Texture = <g_tex>; };"

		" sampler tex_s_f = sampler_state {"
		" Texture = <g_tex>;"
		" MinFilter = LINEAR;"
		" MagFilter = LINEAR;"
		" MipFilter = LINEAR;"
		" AddressU  = WRAP;"
		" AddressV  = WRAP;"
		" };"

		" struct ui_output{  "
		"  float4 pos      : POSITION0; "
		"  float2 tex      : TEXCOORD0;  "
		" };"

		" ui_output VertexShader_ui_foreground( float3 pos : POSITION0 , float2 tex : TEXCOORD0 ) {  "
		"  ui_output output = (ui_output)0; "
		"  output.pos = mul(float4(pos, 1.0f), g_mvp); "
		"  output.tex = tex; "
		"  return output; "
		" }"
		" float4 PixelShader_ui_foreground(float2 tex : TEXCOORD0) : COLOR { return tex2D(tex_s, tex); }"
		" technique ui_foreground_tech { "
		"  pass P0 "
		"     { "
		"       vertexShader = compile vs_2_0 VertexShader_ui_foreground(); "
		"       pixelShader  = compile ps_2_0 PixelShader_ui_foreground(); "
		" 		AlphaBlendEnable = true;"
		"       SrcBlend = SrcAlpha;"
		"       DestBlend = InvSrcAlpha;"
		"     }  "
		"}"

		"float4 PixelShader_floor(float2 tex : TEXCOORD0) : COLOR { return float4(tex2D(tex_s_f, tex).rgb,0.4f); }"
		
		"technique floor_tech { "
		"  pass P0 "
		"     { "
		"       vertexShader = compile vs_2_0 VertexShader_ui_foreground(); "
		"       pixelShader  = compile ps_2_0 PixelShader_floor(); "
		"		AlphaBlendEnable = true;"
		"       SrcBlend = SrcAlpha;"
		"       DestBlend = InvSrcAlpha;"
		"     }  "
		"}"

		"struct ui_output_background { float4 pos : POSITION0; };"
		
		"ui_output_background VertexShader_ui_background( float3 pos       : POSITION0 ) {  "
		"  ui_output_background output = (ui_output_background)0; "
		"  output.pos = mul(float4(pos, 1.0f), g_mvp);"
		"  return output; "
		"}"

		"float4 PixelShader_ui_background() : COLOR { return g_color; }"
		"technique ui_background_tech { "
		"  pass P0 "
		"     { "
		"       vertexShader = compile vs_2_0 VertexShader_ui_background(); "
		"       pixelShader  = compile ps_2_0 PixelShader_ui_background(); "
		"		AlphaBlendEnable = true;"
		"       SrcBlend = SrcAlpha;"
		"       DestBlend = InvSrcAlpha;"
		"     }"
		"}"

		"struct object_output{  "
		"  float4 pos      : POSITION0; "
		"  float2 tex      : TEXCOORD0;  "
		"  float3 normal   : TEXCOORD1;"
		"  float4 eyecoords: TEXCOORD2;"
		"};"

		"object_output VertexShader_blend("
		"  float3 position  : POSITION0,"
		"  float3 normal    : NORMAL0,"
		"  float2 tex       : TEXCOORD0,"
		"  float4 boneindex : BLENDINDICES0,"
		"  float4 weights   : BLENDWEIGHT0  ) {"

		"         object_output output = (object_output)0;"
		"         float4 position_ = weights.x * mul(float4(position, 1.0f), g_bones[ int(boneindex.x) ]);"
		"         position_ += weights.y * mul(float4(position, 1.0f), g_bones[ int(boneindex.y) ]);"
		"         position_ += weights.z * mul(float4(position, 1.0f), g_bones[ int(boneindex.z) ]);"
		"         position_ += weights.w * mul(float4(position, 1.0f), g_bones[ int(boneindex.w) ]);"
		"         position_.w = 1.0f;"
		"         float4 normal_ = weights.x * mul(float4(normal, 0.0f), g_bones[ int(boneindex.x) ]);"
		"         normal_ += weights.y * mul(float4(normal, 0.0f), g_bones[ int(boneindex.y) ]);"
		"         normal_ += weights.z * mul(float4(normal, 0.0f), g_bones[ int(boneindex.z) ]);"
		"         normal_ += weights.w * mul(float4(normal, 0.0f), g_bones[ int(boneindex.w) ]);"
		"         normal_.w = 0.0f;"

		"		  output.normal = normalize( mul(normal_, g_world).xyz );"
		"         output.eyecoords = mul(g_mv , position_);"

		"         output.tex  = tex;"
		"         output.pos  = mul(position_, g_mvp);"

		"         return output;"
		"	}"

		"float4 PixelShader_blend( float4 eye_pos : TEXCOORD4, float2 tex0:TEXCOORD0,float3 normal:TEXCOORD1 ) : COLOR { "

		"     const int levels = 3;"
		"     const float scaleFactor = 1.0 / levels;"
		"	  float3  s = normalize( lightposition_1 - eye_pos.xyz );"
		"	  float  cosine = min( 0.4, max( 0.0, dot( s, normal ) ));"
		"	  float3 diffuse = (floor( cosine * levels ) * scaleFactor) ;"
		"	  diffuse *= 0.2f;"
		"	  diffuse += tex2D(tex_s_f, tex0).rgb;"
		"     return float4(diffuse,g_color.a);"

		"} "

		"technique bone_tech { "
		"     pass P0" 
		"         { "
		"             vertexShader = compile vs_2_0 VertexShader_blend(); "
		"             pixelShader  = compile ps_2_0 PixelShader_blend();"
		"         }"
		"}"

		"object_output VertexShader_obj ( float3 position : POSITION0, float3 normal : NORMAL0 ) {"
		"         object_output output = (object_output)0;"
		"         output.pos        = mul( float4(position,1.0f)    , g_mvp);"
		"		  output.normal     = mul( float4(normal,0.0f) , g_world).xyz;"
		"         output.eyecoords  = mul(g_mv , float4(position,1.0f) );"

		"         return output;"
		"}"
		"float4 PixelShader_obj ( float4 eye_pos : TEXCOORD4,float2 tex0:TEXCOORD0, float3 normal:TEXCOORD1 ) : COLOR { "

		"	 const int levels = 3;"
		"    const float scaleFactor = 1.0 / levels;"
		"	 float3  s = normalize( lightposition_1 - eye_pos.xyz );"
		"	 float  cosine = min( 0.4, max( 0.0, dot( s, normal ) ));"
		"	 float3 diffuse = (floor( cosine * levels ) * scaleFactor) ;"
		"	 diffuse *= 0.2f;"
		"	 diffuse += g_color.rgb;"
		"    return float4(diffuse,g_color.a);"
		"}"
		"technique object_tech { "
		"     pass P0 "
		"         { "
		"             vertexShader = compile vs_2_0 VertexShader_obj(); "
		"             pixelShader  = compile ps_2_0 PixelShader_obj();"
		"             AlphaBlendEnable = true;"
		"             SrcBlend = SrcAlpha;"
		"             DestBlend = InvSrcAlpha;"
		"         }"
		"}"


		"object_output VertexShader_obj_uv ( float3 position : POSITION0, float3 normal : NORMAL0,float2 tex : TEXCOORD0) {"
		"        object_output output = (object_output)0;"
		"		 output.tex       = tex;"
		"        output.pos       = mul( float4(position,1.0f)    , g_mvp);"
		"		 output.normal    = mul( float4(normal,0.0f) , g_world).xyz;"
		"		 output.eyecoords = mul(g_mv , float4(position,1.0f) );"

		"        return output;"
		"	}"

		"float4 PixelShader_obj_uv ( float4 eye_pos : TEXCOORD4, float2 tex0:TEXCOORD0, float3 normal:TEXCOORD1 ) : COLOR { "

		"    const int levels = 3;"
		"    const float scaleFactor = 1.0 / levels;"
		"	 float3  s = normalize( lightposition_1 - eye_pos.xyz );"
		"	 float  cosine = min( 0.4, max( 0.0, dot( s, normal ) ));"
		"	 float3 diffuse = (floor( cosine * levels ) * scaleFactor) ;"
		"	 diffuse *= 0.4f;"
		"	 diffuse += ((tex2D(tex_s, tex0).rgb*0.2f)+(g_color.rgb)*0.8f);"
		"    return float4(diffuse,g_color.a);"

		"} "

		"technique object_uv_tech { "
		"     pass P0 "
		"         { "
		"             vertexShader = compile vs_2_0 VertexShader_obj_uv(); "
		"             pixelShader  = compile ps_2_0 PixelShader_obj_uv();"

		"             AlphaBlendEnable = true;"
		"             SrcBlend = SrcAlpha;"
		"             DestBlend = InvSrcAlpha;"
		"         }"
		"}" ;

	/* load effect **********************************************************/
	ID3DXBuffer* errors = 0;
	HRESULT fx_result =D3DXCreateEffect(_api_manager->m_d3ddevice,
		shader.m_data,
		shader.m_count,
		0, 0, D3DXSHADER_DEBUG, 0, &m_fx, &errors);
	if( errors )    { application_throw((char*)errors->GetBufferPointer()); }
	if( fx_result ) { application_throw("effect file"); }
	/*****************************************************************************/

	/* Obtain handles. **********************************************************/
	m_hmv                = m_fx->GetParameterByName(0, "g_mv");

	m_hmvp                = m_fx->GetParameterByName(0, "g_mvp");
	m_htex                = m_fx->GetParameterByName(0, "g_tex");
	m_hcolor              = m_fx->GetParameterByName(0, "g_color");
	m_hworld              = m_fx->GetParameterByName(0, "g_world");


	m_hbones              = m_fx->GetParameterByName(0, "g_bones");

	m_htech_blend         = m_fx->GetTechniqueByName("bone_tech");
	m_htech_object        = m_fx->GetTechniqueByName("object_tech");
	m_htech_object_uv     = m_fx->GetTechniqueByName("object_uv_tech");
	m_htech_floor         = m_fx->GetTechniqueByName("floor_tech");
	m_htech_ui_foreground = m_fx->GetTechniqueByName("ui_foreground_tech");
	m_htech_ui_background = m_fx->GetTechniqueByName("ui_background_tech");

	if( m_hmv          == (D3DXHANDLE)NULL ){ application_throw("mv handle"); }
	if( m_hmvp         == (D3DXHANDLE)NULL ){ application_throw("mvp handle"); }
	if( m_htex         == (D3DXHANDLE)NULL ){ application_throw("texture handle"); }
	if( m_hcolor       == (D3DXHANDLE)NULL ){ application_throw("color handle"); }
	if( m_hworld       == (D3DXHANDLE)NULL ){ application_throw("world handle"); }

	if( m_hbones       == (D3DXHANDLE)NULL ){ application_throw("bone handle"); }	

	if( m_htech_blend         ==(D3DXHANDLE)NULL ){ application_throw("technique handle"); }
	if( m_htech_object        ==(D3DXHANDLE)NULL ){ application_throw("technique handle"); }
	if( m_htech_object_uv     ==(D3DXHANDLE)NULL ){ application_throw("technique handle"); }
	if( m_htech_floor         ==(D3DXHANDLE)NULL ){ application_throw("technique handle"); }
	if( m_htech_ui_foreground ==(D3DXHANDLE)NULL ){ application_throw("technique handle"); }
	if( m_htech_ui_background ==(D3DXHANDLE)NULL ){ application_throw("technique handle"); }

	/****************************************************************************/

	return true;
}
