// ============================================
// subject_ix.cpp - Fixed Minimap & Input Logic
// ============================================
#include <Godot/godot.hpp>
#include <Godot/classes/timer.hpp>
#include <Godot/classes/node.hpp>
#include <Godot/classes/input.hpp>
#include <Godot/classes/character_body2d.hpp>
#include <Godot/classes/animated_sprite2d.hpp>
#include <Godot/classes/canvas_layer.hpp>
#include <Godot/classes/label.hpp>
#include <Godot/classes/node2d.hpp>
#include <Godot/classes/scene_tree.hpp>
#include <Godot/classes/window.hpp>

// MINIMAP & ANIMATION INCLUDES
#include <Godot/classes/viewport.hpp>
#include <Godot/classes/sub_viewport.hpp>
#include <Godot/classes/world2d.hpp>
#include <Godot/classes/camera2d.hpp>
#include <Godot/classes/animation_player.hpp>

// Retaining necessary headers for logic
#include <Godot/classes/engine.hpp>
#include <Godot/classes/time.hpp>
#include <Godot/classes/audio_stream_player2d.hpp>
#include <Godot/classes/audio_stream_player.hpp>
#include <string>
#include <cmath>

using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN

JENOVA_PROPERTY(int, base_speed, 200);

int current_lives = 3;
bool is_immune = false;
int current_speed = 200;
static int last_horizontal_dir = 1;
bool guide_active = false;
Vector2 portal_pos = Vector2(0,0);
bool is_shield_active = false;
bool is_dead = false;

// NEW STATE VARIABLES
bool is_minimap_visible = false;
bool is_minimap_busy = false;

// Forward declarations
void reset_effects(Caller* instance);
void _on_immunity_timer_timeout(Caller* instance);
void _on_death_timeout(Caller* instance);
void _on_cooldown_timer_timeout(Caller* instance);
void play_portal_sfx(Caller* instance, Vector2 target);

void _ready(Caller* instance)
{
	CharacterBody2D* self = GetSelf<CharacterBody2D>(instance);
	if (!self) return;

	if (self->get_viewport()) {
		self->get_viewport()->set_canvas_cull_mask(17);
	}
	
	// --- MINIMAP WORLD LINK & TIMER CONNECTIONS ---
	SceneTree* tree = self->get_tree();
	if (tree) {
		Node* hud = tree->get_root()->find_child("HUD", true, false);
		if (hud) {
			// FIX: Use recursive find_child to locate container inside Anchor
			Node* sub_vp_container = hud->find_child("SubViewportContainer", true, false);
			
			if (sub_vp_container) {
				SubViewport* map_vp = Object::cast_to<SubViewport>(sub_vp_container->find_child("SubViewport", true, false));
				if (map_vp) {
					map_vp->set_world_2d(self->get_viewport()->get_world_2d());
				}
			}
			
			// FIX: Recursively find timer (it might be inside anchor now too)
			Timer* cooldown_timer = Object::cast_to<Timer>(hud->find_child("CooldownTimer", true, false));
			if (cooldown_timer) {
				if (!cooldown_timer->is_connected("timeout", Callable(self, "_on_cooldown_timer_timeout"))) {
					cooldown_timer->connect("timeout", Callable(self, "_on_cooldown_timer_timeout"));
				}
			}
		}
	}

	current_speed = base_speed;
	self->set_motion_mode(CharacterBody2D::MOTION_MODE_FLOATING);
	self->set_meta("is_immune", false);

	Timer* im_timer = Object::cast_to<Timer>(self->get_node_or_null(NodePath("ImmunityTimer")));
	if (im_timer) {
		if (!im_timer->is_connected("timeout", Callable(self, "_on_immunity_timer_timeout"))) {
			im_timer->connect("timeout", Callable(self, "_on_immunity_timer_timeout"));
		}
	}
	
	Timer* eff_timer = Object::cast_to<Timer>(self->get_node_or_null(NodePath("EffectTimer")));
	if (eff_timer) {
		if (!eff_timer->is_connected("timeout", Callable(self, "reset_effects"))) {
			eff_timer->connect("timeout", Callable(self, "reset_effects"));
		}
	}
	
	AudioStreamPlayer2D* portal_sfx = Object::cast_to<AudioStreamPlayer2D>(self->get_node_or_null(NodePath("PortalSFX")));
	if (portal_sfx) {
		portal_sfx->set_as_top_level(true);
	}
	
	current_lives = 3;
	is_immune = false;
	is_shield_active = false;
	guide_active = false;
	is_dead = false;
	
	
}

