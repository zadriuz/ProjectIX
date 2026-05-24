// ============================================
// pause_menu.cpp - With Juicy Audio!
// ============================================
#include <Godot/godot.hpp>
#include <Godot/classes/canvas_layer.hpp>
#include <Godot/classes/texture_button.hpp>
#include <Godot/classes/base_button.hpp>
#include <Godot/classes/button.hpp>
#include <Godot/classes/control.hpp>
#include <Godot/classes/scene_tree.hpp>
#include <Godot/classes/input.hpp>
#include <Godot/classes/audio_server.hpp>
#include <Godot/classes/h_slider.hpp>
#include <Godot/classes/check_button.hpp>
#include <Godot/classes/animation_player.hpp>
#include <Godot/classes/audio_stream_player.hpp> // Required for SFX
#include <Godot/variant/string.hpp>
#include <cmath>

using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN

// UI References
Control* settings_panel = nullptr;
Control* help_panel = nullptr;
AnimationPlayer* anim_player = nullptr;

// Help State
int current_help_page = 1;
int max_help_pages = 5; 

// Forward Declarations
void _on_resume_pressed(Caller* instance);
void _on_leave_pressed(Caller* instance);
void _on_help_pressed(Caller* instance);
void _on_close_help_pressed(Caller* instance);

// Audio & Help Logic
void _on_music_toggled(Caller* instance);
void _on_sfx_toggled(Caller* instance);
void _on_volume_changed(Caller* instance, double value);
void _on_help_next(Caller* instance);
void _on_help_prev(Caller* instance);
void _update_help_ui(Control* self);

// SFX Helpers
void _on_button_hover(Caller* instance);
void _play_click(CanvasLayer* self);

void _ready(Caller* instance)
{
	CanvasLayer* self = GetSelf<CanvasLayer>(instance);
	if (!self) return;

	// --- 1. GET REFERENCES ---
	settings_panel = Object::cast_to<Control>(self->find_child("SettingsPanel", true, false));
	help_panel     = Object::cast_to<Control>(self->find_child("HelpPanel", true, false));
	anim_player    = Object::cast_to<AnimationPlayer>(self->find_child("AnimationPlayer", true, false));

	if (settings_panel) settings_panel->set_visible(false);
	if (help_panel)     help_panel->set_visible(false);
	Control* ov = Object::cast_to<Control>(self->find_child("DimOverlay", true, false));
	if (ov) ov->set_visible(false);

	// --- 2. CONNECT NAVIGATION BUTTONS (With Audio) ---
	auto setup_btn = [&](String name, String callback) {
		BaseButton* btn = Object::cast_to<BaseButton>(self->find_child(name, true, false));
		if (btn) {
			btn->set_default_cursor_shape(Control::CURSOR_POINTING_HAND);
			
			// Connect Click
			if (!btn->is_connected("pressed", Callable(self, callback)))
				btn->connect("pressed", Callable(self, callback));
				
			// Connect Hover Sound (NEW)
			if (!btn->is_connected("mouse_entered", Callable(self, "_on_button_hover")))
				btn->connect("mouse_entered", Callable(self, "_on_button_hover"));
		}
	};

	setup_btn("resumeBtn", "_on_resume_pressed");
	setup_btn("leaveBtn", "_on_leave_pressed");
	setup_btn("helpBtn", "_on_help_pressed");
	setup_btn("HelpNext", "_on_help_next");
	setup_btn("HelpPrev", "_on_help_prev");
	setup_btn("CloseHelpBtn", "_on_close_help_pressed");

	// --- 3. AUDIO SETTINGS SETUP ---
	
	// Music Toggle
	BaseButton* music = Object::cast_to<BaseButton>(self->find_child("MusicToggle", true, false));
	if (music) { 
		music->set_toggle_mode(true);
		int bus = AudioServer::get_singleton()->get_bus_index("Music");
		if (bus != -1) music->set_pressed(!AudioServer::get_singleton()->is_bus_mute(bus));
		
		if (!music->is_connected("pressed", Callable(self, "_on_music_toggled"))) 
			music->connect("pressed", Callable(self, "_on_music_toggled"));
			
		// Add Hover sound to toggle
		if (!music->is_connected("mouse_entered", Callable(self, "_on_button_hover")))
			music->connect("mouse_entered", Callable(self, "_on_button_hover"));
	}

	// SFX Toggle
	BaseButton* sfx = Object::cast_to<BaseButton>(self->find_child("SFXToggle", true, false));
	if (sfx) { 
		sfx->set_toggle_mode(true);
		int bus = AudioServer::get_singleton()->get_bus_index("SFX");
		if (bus != -1) sfx->set_pressed(!AudioServer::get_singleton()->is_bus_mute(bus));

		if (!sfx->is_connected("pressed", Callable(self, "_on_sfx_toggled"))) 
			sfx->connect("pressed", Callable(self, "_on_sfx_toggled")); 
			
		// Add Hover sound to toggle
		if (!sfx->is_connected("mouse_entered", Callable(self, "_on_button_hover")))
			sfx->connect("mouse_entered", Callable(self, "_on_button_hover"));
	}

	// Volume Slider
	HSlider* vol = Object::cast_to<HSlider>(self->find_child("VolumeSlider", true, false));
	if (vol) { 
		int bus = AudioServer::get_singleton()->get_bus_index("Master");
		if (bus != -1) {
			float db = AudioServer::get_singleton()->get_bus_volume_db(bus);
			vol->set_value(std::pow(10.0f, db / 20.0f)); 
		}
		if (!vol->is_connected("value_changed", Callable(self, "_on_volume_changed"))) 
			vol->connect("value_changed", Callable(self, "_on_volume_changed")); 
			
		// Sliders don't usually click, but we can add hover
		if (!vol->is_connected("mouse_entered", Callable(self, "_on_button_hover")))
			vol->connect("mouse_entered", Callable(self, "_on_button_hover"));
	}

	self->set_process_mode(Node::PROCESS_MODE_ALWAYS);
	
}

