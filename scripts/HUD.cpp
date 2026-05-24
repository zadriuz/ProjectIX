// ============================================
// HUD.cpp - Final Production (Position & Crash Fix)
// ============================================
#include <Godot/godot.hpp>
#include <Godot/classes/canvas_layer.hpp>
#include <Godot/classes/label.hpp>
#include <Godot/classes/node.hpp>
#include <Godot/classes/node2d.hpp>
#include <Godot/classes/animated_sprite2d.hpp>
#include <Godot/classes/texture_progress_bar.hpp>
#include <Godot/classes/scene_tree.hpp>
#include <Godot/classes/window.hpp>
#include <Godot/variant/variant.hpp>
#include <Godot/classes/texture_button.hpp>
#include <Godot/classes/animation_player.hpp>
#include <Godot/classes/control.hpp>
#include <Godot/classes/audio_stream_player.hpp>
#include <Godot/classes/texture_rect.hpp>
#include <Godot/classes/canvas_item.hpp>
#include <string>
#include <functional>

using namespace godot;
using namespace jenova::sdk;

// Global State
int capsules_remaining = 0;
int total_capsules_at_start = 0;

// SAFE CACHE
static std::string cached_settings_path = "";

// Audio Global Cache
AudioStreamPlayer* hud_sfx_hover = nullptr;
AudioStreamPlayer* hud_sfx_click = nullptr;

JENOVA_SCRIPT_BEGIN

// Forward Declarations
void _on_settings_clicked(Caller* instance);
void _on_settings_hover(Caller* instance);
void _on_resume_clicked(Caller* instance);
void TogglePauseMenu(Control* self, bool open);