void activate_speed_boost(Caller* instance)
{
	CharacterBody2D* self = GetSelf<CharacterBody2D>(instance);
	if (!self || is_dead) return;
	current_speed = 320;

	AudioStreamPlayer2D* sfx = Object::cast_to<AudioStreamPlayer2D>(self->get_node_or_null(NodePath("PowerUpSFX")));
	if (sfx) sfx->play();

	SceneTree* tree = self->get_tree();
	if (tree) {
		Node* hud = tree->get_root()->find_child("HUD", true, false);
		if (hud) {
			Label* lbl = Object::cast_to<Label>(hud->find_child("PowerUpActive", true, false));
			if (lbl) {
				lbl->set_text("Speed Boost Active!");
				lbl->set_visible(true);
			}
		}
	}
	Timer* eff_timer = Object::cast_to<Timer>(self->get_node_or_null(NodePath("EffectTimer")));
	if (eff_timer) { eff_timer->set_wait_time(5.0); eff_timer->set_one_shot(true); eff_timer->start(); }
}

void activate_immunity(Caller* instance)
{
	CharacterBody2D* self = GetSelf<CharacterBody2D>(instance);
	if (!self || is_dead) return;
	is_immune = true; is_shield_active = true; self->set_meta("is_immune", true);

	AudioStreamPlayer2D* sfx = Object::cast_to<AudioStreamPlayer2D>(self->get_node_or_null(NodePath("PowerUpSFX")));
	if (sfx) sfx->play();

	SceneTree* tree = self->get_tree();
	if (tree) {
		Node* hud = tree->get_root()->find_child("HUD", true, false);
		if (hud) {
			Label* lbl = Object::cast_to<Label>(hud->find_child("PowerUpActive", true, false));
			if (lbl) {
				lbl->set_text("Immunity Shield Active!");
				lbl->set_visible(true);
			}
		}
	}
	Timer* eff_timer = Object::cast_to<Timer>(self->get_node_or_null(NodePath("EffectTimer")));
	if (eff_timer) { eff_timer->set_wait_time(5.0); eff_timer->set_one_shot(true); eff_timer->start(); }
}

void reset_effects(Caller* instance)
{
	if (is_dead) { _on_death_timeout(instance); return; }
	CharacterBody2D* self = GetSelf<CharacterBody2D>(instance);
	current_speed = base_speed;
	Timer* hurt_timer = Object::cast_to<Timer>(self->get_node_or_null(NodePath("ImmunityTimer")));
	if (hurt_timer && hurt_timer->is_stopped()) {
		is_immune = false; is_shield_active = false; self->set_meta("is_immune", false);
	}
}