void _input(Caller* instance, Variant event)
{
	if (Input::get_singleton()->is_action_just_pressed("ui_cancel")) {
		_on_resume_pressed(instance);
	}
}

// --- SFX HELPERS ---
void _play_click(CanvasLayer* self) {
	AudioStreamPlayer* sfx = Object::cast_to<AudioStreamPlayer>(self->find_child("ClickSFX", true, false));
	if (sfx) sfx->play();
}

void _on_button_hover(Caller* instance) {
	CanvasLayer* self = GetSelf<CanvasLayer>(instance);
	AudioStreamPlayer* sfx = Object::cast_to<AudioStreamPlayer>(self->find_child("HoverSFX", true, false));
	if (sfx) sfx->play();
}

// --- NAVIGATION CALLBACKS ---

void _on_resume_pressed(Caller* instance)
{
	CanvasLayer* self = GetSelf<CanvasLayer>(instance);
	_play_click(self); // Play Sound
	
	SceneTree* tree = self->get_tree();
	bool current_pause_state = tree->get("paused");
	
	if (current_pause_state == false) {
		// PAUSE
		tree->set("paused", true);
		if (anim_player) anim_player->play("OpenSettings");
		else if (settings_panel) settings_panel->set_visible(true);
	} 
	else {
		// UNPAUSE
		tree->set("paused", false);
		if (anim_player) anim_player->play("CloseMenu");
		else {
			if (settings_panel) settings_panel->set_visible(false);
			if (help_panel) help_panel->set_visible(false);
			Control* ov = Object::cast_to<Control>(self->find_child("DimOverlay", true, false));
			if (ov) ov->set_visible(false);
		}
	}
}

