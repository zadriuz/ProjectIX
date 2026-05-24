// ============================================
// follower.cpp - AMBUSH TELEPORT (FIXED IDLE)
// ============================================
#include <Godot/godot.hpp>
#include <Godot/classes/character_body2d.hpp>
#include <Godot/classes/navigation_agent2d.hpp>
#include <Godot/classes/node2d.hpp>
#include <Godot/classes/engine.hpp>
#include <Godot/classes/animated_sprite2d.hpp>
#include <Godot/classes/navigation_server2d.hpp> 
#include <Godot/classes/timer.hpp> 
#include <Godot/variant/variant.hpp>
#include <Godot/variant/node_path.hpp>
#include <Godot/variant/string.hpp>
#include <Godot/classes/audio_stream_player2d.hpp>
#include <cmath> 
#include <cstdlib> 

using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN

JENOVA_PROPERTY(NodePath, player_path, NodePath(""));

static float speed = 200.0f;
static int physics_frame_tick = 0;

void _ready(Caller* instance)
{
	CharacterBody2D* self = GetSelf<CharacterBody2D>(instance);
	if (!self) return;

	self->set_motion_mode(CharacterBody2D::MOTION_MODE_FLOATING);

	NavigationAgent2D* nav = self->get_node<NavigationAgent2D>(NodePath("NavigationAgent2D"));
	if (!nav) return;

	nav->set_target_desired_distance(20.0f);
	nav->set_path_desired_distance(12.0f);
	nav->set_radius(8.0f);

	self->set_meta("stuck_timer", 0.0f);
	self->set_meta("last_stuck_pos", self->get_global_position());

	// AUDIO START (FIRE AND FORGET)
	AudioStreamPlayer2D* sfx = self->get_node<AudioStreamPlayer2D>(NodePath("FootstepSFX"));
	if (sfx) sfx->play();
	

}

void _physics_process(Caller* instance, Variant& delta)
{
	if (Engine::get_singleton()->is_editor_hint()) return;

	CharacterBody2D* self = GetSelf<CharacterBody2D>(instance);
	if (!self) return;

	// --- 1. CHECK STUN TIMER FIRST ---
	Timer* stun_timer = self->get_node<Timer>(NodePath("StunTimer"));
	if (stun_timer && !stun_timer->is_stopped()) {
		// FIX: Stop playing the non-existent 'idle' animation.
		// Instead, we stop movement and ensure the sprite is not animating.
		AnimatedSprite2D* sprite = self->get_node<AnimatedSprite2D>(NodePath("AnimatedSprite2D"));
		if (sprite) {
			// Stop animation so it freezes on the current frame/pose
			if (sprite->is_playing()) {
				sprite->stop();
			}
		}
		
		// Stop Moving
		self->set_velocity(Vector2(0,0));
		self->move_and_slide();
		return; 
	}
	
	// Resume animation if the timer just stopped (otherwise it might remain stopped)
	AnimatedSprite2D* sprite_res = self->get_node<AnimatedSprite2D>(NodePath("AnimatedSprite2D"));
	if (sprite_res && !sprite_res->is_playing() && stun_timer && stun_timer->is_stopped()) {
		// Play a default animation (like 'walk_left') to resume movement state
		sprite_res->play("walk_left"); 
	}


	NavigationAgent2D* nav = self->get_node<NavigationAgent2D>(NodePath("NavigationAgent2D"));
	if (!nav) return;

	if (player_path.is_empty()) return;

	Node* target_node = self->get_node_or_null(player_path);
	Node2D* player = Object::cast_to<Node2D>(target_node);
	if (!player) return;

	physics_frame_tick++;

	// 2. UPDATE PATH
	if (physics_frame_tick % 15 == 0) {
		nav->set_target_position(player->get_global_position());
	}

	// 3. STUCK DETECTION
	float stuck_counter = (float)self->get_meta("stuck_timer", 0.0f);
	if (physics_frame_tick % 30 == 0) {
		Vector2 last_pos = (Vector2)self->get_meta("last_stuck_pos", self->get_global_position());
		Vector2 current_pos = self->get_global_position();

		if (current_pos.distance_to(last_pos) < 10.0f) stuck_counter += 0.5f;
		else {
			stuck_counter = 0.0f;
			self->set_meta("last_stuck_pos", current_pos);
		}
		self->set_meta("stuck_timer", stuck_counter);
	}

	// 4. TELEPORT FAILSAFE (AMBUSH)
	if (stuck_counter > 5.0f) {
	
		
		float random_val = (float)std::rand() / RAND_MAX; 
		float angle = random_val * 6.283185f; 
		
		// *** THE CHANGE: 120.0f IS VERY CLOSE ***
		Vector2 offset = Vector2(std::cos(angle), std::sin(angle)) * 120.0f;
		Vector2 random_pos = player->get_global_position() + offset;
		
		RID map = nav->get_navigation_map();
		Vector2 safe_pos = NavigationServer2D::get_singleton()->map_get_closest_point(map, random_pos);
		
		self->set_global_position(safe_pos);
		
		// Reset stuck logic
		self->set_meta("stuck_timer", 0.0f);
		self->set_meta("last_stuck_pos", safe_pos);
		
		// *** ACTIVATE STUN TIMER (2 Seconds of Terror) ***
		if (stun_timer) {
			stun_timer->start(); 
		}
		return;
	}

	// 5. MOVEMENT
	if (nav->is_navigation_finished()) {
		self->set_velocity(Vector2(0, 0));
		self->move_and_slide();
		// Stop the sprite if it's finished navigating and is standing still
		AnimatedSprite2D* sprite_stop = self->get_node<AnimatedSprite2D>(NodePath("AnimatedSprite2D"));
		if (sprite_stop && sprite_stop->is_playing()) sprite_stop->stop();
		return;
	}

	Vector2 current_pos = self->get_global_position(); 
	Vector2 next_pos = nav->get_next_path_position();
	Vector2 direction = (next_pos - current_pos).normalized();
	Vector2 velocity = direction * speed;

	AnimatedSprite2D* sprite = self->get_node<AnimatedSprite2D>(NodePath("AnimatedSprite2D"));
	if (sprite && std::abs(direction.x) > 0.3f) {
		if (direction.x < -0.3f) sprite->play("walk_left");
		else if (direction.x > 0.3f) sprite->play("walk_right");
	}

	self->set_velocity(velocity);
	self->move_and_slide();

	// 6. ATTACK CHECK
	float dist_to_player = self->get_global_position().distance_to(player->get_global_position());
	if (dist_to_player < 25.0f) {
		if (player->has_method("take_damage")) {
			player->call("take_damage", self->get_global_position());
		}
	}
}

JENOVA_SCRIPT_END