void OnPhysicsProcess(Caller* instance, Variant& _delta)
{
	CharacterBody2D* self = GetSelf<CharacterBody2D>(instance);
	if (!self || is_dead) return;

	Input* input = godot::Input::get_singleton();
	
	// --- MINIMAP CAMERA FOLLOW & INPUT TOGGLE LOGIC ---
	SceneTree* tree = self->get_tree();
	Node* hud = tree->get_root()->find_child("HUD", true, false);

	if (hud) {
		// A. Camera Follow Logic
		// FIX: Use recursive find_child to locate container inside Anchor
		Node* sub_vp_container = hud->find_child("SubViewportContainer", true, false);
		
		if (sub_vp_container) {
			Node* viewport = sub_vp_container->find_child("SubViewport", true, false);
			if (viewport) {
				Camera2D* map_cam = Object::cast_to<Camera2D>(viewport->find_child("MapCam", true, false));
				if (map_cam) {
					map_cam->set_global_position(self->get_global_position());
				}
			}
		}
		
		// B. Input/Slide Logic (Toggle Only)
		if (input->is_action_just_pressed("userInput_M") && !is_minimap_busy) {
			// FIX: Use recursive search for Animator and Timer (DEFINED HERE)
			AnimationPlayer* animator = Object::cast_to<AnimationPlayer>(hud->find_child("MiniMapAnimator", true, false));
			Timer* cooldown_timer = Object::cast_to<Timer>(hud->find_child("CooldownTimer", true, false));

			if (animator && cooldown_timer) {
				is_minimap_busy = true;
				cooldown_timer->start();
				if (!is_minimap_visible) { animator->play("SlideUp"); is_minimap_visible = true; }
				else { animator->play("SlideDown"); is_minimap_visible = false; }
			} 
		}
	}
	
	if (!input) return;

	// --- VISUALS/HUD UPDATES ---
	Timer* eff_timer = Object::cast_to<Timer>(self->get_node_or_null(NodePath("EffectTimer")));
	if (hud && eff_timer) {
		Label* lbl = Object::cast_to<Label>(hud->find_child("PowerUpActive", true, false));
		if (lbl) {
			if (!eff_timer->is_stopped()) {
				double time = godot::Time::get_singleton()->get_ticks_msec() / 1000.0;
				float r = (std::sin(time * 5.0f) + 1.0f) / 2.0f;
				float g = (std::sin(time * 5.0f + 2.0f) + 1.0f) / 2.0f;
				float b = (std::sin(time * 5.0f + 4.0f) + 1.0f) / 2.0f;
				
				if (eff_timer->get_time_left() < 1.5) {
					bool on = ((int)(eff_timer->get_time_left() * 10.0) % 2 == 0);
					lbl->set_visible(on); if (on) lbl->set_modulate(Color(1, 0, 0));
				} else {
					lbl->set_visible(true); lbl->set_modulate(Color(r, g, b));
				}
			} else { lbl->set_visible(false); }
		}
	}

	AnimatedSprite2D* sprite = self->get_node<AnimatedSprite2D>(NodePath("AnimatedSprite2D"));
	if (sprite) {
		if (is_immune) {
			float opacity = (godot::Engine::get_singleton()->get_process_frames() % 10 < 5) ? 0.5f : 1.0f;
			if (is_shield_active) sprite->set_modulate(Color(0.5, 0.5, 1.0, opacity));
			else sprite->set_modulate(Color(1, 0, 0, opacity));
		} else { sprite->set_modulate(Color(1, 1, 1, 1)); }
	}

	Vector2 direction(0, 0);
	if (input->is_action_pressed("ui_left"))	 direction.x -= 1;
	if (input->is_action_pressed("ui_right")) direction.x += 1;
	if (input->is_action_pressed("ui_up"))	 	 direction.y -= 1;
	if (input->is_action_pressed("ui_down"))	 direction.y += 1;

	if (direction.length() > 0) direction = direction.normalized();
	Vector2 vel = direction * (float)current_speed;

	if (sprite) {
		if (direction.x < 0) { last_horizontal_dir = -1; sprite->play("walk_left"); }
		else if (direction.x > 0) { last_horizontal_dir = 1; sprite->play("walk_right"); }
		else if (direction.length() == 0) {
			if (last_horizontal_dir == 1) sprite->play("idle_right");
			else sprite->play("idle_left");
		} else {
			if (last_horizontal_dir == 1) sprite->play("walk_right");
			else sprite->play("walk_left");
		}
	}

	self->set_velocity(vel);
	self->move_and_slide();
	
	AudioStreamPlayer2D* run_sfx = Object::cast_to<AudioStreamPlayer2D>(self->get_node_or_null(NodePath("RunSFX")));
	if (run_sfx) {
		bool is_moving = direction.length() > 0;
		bool is_playing = run_sfx->is_playing();
		if (is_moving && !is_playing) run_sfx->play();
		else if (!is_moving && is_playing) run_sfx->stop();
	}
}