void _ready(Caller* instance)
{
	CanvasLayer* hud = GetSelf<CanvasLayer>(instance);
	if (!hud) return;

	hud->set_process_mode(Node::PROCESS_MODE_ALWAYS);


	// --- GET AUDIO PLAYERS ---
	hud_sfx_hover = Object::cast_to<AudioStreamPlayer>(hud->find_child("SFX_Hover", true, false));
	hud_sfx_click = Object::cast_to<AudioStreamPlayer>(hud->find_child("SFX_Click", true, false));

	// --- CAPSULE LOGIC ---
	SceneTree* tree = hud->get_tree();
	Node* capsule_container = tree->get_root()->find_child("Capsules", true, false);
	if (capsule_container) {
		capsules_remaining = capsule_container->get_child_count();
		total_capsules_at_start = capsules_remaining;
		
	} else {
		capsules_remaining = 250;
		total_capsules_at_start = 250;
		
	}

	// --- HARD MODE HUD LOGIC ---
	Node* current_scene = tree->get_current_scene();
	if (current_scene && String(current_scene->get_name()) == "HardLevel") {
		
		
		Control* vignette = Object::cast_to<Control>(hud->find_child("MiniMapVignette", true, false));
		if (vignette) {
			vignette->set_visible(true);
		}

		std::function<void(Node*, int, bool)> recursive_set_layer = 
			[&](Node* root, int bit, bool value) {
				if (!root) return;
				CanvasItem* item = Object::cast_to<CanvasItem>(root);
				if (item) item->set_visibility_layer_bit(bit, value);
				TypedArray<Node> kids = root->get_children();
				for (int i = 0; i < kids.size(); i++) {
					recursive_set_layer(Object::cast_to<Node>(kids[i]), bit, value);
				}
			};

		if (capsule_container) {
			TypedArray<Node> caps = capsule_container->get_children();
			for(int i=0; i<caps.size(); i++) {
				recursive_set_layer(Object::cast_to<Node>(caps[i]), 1, false);
			}
		}

		Node* charger = tree->get_root()->find_child("TheCharger", true, false);
		if (charger) {
			recursive_set_layer(charger, 1, false);
			Node* icon = charger->find_child("MapIcon", true, false);
			if (icon) {
				CanvasItem* icon_item = Object::cast_to<CanvasItem>(icon);
				if (icon_item) {
					icon_item->set_visible(true);
					icon_item->set_visibility_layer_bit(0, false);
					icon_item->set_visibility_layer_bit(1, true);
				}
			}
		}
	}

	Label* counter = Object::cast_to<Label>(hud->get_node_or_null(NodePath("Counter")));
	if (counter) counter->set_text("x" + String::num_int64(capsules_remaining));

	AnimatedSprite2D* shield_bar = Object::cast_to<AnimatedSprite2D>(hud->get_node_or_null(NodePath("ShieldBar")));
	if (shield_bar) shield_bar->set_frame(0);

	AnimatedSprite2D* cap_bar = Object::cast_to<AnimatedSprite2D>(hud->get_node_or_null(NodePath("CapsuleBar")));
	if (cap_bar) cap_bar->set_frame(0);

	Label* power_label = Object::cast_to<Label>(hud->get_node_or_null(NodePath("PowerUpActive")));
	if (power_label) power_label->set_visible(false);

	Control* sett_btn = Object::cast_to<Control>(hud->find_child("SettingsBtn", true, false));
	if (sett_btn) {
		cached_settings_path = String(sett_btn->get_path()).utf8().get_data();
		sett_btn->set_process_mode(Node::PROCESS_MODE_ALWAYS);
		sett_btn->set_default_cursor_shape(Control::CURSOR_POINTING_HAND);
		
		if (!sett_btn->is_connected("pressed", Callable(hud, "_on_settings_clicked"))) {
			sett_btn->connect("pressed", Callable(hud, "_on_settings_clicked"));
		}

		if (!sett_btn->is_connected("mouse_entered", Callable(hud, "_on_settings_hover"))) {
			sett_btn->connect("mouse_entered", Callable(hud, "_on_settings_hover"));
		}
	}

	Node* root = tree->get_root();
	Node* pause_layer_node = root->find_child("PauseLayer", true, false);
	
	if (pause_layer_node) {
		Control* resume_btn = Object::cast_to<Control>(pause_layer_node->find_child("resumeBtn", true, false));
		if (!resume_btn) resume_btn = Object::cast_to<Control>(pause_layer_node->find_child("ResumeBtn", true, false));
		if (!resume_btn) resume_btn = Object::cast_to<Control>(pause_layer_node->find_child("ResumeButton", true, false));

		if (resume_btn) {
			resume_btn->set_process_mode(Node::PROCESS_MODE_ALWAYS);
			if (!resume_btn->is_connected("pressed", Callable(hud, "_on_resume_clicked"))) {
				resume_btn->connect("pressed", Callable(hud, "_on_resume_clicked"));
			}
		}
		
		CanvasLayer* pl = Object::cast_to<CanvasLayer>(pause_layer_node);
		if (pl) pl->set_visible(false);
	}
}

void _on_settings_hover(Caller* instance)
{
	if (hud_sfx_hover) {
		hud_sfx_hover->play();
	}
}

void _on_settings_clicked(Caller* instance)
{
	if (hud_sfx_click) {
		hud_sfx_click->play();
	}
	Control* self = GetSelf<Control>(instance);
	TogglePauseMenu(self, true);
}

void _on_resume_clicked(Caller* instance)
{
	if (hud_sfx_click) {
		hud_sfx_click->play();
	}
	Control* self = GetSelf<Control>(instance);
	TogglePauseMenu(self, false);
}

void TogglePauseMenu(Control* self, bool open)
{
	SceneTree* tree = self->get_tree();
	Node* root = tree->get_root();
	
	CanvasLayer* pause_layer = Object::cast_to<CanvasLayer>(root->find_child("PauseLayer", true, false));
	if (pause_layer) {
		pause_layer->set_visible(open);
		
		AnimationPlayer* anim = Object::cast_to<AnimationPlayer>(pause_layer->find_child("AnimationPlayer", true, false));
		if (anim) {
			if (open) anim->play("OpenSettings");
			else anim->play("RESET");
		}
	}

	if (!cached_settings_path.empty()) {
		NodePath path = NodePath(String(cached_settings_path.c_str()));
		Control* btn = Object::cast_to<Control>(self->get_node_or_null(path));
		if (btn) {
			btn->set_visible(!open);
		} else {
			Control* fallback = Object::cast_to<Control>(self->find_child("SettingsBtn", true, false));
			if (fallback) fallback->set_visible(!open);
		}
	} else {
		Control* fallback = Object::cast_to<Control>(self->find_child("SettingsBtn", true, false));
		if (fallback) fallback->set_visible(!open);
	}

	if (open) {
		tree->set("paused", true);
	} else {
		tree->set("paused", false);
	}
}

