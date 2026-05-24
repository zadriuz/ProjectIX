// ============================================
// cloner.cpp - Safe Teleport & NavMesh Snap (SFX FIX)
// ============================================
#include <Godot/godot.hpp>
#include <Godot/classes/character_body2d.hpp>
#include <Godot/classes/animated_sprite2d.hpp>
#include <Godot/classes/sprite2d.hpp>
#include <Godot/classes/navigation_agent2d.hpp>
#include <Godot/classes/area2d.hpp>
#include <Godot/classes/timer.hpp>
#include <Godot/classes/scene_tree.hpp>
#include <Godot/classes/node.hpp>
#include <Godot/classes/node2d.hpp>
#include <Godot/classes/window.hpp>
#include <Godot/classes/navigation_server2d.hpp> // REQUIRED FOR SAFETY CHECK
#include <Godot/classes/audio_stream_player2d.hpp> // REQUIRED FOR SFX
#include <Godot/variant/callable.hpp>
#include <Godot/variant/string_name.hpp>
#include <string>
#include <cstdlib>
#include <cmath>

using namespace godot;
using namespace jenova::sdk;

// --- SETTINGS ---
constexpr float CHASE_SPEED = 155.0f;
constexpr float DAMAGE_RADIUS = 25.0f;
constexpr float TELEPORT_DISTANCE = 150.0f; 

JENOVA_SCRIPT_BEGIN

// Forward declarations
void _on_territory_entered(Caller* instance, Variant& body_v);
void _on_stuck_check(Caller* instance);

void _ready(Caller* instance)
{
	CharacterBody2D* self = GetSelf<CharacterBody2D>(instance);
	if (!self) return;

	// 1. STATE INIT
	self->set_meta("is_triggered", false);
	self->set_meta("last_pos", self->get_global_position());
	self->set_meta("target_node", Variant());

	// 2. HIDE ICON
	Node2D* icon = Object::cast_to<Node2D>(self->get_node_or_null(NodePath("AlertIcon")));
	if (icon) icon->set_visible(false);

	// 3. SPRITE SETUP
	AnimatedSprite2D* sprite = Object::cast_to<AnimatedSprite2D>(self->get_node_or_null(NodePath("AnimatedSprite2D")));
	if (sprite) {
		sprite->play("idle");
		sprite->pause();
		sprite->set_frame(std::rand() % 2);
		sprite->set_modulate(Color(0.4, 0.4, 0.4, 1.0));
	}

	// 4. DETECTION SETUP
	Area2D* territory = Object::cast_to<Area2D>(self->get_node_or_null(NodePath("DetectionRadius")));
	if (territory) {
		if (!territory->is_connected("body_entered", Callable(self, "_on_territory_entered"))) {
			territory->connect("body_entered", Callable(self, "_on_territory_entered"));
		}
	}

	// 5. STUCK CHECK TIMER
	Timer* stuck_timer = Object::cast_to<Timer>(self->get_node_or_null(NodePath("StuckCheckTimer")));
	if (stuck_timer) {
		if (!stuck_timer->is_connected("timeout", Callable(self, "_on_stuck_check"))) {
			stuck_timer->connect("timeout", Callable(self, "_on_stuck_check"));
		}
	}
	
	// 6. AUDIO START FIX: Do NOT play footsteps here. We control it in the physics loop.
	AudioStreamPlayer2D* sfx = Object::cast_to<AudioStreamPlayer2D>(self->get_node_or_null(NodePath("FootstepSFX")));
	if (sfx) {
		// Ensure it's not playing from a previous state/scene restart
		sfx->stop(); 
	}
	
}

void _on_stuck_check(Caller* instance)
{
	CharacterBody2D* self = GetSelf<CharacterBody2D>(instance);
	if (!self) return;

	bool triggered = (bool)self->get_meta("is_triggered", false);
	if (!triggered) return; 

	// 1. Movement Check
	Vector2 current_pos = self->get_global_position();
	Vector2 last_pos = (Vector2)self->get_meta("last_pos", current_pos);
	float dist_moved = current_pos.distance_to(last_pos);
	self->set_meta("last_pos", current_pos); 

	// 2. Teleport Logic (If stuck for the Timer Duration, usually 5s)
	if (dist_moved < 50.0f) {
		

		Object* target_obj = self->get_meta("target_node");
		Node2D* player = Object::cast_to<Node2D>(target_obj);
		
		if (!player) {
			SceneTree* tree = self->get_tree();
			if (tree) player = Object::cast_to<Node2D>(tree->get_current_scene()->find_child("SubjectIX", true, false));
		}
		
		if (player) {
			// Stop footstep audio during teleport
			AudioStreamPlayer2D* sfx = Object::cast_to<AudioStreamPlayer2D>(self->get_node_or_null(NodePath("FootstepSFX")));
			if (sfx) sfx->stop();
			
			// A. Calculate Random Point around Player
			float angle = (float)(std::rand() % 360) * 3.14159f / 180.0f;
			Vector2 offset = Vector2(std::cos(angle), std::sin(angle)) * TELEPORT_DISTANCE;
			Vector2 raw_target = player->get_global_position() + offset;

			// B. SNAP TO NAVMESH (The "Wall Check")
			NavigationAgent2D* nav = Object::cast_to<NavigationAgent2D>(self->get_node_or_null(NodePath("NavigationAgent2D")));
			if (nav) {
				RID map = nav->get_navigation_map();
				Vector2 safe_point = NavigationServer2D::get_singleton()->map_get_closest_point(map, raw_target);
				
				// Teleport there
				self->set_global_position(safe_point);
				nav->set_target_position(player->get_global_position());
			
			}
		}
	}
}

