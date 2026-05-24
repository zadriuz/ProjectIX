// ============================================
// charger.cpp - THE CHARGER (WALK/RUSH + TP FIX)
// ============================================
#include <Godot/godot.hpp>
#include <Godot/classes/character_body2d.hpp>
#include <Godot/classes/navigation_agent2d.hpp>
#include <Godot/classes/node2d.hpp>
#include <Godot/classes/engine.hpp>
#include <Godot/classes/animated_sprite2d.hpp>
#include <Godot/classes/navigation_server2d.hpp> 
#include <Godot/classes/timer.hpp> 
#include <Godot/classes/scene_tree.hpp> 
#include <Godot/classes/window.hpp>     
#include <Godot/variant/variant.hpp>
#include <Godot/variant/node_path.hpp>
#include <Godot/variant/string.hpp>
#include <Godot/classes/audio_stream_player2d.hpp>
#include <Godot/classes/color_rect.hpp> 
#include <Godot/classes/kinematic_collision2d.hpp> 
#include <cmath> 
#include <cstdlib> 

using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN

JENOVA_PROPERTY(NodePath, player_path, NodePath(""));

// CHARGER SETTINGS
static float speed_walk = 130.0f;
static float speed_rush = 300.0f;
static float current_speed = 130.0f;
static bool is_rushing = false;
static float TELEPORT_RADIUS = 250.0f; 
static float DAMAGE_RANGE = 45.0f;      
static float MIN_TELEPORT_DISTANCE = 150.0f; 

// AUDIO PITCH SETTINGS
static constexpr float PITCH_WALK = 1.0f; // Normal pitch
static constexpr float PITCH_RUSH = 4.0f; // Increased pitch for rushing (was 2.0f)

static int physics_frame_tick = 0;
static bool has_started = false; 

// Forward declaration for the timer callback
void _on_state_timer_timeout(Caller* instance);
void _on_stuck_check_timeout(Caller* instance); 
void _on_delay_timeout(Caller* instance);      

void _ready(Caller* instance)
{
	CharacterBody2D* self = GetSelf<CharacterBody2D>(instance);
	if (!self) return;

	self->set_motion_mode(CharacterBody2D::MOTION_MODE_FLOATING);

	// --- METADATA FOR STUCK CHECK ---
	self->set_meta("stuck_timer_counter", 0.0f);
	self->set_meta("last_stuck_pos", self->get_global_position());
	has_started = false;

	// --- PHYSICS SETUP ---
	self->set_collision_layer_value(4, true); 
	self->set_collision_mask(0); 
	self->set_collision_mask_value(1, true); // Mask Walls/World (Layer 1)

	// --- TIMER SETUP ---
	Timer* stuck_check_timer = self->get_node<Timer>(NodePath("StuckCheckTimer"));
	Timer* delay_timer = self->get_node<Timer>(NodePath("DelayTimer"));

	if (stuck_check_timer) {
		stuck_check_timer->set_process_mode(Node::PROCESS_MODE_INHERIT); 
		stuck_check_timer->set_wait_time(0.5); 
		stuck_check_timer->set_autostart(false);
		if (!stuck_check_timer->is_connected("timeout", Callable(self, "_on_stuck_check_timeout"))) {
			stuck_check_timer->connect("timeout", Callable(self, "_on_stuck_check_timeout"));
		}
		
	} 
	if (delay_timer) {
		delay_timer->set_one_shot(true);
		if (!delay_timer->is_connected("timeout", Callable(self, "_on_delay_timeout"))) {
			delay_timer->connect("timeout", Callable(self, "_on_delay_timeout"));
		}
	}

	// 1. SETUP NAVIGATION
	NavigationAgent2D* nav = self->get_node<NavigationAgent2D>(NodePath("NavigationAgent2D"));
	if (nav) {
		nav->set_target_desired_distance(20.0f);
		nav->set_path_desired_distance(12.0f);
		nav->set_radius(65.0f); 
	}

	// 2. SETUP STATE TIMER (Walk 5s / Rush 2.5s)
	Timer* state_timer = memnew(Timer);
	state_timer->set_name("StateTimer");
	state_timer->set_one_shot(true);
	state_timer->connect("timeout", Callable(self, "_on_state_timer_timeout"));
	self->add_child(state_timer);
	
	// Initial State: Walk
	is_rushing = false;
	current_speed = speed_walk;

	// 3. AUDIO START
	AudioStreamPlayer2D* sfx = self->get_node<AudioStreamPlayer2D>(NodePath("FootstepSFX"));
	if (sfx) {
		// Set initial pitch to walk speed
		sfx->set_pitch_scale(PITCH_WALK); 
		sfx->play();
	}

	// --- MINIMAP ICON SETUP ---
	ColorRect* minimap_icon = Object::cast_to<ColorRect>(self->find_child("MapIcon", true, false)); 
	if (minimap_icon) {
		minimap_icon->set_visibility_layer_bit(0, false); 
		minimap_icon->set_visibility_layer_bit(1, true);  
		
	}
	
	
}

