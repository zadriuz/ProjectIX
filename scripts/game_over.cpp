// ============================================
// game_over.cpp - Game Over Screen Logic
// ============================================
#include <Godot/godot.hpp>
#include <Godot/classes/control.hpp>
#include <Godot/classes/scene_tree.hpp>
#include <Godot/classes/texture_button.hpp>
#include <Godot/classes/audio_stream_player.hpp>

using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN

// Forward Declarations
void _on_retry_btn_pressed(Caller* instance);
void _on_leave_btn_pressed(Caller* instance);
void _play_click(Control* self);
void _play_hover(Caller* instance);

void _ready(Caller* instance)
{
	Control* self = GetSelf<Control>(instance);
	if (!self) return;

	// We must ensure the game is unpaused so buttons can be clicked
	SceneTree* tree = self->get_tree();
	if (tree) tree->set("paused", false);
	
	// --- CONNECT BUTTONS ---
	auto setup_btn = [&](String name, String pressed_callback, String hover_callback) {
		TextureButton* btn = Object::cast_to<TextureButton>(self->find_child(name, true, false));
		if (!btn) return;
		
		btn->set_default_cursor_shape(Control::CURSOR_POINTING_HAND);
		
		// Connect Pressed Signal
		if (!btn->is_connected("pressed", Callable(self, pressed_callback)))
			btn->connect("pressed", Callable(self, pressed_callback));
			
		// Connect Hover Signal
		if (!btn->is_connected("mouse_entered", Callable(self, hover_callback)))
			btn->connect("mouse_entered", Callable(self, hover_callback));
	};

	// Assuming your button names are exactly retryBtn and leaveBtn
	setup_btn("retryBtn", "_on_retry_btn_pressed", "_play_hover");
	setup_btn("leaveBtn", "_on_leave_btn_pressed", "_play_hover");


}

void _play_click(Control* self) {
	AudioStreamPlayer* sfx = Object::cast_to<AudioStreamPlayer>(self->find_child("ClickSFX", true, false));
	if (sfx) sfx->play();
}

void _play_hover(Caller* instance) {
	Control* self = GetSelf<Control>(instance);
	AudioStreamPlayer* sfx = Object::cast_to<AudioStreamPlayer>(self->find_child("HoverSFX", true, false));
	if (sfx) sfx->play();
}

void _on_retry_btn_pressed(Caller* instance)
{
	Control* self = GetSelf<Control>(instance);
	_play_click(self);

	// Load the first level to restart the game
	SceneTree* tree = self->get_tree();
	if (tree) {
		tree->set("paused", false);
		tree->change_scene_to_file("res://scenes/EasyLevel.tscn");
	}
}

void _on_leave_btn_pressed(Caller* instance)
{
	Control* self = GetSelf<Control>(instance);
	_play_click(self);

	// FIX: Change destination to Main Menu instead of quitting
	SceneTree* tree = self->get_tree();
	if (tree) {
		tree->set("paused", false);
		tree->change_scene_to_file("res://scenes/main_menu.tscn");
	}
}

JENOVA_SCRIPT_END