void _on_leave_pressed(Caller* instance)
{
	CanvasLayer* self = GetSelf<CanvasLayer>(instance);
	_play_click(self); // Play Sound
	
	self->get_tree()->set("paused", false);

	self->get_tree()->change_scene_to_file("res://scenes/main_menu.tscn"); 
}

void _on_help_pressed(Caller* instance)
{
	CanvasLayer* self = GetSelf<CanvasLayer>(instance);
	_play_click(self); // Play Sound

	if (anim_player) anim_player->play("OpenHelp");
	else {
		if (settings_panel) settings_panel->set_visible(false);
		if (help_panel) help_panel->set_visible(true);
	}
	current_help_page = 1;
	_update_help_ui(Object::cast_to<Control>(help_panel));
}

void _on_close_help_pressed(Caller* instance)
{
	CanvasLayer* self = GetSelf<CanvasLayer>(instance);
	_play_click(self); // Play Sound

	if (anim_player) anim_player->play("OpenSettings");
	else {
		if (help_panel) help_panel->set_visible(false);
		if (settings_panel) settings_panel->set_visible(true);
	}
}

// --- HELP LOGIC ---
void _update_help_ui(Control* help_node) {
	if (!help_node) return;
	for (int i = 1; i <= max_help_pages; i++) {
		String page_name = "Page" + String::num_int64(i);
		Control* page = Object::cast_to<Control>(help_node->find_child(page_name, true, false));
		if (page) page->set_visible(current_help_page == i);
	}
	Control* prev = Object::cast_to<Control>(help_node->find_child("HelpPrev", true, false));
	Control* next = Object::cast_to<Control>(help_node->find_child("HelpNext", true, false));
	if (prev) prev->set_visible(current_help_page > 1);
	if (next) next->set_visible(current_help_page < max_help_pages);
}

void _on_help_next(Caller* instance) {
	CanvasLayer* self = GetSelf<CanvasLayer>(instance);
	if (current_help_page < max_help_pages) {
		current_help_page++;
		_play_click(self); // Play Sound
		_update_help_ui(Object::cast_to<Control>(self->find_child("HelpPanel", true, false)));
	}
}

void _on_help_prev(Caller* instance) {
	CanvasLayer* self = GetSelf<CanvasLayer>(instance);
	if (current_help_page > 1) {
		current_help_page--;
		_play_click(self); // Play Sound
		_update_help_ui(Object::cast_to<Control>(self->find_child("HelpPanel", true, false)));
	}
}

// --- REAL AUDIO LOGIC ---

void _on_music_toggled(Caller* instance) {
	CanvasLayer* self = GetSelf<CanvasLayer>(instance);
	_play_click(self); // Play Sound
	
	BaseButton* btn = Object::cast_to<BaseButton>(self->find_child("MusicToggle", true, false));
	if (btn) {
		bool is_active = btn->is_pressed(); 
		int bus = AudioServer::get_singleton()->get_bus_index("Music");
		AudioServer::get_singleton()->set_bus_mute(bus, !is_active);
	}
}

void _on_sfx_toggled(Caller* instance) {
	CanvasLayer* self = GetSelf<CanvasLayer>(instance);
	_play_click(self); // Play Sound
	
	BaseButton* btn = Object::cast_to<BaseButton>(self->find_child("SFXToggle", true, false));
	if (btn) {
		bool is_active = btn->is_pressed();
		int bus = AudioServer::get_singleton()->get_bus_index("SFX");
		AudioServer::get_singleton()->set_bus_mute(bus, !is_active);
	}
}

void _on_volume_changed(Caller* instance, double value) {
	int bus = AudioServer::get_singleton()->get_bus_index("Master");
	if (value <= 0.05) {
		AudioServer::get_singleton()->set_bus_mute(bus, true);
	} else {
		AudioServer::get_singleton()->set_bus_mute(bus, false);
		float db = 20.0f * std::log10((float)value);
		AudioServer::get_singleton()->set_bus_volume_db(bus, db);
	}
}

JENOVA_SCRIPT_END