// --- STANDARD GAMEPLAY LOGIC ---
void update_lives(Caller* instance, int lives) {
	CanvasLayer* hud = GetSelf<CanvasLayer>(instance);
	if (!hud) return;
	AnimatedSprite2D* shield_bar = Object::cast_to<AnimatedSprite2D>(hud->get_node_or_null(NodePath("ShieldBar")));
	if (shield_bar) {
		int frame = (lives >= 3) ? 0 : (lives == 2) ? 1 : (lives == 1) ? 2 : 3;
		shield_bar->set_frame(frame);
	}
}

void decrease_capsule_count(Caller* instance) {
	capsules_remaining -= 1;
	if (capsules_remaining < 0) capsules_remaining = 0;
	
	CanvasLayer* hud = GetSelf<CanvasLayer>(instance);
	if (!hud) return;
	
	Label* counter = Object::cast_to<Label>(hud->get_node_or_null(NodePath("Counter")));
	if (counter) counter->set_text("x" + String::num_int64(capsules_remaining));
	
	AnimatedSprite2D* cap_bar = Object::cast_to<AnimatedSprite2D>(hud->get_node_or_null(NodePath("CapsuleBar")));
	if (cap_bar) {
		float collected = (float)(total_capsules_at_start - capsules_remaining);
		float percent = (collected / (float)total_capsules_at_start) * 100.0f;
		if (total_capsules_at_start == 0) percent = 0.0f;
		
		int frame = (percent >= 100.0f) ? 5 : (percent >= 80.0f) ? 4 : (percent >= 60.0f) ? 3 : (percent >= 40.0f) ? 2 : (percent >= 20.0f) ? 1 : 0;
		if (cap_bar->get_frame() != frame) cap_bar->set_frame(frame);
	}

	// --- CLONE ACTIVATION ---
	int collected_count = total_capsules_at_start - capsules_remaining;
	String clone_to_activate = "";
	if (collected_count == 1) clone_to_activate = "Clone1";
	else if (collected_count == 2) clone_to_activate = "Clone2";
	else if (collected_count == 3) clone_to_activate = "Clone3";

	if (clone_to_activate != "") {
		Node* clone = hud->get_tree()->get_root()->find_child(clone_to_activate, true, false);
		if (clone) {
			clone->set_process_mode(Node::PROCESS_MODE_INHERIT);
			Node2D* clone_2d = Object::cast_to<Node2D>(clone);
			if (clone_2d) clone_2d->set_visible(true);
		}
	}

	if (capsules_remaining == 0) {
		// FIX 1: Use get_current_scene() instead of get_root()
		// This prevents finding the wrong "Portal" node or a cached/hidden one.
		Node* scene = hud->get_tree()->get_current_scene();
		if (!scene) return;
		
		// --- PORTAL ACTIVATION ---
		Node* portal = scene->find_child("Portal", true, false);
		
		if (portal) {
			if (portal->has_method("activate")) portal->call("activate");
			
			Node* player = scene->find_child("SubjectIX", true, false);
			if (player) {
				Node2D* p2d = Object::cast_to<Node2D>(portal);
				// FIX 2: Ensure we got a valid Node2D before getting position
				Vector2 tpos = (p2d) ? p2d->get_global_position() : Vector2(0,0);
				
				
				
				// FIX 3: Use call_deferred to prevent Access Violation Crash
				// FIX 4: Explicit Variant wrapper to prevent marshalling errors
				player->call_deferred("enable_guide", Variant(tpos));
				player->call_deferred("play_portal_sfx", Variant(tpos));
				
			}
		}
	}
}

JENOVA_SCRIPT_END