void _on_territory_entered(Caller* instance, Variant& body_v)
{
	CharacterBody2D* self = GetSelf<CharacterBody2D>(instance);
	if (!self) return;

	if ((bool)self->get_meta("is_triggered", false)) return;

	Object* obj = body_v;
	Node* body = Object::cast_to<Node>(obj);
	if (!body) return;

	if (body->get_name() == StringName("SubjectIX") || body->is_in_group("Player"))
	{
		self->set_meta("is_triggered", true);
		self->set_meta("target_node", body); 
		

		Timer* timer = Object::cast_to<Timer>(self->get_node_or_null(NodePath("DelayTimer")));
		if (timer) timer->start(); 

		Node2D* icon = Object::cast_to<Node2D>(self->get_node_or_null(NodePath("AlertIcon")));
		if (icon) icon->set_visible(true);

		AnimatedSprite2D* sprite = Object::cast_to<AnimatedSprite2D>(self->get_node_or_null(NodePath("AnimatedSprite2D")));
		Node2D* player_node = Object::cast_to<Node2D>(body); 

		if (sprite && player_node) {
			sprite->set_modulate(Color(1, 1, 1, 1)); 
			float player_x = player_node->get_global_position().x;
			float my_x = self->get_global_position().x;
			if (player_x < my_x) sprite->set_frame(0); 
			else sprite->set_frame(1); 
		}
	}
}

void OnPhysicsProcess(Caller* instance, Variant& delta_v)
{
	CharacterBody2D* self = GetSelf<CharacterBody2D>(instance);
	if (!self) return;

	// Halt all processing if the game is paused (by Dialogue Manager)
	if (self->get_tree()->is_paused()) {
		AudioStreamPlayer2D* sfx = Object::cast_to<AudioStreamPlayer2D>(self->get_node_or_null(NodePath("FootstepSFX")));
		if (sfx) sfx->stop();
		return;
	}


	bool triggered = (bool)self->get_meta("is_triggered", false);
	if (!triggered) return; 

	// 1. DELAY TIMER
	Timer* delay_timer = Object::cast_to<Timer>(self->get_node_or_null(NodePath("DelayTimer")));
	if (delay_timer) {
		if (!delay_timer->is_stopped()) {
			Node2D* icon = Object::cast_to<Node2D>(self->get_node_or_null(NodePath("AlertIcon")));
			if (icon) {
				float t = delay_timer->get_time_left();
				bool blink_on = (int)(t * 20.0f) % 2 == 0; 
				icon->set_visible(blink_on);
			}
			// Stop footsteps while blinking/delayed
			AudioStreamPlayer2D* sfx = Object::cast_to<AudioStreamPlayer2D>(self->get_node_or_null(NodePath("FootstepSFX")));
			if (sfx) sfx->stop();
			return; // STOP MOVEMENT AND CHASE LOGIC
		}
	}

	// 2. ACTIVE CHASE START (After delay)
	Node2D* icon = Object::cast_to<Node2D>(self->get_node_or_null(NodePath("AlertIcon")));
	if (icon) icon->set_visible(false);

	Timer* stuck_timer = Object::cast_to<Timer>(self->get_node_or_null(NodePath("StuckCheckTimer")));
	if (stuck_timer && stuck_timer->is_stopped()) {
		stuck_timer->start(); 
		self->set_meta("last_pos", self->get_global_position()); 
	}

	Object* target_obj = self->get_meta("target_node");
	Node2D* player = Object::cast_to<Node2D>(target_obj);

	if (!player) {
		SceneTree* tree = self->get_tree();
		if (tree) {
			player = Object::cast_to<Node2D>(tree->get_current_scene()->find_child("SubjectIX", true, false));
			if (player) self->set_meta("target_node", player);
		}
	}

	if (!player) return;

	NavigationAgent2D* nav = Object::cast_to<NavigationAgent2D>(self->get_node_or_null(NodePath("NavigationAgent2D")));
	if (!nav) return;

	nav->set_target_position(player->get_global_position());

	if (nav->is_navigation_finished()) {
		// Stop footsteps when standing still
		AudioStreamPlayer2D* sfx = Object::cast_to<AudioStreamPlayer2D>(self->get_node_or_null(NodePath("FootstepSFX")));
		if (sfx) sfx->stop();
		return;
	}


	Vector2 current_pos = self->get_global_position();
	Vector2 next_path_pos = nav->get_next_path_position();
	Vector2 direction = (next_path_pos - current_pos).normalized();
	
	self->set_velocity(direction * CHASE_SPEED);
	self->move_and_slide();

	// --- SFX FIX: START FOOTSTEPS IF MOVING ---
	AudioStreamPlayer2D* sfx = Object::cast_to<AudioStreamPlayer2D>(self->get_node_or_null(NodePath("FootstepSFX")));
	if (sfx && !sfx->is_playing()) {
		// Ensure the sound is set to LOOP in the editor!
		sfx->play();
	}
	// ------------------------------------------

	AnimatedSprite2D* sprite = Object::cast_to<AnimatedSprite2D>(self->get_node_or_null(NodePath("AnimatedSprite2D")));
	if (sprite) {
		if (direction.x < 0) sprite->play("walk_left");
		else if (direction.x > 0) sprite->play("walk_right");
		else sprite->play("idle"); // If moving vertically, keep the walk animation going
	}

	float dist_to_player = current_pos.distance_to(player->get_global_position());
	if (dist_to_player < DAMAGE_RADIUS) {
		if (player->has_method("take_damage")) {
			player->call("take_damage", self->get_global_position());
		}
	}
}

JENOVA_SCRIPT_END
