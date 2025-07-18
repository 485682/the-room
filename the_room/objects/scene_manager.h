#pragma once

#include "application_header.h"
#include "physics.h"

/** forward declaration  */
struct ui;
struct ui_text;
struct ui_static;
struct ui_button;

struct ui_control;

struct camera;

struct the_room;
struct object_485;
/*************************/


enum shotstate { UNUSED = 0, FIREING };
struct ammo_round : public collision_sphere {

	float m_update_time;
	shotstate m_type;

	ammo_round()  { m_body = new rigid_body; }

	~ammo_round() { delete m_body; }

	// sets the properties of the round
	void setstate();
};


#define box_count 10

struct box : public collision_box {

	bool m_is_over_lapping;

	box()  { m_body = new rigid_body(); }

	~box() { delete m_body; }

	/** sets the box to a specific location. */
	void setstate(const _vec3 &position,
		const _quaternion &orientation,
		const _vec3 &extents,
		const _vec3 &velocity)
	{
		m_body->setposition(position);
		m_body->setorientation(orientation);
		m_body->setvelocity(velocity);
		m_body->setrotation( _vec3(0,0,0) );
		m_half_size = extents;

		float mass = m_half_size.x * m_half_size.y * m_half_size.z * 8.0f;
		m_body->setmass(mass);


		_mat3 tensor;
		tensor.setblockinertiatensor( m_half_size , mass);
		m_body->setinertiatensor(tensor);

		m_body->setlineardamping(0.95f);
		m_body->setangulardamping(0.8f);
		m_body->clearaccumulators();
		m_body->setacceleration(0,-10.0f,0);

		m_body->setawake();

		m_body->calculatederiveddata();
	}

	static _vec4 * s_box_colors;
};

#define _scene_aim  0x01
#define _scene_menu 0x02

struct scene_manager : public application_object {

	scene_manager();

	virtual bool init();
	virtual void clear();
	virtual bool update();
	virtual void onlostdevice();
	virtual void onresetdevice();

	virtual void msgproc(UINT msg, WPARAM wParam, LPARAM lParam);

	bool loadmesh( _mesh * mesh, int id);

	/* global object array */
	_array<application_object*> m_object_array;
	/**********************************************************/

	object_485 * m_485;
	the_room   * m_the_room;


	/* global ui class */
	ui * m_ui;

	/***** ui control pointers*/
	ui_static * m_cross_hair_1;
	ui_static * m_cross_hair_2;

	ui_static * m_fps_control;
	ui_text   * m_directions;
	ui_button * m_continue;
	ui_button * m_vsync;
	ui_button * m_fullscreen;
	ui_button * m_exit;
	/**************************/

	/* main camera; */
	camera * m_camera;


	/* physics... */

	/** holds the maximum number of contacts. */
	const static unsigned max_contacts = 256;

	/** holds the array of contacts. */
	contact m_contacts[max_contacts];

	/** holds the collision data structure for collision detection. */
	collision_data m_cdata;

	/** holds the contact resolver. */
	contact_resolver m_resolver;

	/**
	* holds the maximum number of  rounds that can be
	* fired.
	*/
	const static unsigned m_ammo_rounds = 1024;

	/** 
	* holds the particle data 
	*/
	ammo_round m_ammo[m_ammo_rounds];


	box m_box_data[box_count];

	/** processes the contact generation code. */
	void generatecontacts();

	/** processes the objects in the simulation forward in time. */
	void updateobjects( float duration);

	void reset();

};