void _on_stuck_check_timeout(Caller* instance) {
	CharacterBody2D* self = GetSelf<CharacterBody2D>(instance);
	if (!self) return;

	Vector2 last_pos = (Vector2)self->get_meta("last_stuck_pos", self->get_global_position());
	Vector2 current_pos = self->get_global_position();
	float moved_distance = current_pos.distance_to(last_pos);

	float stuck_counter = (float)self->get_meta("stuck_timer_counter", 0.0f);
	
	if (moved_distance < 5.0f) {
		stuck_counter += 0.5f;
	} else {
		stuck_counter = 0.0f;
	}
	
	// --- DEBUG LOGGING ---
	
			

	self->set_meta("last_stuck_pos", current_pos);
	self->set_meta("stuck_timer_counter", stuck_counter);

	if (stuck_counter >= 5.0f) {
		
		
		self->set_meta("stuck_timer_counter", 0.0f);
		
		Timer* delay_timer = self->get_node<Timer>(NodePath("DelayTimer"));

		// DISABLE MOVEMENT
		current_speed = 0.0f;
		
		// Start the 1.0s delay timer before teleporting
		if (delay_timer) delay_timer->start(1.0);
	}
}

void _on_delay_timeout(Caller* instance) {
	CharacterBody2D* self = GetSelf<CharacterBody2D>(instance);
	if (!self) return;

	Node2D* player = Object::cast_to<Node2D>(self->get_tree()->get_root()->find_child("SubjectIX", true, false));
	if (!player) return;

	// --- TELEPORT LOGIC ---
	Vector2 safe_pos;
	NavigationAgent2D* nav = self->get_node<NavigationAgent2D>(NodePath("NavigationAgent2D"));

	if (nav) {
		RID map = nav->get_navigation_map();
		Vector2 target_pos;
		float current_teleport_radius = TELEPORT_RADIUS;
		int attempts = 0;
		const int MAX_ATTEMPTS = 10;
		
		do {
			float random_val = (float)std::rand() / RAND_MAX; 
			float angle = random_val * 6.283185f;
			
			Vector2 offset = Vector2(std::cos(angle), std::sin(angle)) * current_teleport_radius;
			target_pos = player->get_global_position() + offset;
			
			safe_pos = NavigationServer2D::get_singleton()->map_get_closest_point(map, target_pos);
			
			current_teleport_radius *= 0.95f; 
			attempts++;
		} while (safe_pos.distance_to(player->get_global_position()) < MIN_TELEPORT_DISTANCE && attempts < MAX_ATTEMPTS);


		self->set_global_position(safe_pos);
		self->set_meta("last_stuck_pos", safe_pos);
		
	} else {
		
	}

	// Resume movement at current state speed (walk or rush)
	current_speed = is_rushing ? speed_rush : speed_walk;
	
}