void play_portal_sfx(Caller* instance, Vector2 target)
{
	CharacterBody2D* self = GetSelf<CharacterBody2D>(instance);
	if (!self) return;

	

	SceneTree* tree = self->get_tree();
	if (!tree) return;

	Node* root = tree->get_root();
	Node* music_node = root->find_child("MusicPlayer", true, false); 
	
	if (music_node) {
		if (music_node->is_class("AudioStreamPlayer")) {
			Object::cast_to<AudioStreamPlayer>(music_node)->stop();
	
		} else if (music_node->is_class("AudioStreamPlayer2D")) {
			Object::cast_to<AudioStreamPlayer2D>(music_node)->stop();
		
		}
	} 

	AudioStreamPlayer2D* portal_sfx = Object::cast_to<AudioStreamPlayer2D>(self->get_node_or_null(NodePath("PortalSFX")));
	if (portal_sfx) {
		portal_sfx->set_as_top_level(true);
		portal_sfx->set_global_position(target);
		portal_sfx->set_max_distance(3000.0f); 
		portal_sfx->set_attenuation(1.0f);
		portal_sfx->play();
		String msg = "DEBUG: Portal SFX played at " + String(target) + " (Attenuation: 1.0)";
	
	} else {
		
	}
}

void enable_guide(Caller* instance, Vector2 target)
{
	guide_active = true;
	portal_pos = target;
}

void take_damage(Caller* instance, Vector2 enemy_pos)
{
	if (is_immune || is_dead) return;

	CharacterBody2D* self = GetSelf<CharacterBody2D>(instance);
	current_lives -= 1;
	
	SceneTree* tree = self->get_tree();
	Node* hud = nullptr;
	if (tree) hud = tree->get_root()->find_child("HUD", true, false);
	if (hud) hud->call("update_lives", current_lives);

	if (current_lives > 0) {
		is_immune = true; is_shield_active = false; self->set_meta("is_immune", true);
		
		AudioStreamPlayer2D* sfx = Object::cast_to<AudioStreamPlayer2D>(self->get_node_or_null(NodePath("HurtSFX")));
		if (sfx) sfx->play();

		Vector2 knock_dir = (self->get_global_position() - enemy_pos).normalized();
		self->set_velocity(knock_dir * 500.0f);
		self->move_and_slide();

		Timer* im_timer = Object::cast_to<Timer>(self->get_node_or_null(NodePath("ImmunityTimer")));
		if (im_timer) { im_timer->set_wait_time(4.0); im_timer->start(); }
	} else {
		is_dead = true;
		AnimatedSprite2D* sprite = self->get_node<AnimatedSprite2D>(NodePath("AnimatedSprite2D"));
		if (sprite) sprite->set_visible(false);
		AudioStreamPlayer2D* run_sfx = Object::cast_to<AudioStreamPlayer2D>(self->get_node_or_null(NodePath("RunSFX")));
		if (run_sfx) run_sfx->stop();
		
		AudioStreamPlayer2D* sfx = Object::cast_to<AudioStreamPlayer2D>(self->get_node_or_null(NodePath("ExplosionSFX")));
		if (sfx) sfx->play();
		
		
		
		Timer* eff_timer = Object::cast_to<Timer>(self->get_node_or_null(NodePath("EffectTimer")));
		if (eff_timer) { eff_timer->set_wait_time(2.0); eff_timer->start(); }
		else { _on_death_timeout(instance); }
	}
}

void _on_death_timeout(Caller* instance) {
	GetSelf<CharacterBody2D>(instance)->get_tree()->change_scene_to_file("res://scenes/game_over.tscn");
}

void _on_immunity_timer_timeout(Caller* instance) {
	CharacterBody2D* self = GetSelf<CharacterBody2D>(instance);
	is_immune = false; is_shield_active = false; self->set_meta("is_immune", false);
}

void collect_capsule(Caller* instance) {
	CharacterBody2D* self = GetSelf<CharacterBody2D>(instance);
	if (!self || is_dead) return;
	
	AudioStreamPlayer2D* sfx = Object::cast_to<AudioStreamPlayer2D>(self->get_node_or_null(NodePath("CollectSFX")));
	if (sfx) sfx->play();
	
	SceneTree* tree = self->get_tree();
	Node* hud = nullptr;
	if (tree) hud = tree->get_root()->find_child("HUD", true, false);
	if (hud && hud->has_method("decrease_capsule_count")) hud->call("decrease_capsule_count");
}

void _on_cooldown_timer_timeout(Caller* instance) {
	is_minimap_busy = false;
}

JENOVA_SCRIPT_END