void _on_state_timer_timeout(Caller* instance)
{
	CharacterBody2D* self = GetSelf<CharacterBody2D>(instance);
	if (!self) return;

	// Dialogue Manager handles pausing here via is_paused() check in _physics_process
	if (self->get_tree()->is_paused()) {
		
		Timer* t = Object::cast_to<Timer>(self->find_child("StateTimer", false, false));
		if (t) t->start(0.1); 
		return;
	}

	// Get SFX reference
	AudioStreamPlayer2D* sfx = Object::cast_to<AudioStreamPlayer2D>(self->get_node_or_null(NodePath("FootstepSFX")));

	Timer* t = Object::cast_to<Timer>(self->find_child("StateTimer", false, false));
	
	if (is_rushing) {
		is_rushing = false;
		current_speed = speed_walk;
		// Set pitch back to normal walk pitch
		if (sfx) sfx->set_pitch_scale(PITCH_WALK); 
		if(t) t->start(5.0); 
	} else {
		is_rushing = true;
		current_speed = speed_rush;
		// Increase pitch for rushing
		if (sfx) sfx->set_pitch_scale(PITCH_RUSH); 
		if(t) t->start(2.5); 
	}
}

void _physics_process(Caller* instance, Variant& delta)
{
	if (Engine::get_singleton()->is_editor_hint()) return;

	CharacterBody2D* self = GetSelf<CharacterBody2D>(instance);
	if (!self) return;

	// --- INITIAL START CHECK (Runs when Dialogue is finished) ---
	if (!has_started && !self->get_tree()->is_paused()) {
		
		
		Timer* stuck_check_timer = self->get_node<Timer>(NodePath("StuckCheckTimer"));
		Timer* state_timer = Object::cast_to<Timer>(self->find_child("StateTimer", false, false));
		
		if (stuck_check_timer) stuck_check_timer->start();
		if (state_timer) state_timer->start(5.0); // Initial 5s walk state
		
		has_started = true;
	}


	// --- PAUSE CHECK (CRITICAL) ---
	if (self->get_tree()->is_paused()) {
		self->set_velocity(Vector2(0, 0));
		self->move_and_slide();
		AnimatedSprite2D* sprite = self->get_node<AnimatedSprite2D>(NodePath("AnimatedSprite2D"));
		if (sprite && sprite->is_playing()) sprite->stop();
		return;
	}
	// -----------------------------


	NavigationAgent2D* nav = self->get_node<NavigationAgent2D>(NodePath("NavigationAgent2D"));
	if (!nav) return;

	// Resolve Player
	Node2D* player = nullptr;
	if (!player_path.is_empty()) {
		player = Object::cast_to<Node2D>(self->get_node_or_null(player_path));
	}
	if (!player) {
		player = Object::cast_to<Node2D>(self->get_tree()->get_root()->find_child("SubjectIX", true, false));
	}
	if (!player) return;

	physics_frame_tick++;

	// 1. UPDATE PATH
	if (physics_frame_tick % 15 == 0) {
		nav->set_target_position(player->get_global_position());
	}

	// 2. MOVEMENT
	if (nav->is_navigation_finished() || current_speed == 0.0f) {
		self->set_velocity(Vector2(0, 0));
		self->move_and_slide();
		AnimatedSprite2D* sprite = self->get_node<AnimatedSprite2D>(NodePath("AnimatedSprite2D"));
		if (sprite && sprite->is_playing()) sprite->stop();
	} 
	else {
		Vector2 current_pos = self->get_global_position(); 
		Vector2 next_pos = nav->get_next_path_position();
		Vector2 direction = (next_pos - current_pos).normalized();
		Vector2 velocity = direction * current_speed;

		// 3. ANIMATION
		AnimatedSprite2D* sprite = self->get_node<AnimatedSprite2D>(NodePath("AnimatedSprite2D"));
		if (sprite) {
			if (direction.x < -0.1f) {
				sprite->play("walk_left");
			} else if (direction.x > 0.1f) {
				sprite->play("walk_right");
			} else {
				if (!sprite->is_playing()) sprite->play("walk_right"); 
			}
			float anim_speed = is_rushing ? 2.0f : 1.0f;
			sprite->set_speed_scale(anim_speed);
		}

		self->set_velocity(velocity);
		self->move_and_slide();
	}

	// 4. ATTACK CHECK (Mathematical Distance Check)
	float dist_to_player = self->get_global_position().distance_to(player->get_global_position());
	
	if (dist_to_player < DAMAGE_RANGE) {
		if (player->has_method("take_damage")) {
			player->call("take_damage", self->get_global_position());
		}
	}
}

JENOVA_SCRIPT_END
