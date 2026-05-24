// ============================================
// main_menu.cpp - Fixed Overlay Z-Order Stacking
// ============================================
#include <Godot/godot.hpp>
#include <Godot/classes/control.hpp>
#include <Godot/classes/base_button.hpp>
#include <Godot/classes/texture_button.hpp>
#include <Godot/classes/panel.hpp>
#include <Godot/classes/scene_tree.hpp>
#include <Godot/classes/audio_stream_player.hpp>
#include <Godot/classes/animation_player.hpp>
#include <Godot/classes/video_stream_player.hpp>
#include <Godot/classes/input.hpp>
#include <Godot/classes/input_event.hpp>
#include <Godot/classes/input_event_key.hpp>
#include <Godot/classes/input_event_mouse_button.hpp>
#include <Godot/classes/timer.hpp> 

using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN

// UI References
Control* help_panel = nullptr;
Control* credits_panel = nullptr;
Control* overlay = nullptr;
Control* skip_btn = nullptr;
Control* game_title = nullptr;      // Reference for Title
Control* title_anchor = nullptr;    // Reference for Title Parent
AnimationPlayer* anim_player = nullptr;
VideoStreamPlayer* bg_video = nullptr;
VideoStreamPlayer* cutscene_player = nullptr;
AudioStreamPlayer* menu_music = nullptr;

// State
int current_help_page = 1;
int max_help_pages = 5; 
bool is_watching_cutscene = false;

// Forward Declarations
void _on_play_pressed(Caller* instance);
void _on_help_pressed(Caller* instance);
void _on_credits_pressed(Caller* instance);
void _on_exit_pressed(Caller* instance);
void _on_exit_confirm(Caller* instance); 
void _on_close_panels(Caller* instance);
void _on_video_finished(Caller* instance);       
void _on_cutscene_finished(Caller* instance);   
void _start_game_level(Control* self);           
void _on_help_next(Caller* instance);
void _on_help_prev(Caller* instance);
void _update_help_ui(Control* self);
void _on_skip_pressed(Caller* instance);

// Helpers
void _play_hover(Caller* instance);
void _play_click(Control* self);
void _set_buttons_disabled(Control* self, bool disabled); 

void _ready(Caller* instance)
{
	Control* self = GetSelf<Control>(instance);
	if (!self) return;

	// --- 1. GET REFERENCES ---
	help_panel      = Object::cast_to<Control>(self->find_child("HelpPanel", true, false));
	credits_panel   = Object::cast_to<Control>(self->find_child("CreditsPanel", true, false));
	overlay         = Object::cast_to<Control>(self->find_child("DimOverlay", true, false));
	anim_player     = Object::cast_to<AnimationPlayer>(self->find_child("AnimationPlayer", true, false));
	bg_video        = Object::cast_to<VideoStreamPlayer>(self->find_child("BackgroundVideo", true, false)); 
	cutscene_player = Object::cast_to<VideoStreamPlayer>(self->find_child("CutscenePlayer", true, false));
	menu_music      = Object::cast_to<AudioStreamPlayer>(self->find_child("MusicPlayer", true, false));
	skip_btn        = Object::cast_to<Control>(self->find_child("SkipBtn", true, false));
	
	// Get Title References
	game_title      = Object::cast_to<Control>(self->find_child("gameTitle", true, false));
	title_anchor    = Object::cast_to<Control>(self->find_child("TitleAnchor", true, false));

	// --- 2. FORCE START STATE ---
	if (help_panel) help_panel->set_visible(false);
	if (credits_panel) credits_panel->set_visible(false);
	
	// Setup Overlay Logic
	if (overlay) {
		overlay->set_visible(false);
		// Important: Ensure overlay blocks mouse when visible
		overlay->set_mouse_filter(Control::MOUSE_FILTER_STOP);
	}

	// FORCE TITLE VISIBLE (Fix for invisible title)
	if (title_anchor) title_anchor->set_visible(true);
	if (game_title) game_title->set_visible(true);
	
	// Init Video
	if (cutscene_player) {
		cutscene_player->set_visible(false);
		cutscene_player->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	}
	
	if (skip_btn) skip_btn->set_visible(false);

	// BG Video Loop
	if (bg_video) {
		bg_video->play();
		bg_video->set_mouse_filter(Control::MOUSE_FILTER_IGNORE); 
		if (!bg_video->is_connected("finished", Callable(self, "_on_video_finished")))
			bg_video->connect("finished", Callable(self, "_on_video_finished"));
	}
	
	// Cutscene Signals
	if (cutscene_player) {
		if (!cutscene_player->is_connected("finished", Callable(self, "_on_cutscene_finished")))
			cutscene_player->connect("finished", Callable(self, "_on_cutscene_finished"));
	}

	// --- 3. CONNECT BUTTONS ---
	auto setup_btn = [&](String name, String callback) {
		BaseButton* btn = Object::cast_to<BaseButton>(self->find_child(name, true, false));
		if (!btn) return;
		
		btn->set_focus_mode(Control::FOCUS_NONE);
		btn->set_default_cursor_shape(Control::CURSOR_POINTING_HAND);
		
		if (!btn->is_connected("pressed", Callable(self, callback)))
			btn->connect("pressed", Callable(self, callback));
			
		// HOVER SOUND
		if (!btn->is_connected("mouse_entered", Callable(self, "_play_hover")))
			btn->connect("mouse_entered", Callable(self, "_play_hover"));
	};

	setup_btn("playBtn", "_on_play_pressed");
	setup_btn("helpBtn", "_on_help_pressed");
	setup_btn("creditsBtn", "_on_credits_pressed");
	setup_btn("exitBtn", "_on_exit_pressed");
	setup_btn("HelpNext", "_on_help_next");
	setup_btn("HelpPrev", "_on_help_prev");
	setup_btn("CloseHelpBtn", "_on_close_panels");
	setup_btn("CloseCreditsBtn", "_on_close_panels");
	
	// Skip Button
	if (skip_btn) {
		BaseButton* sbtn = Object::cast_to<BaseButton>(skip_btn);
		if (sbtn) {
			sbtn->set_default_cursor_shape(Control::CURSOR_POINTING_HAND);
			if (!sbtn->is_connected("pressed", Callable(self, "_on_skip_pressed"))) {
				sbtn->connect("pressed", Callable(self, "_on_skip_pressed"));
			}
			if (!sbtn->is_connected("mouse_entered", Callable(self, "_play_hover"))) {
				sbtn->connect("mouse_entered", Callable(self, "_play_hover"));
			}
		}
	}
	
	// Exit Timer
	Timer* exit_timer = memnew(Timer);
	exit_timer->set_name("ExitDelayTimer");
	exit_timer->set_wait_time(0.2); 
	exit_timer->set_one_shot(true);
	exit_timer->connect("timeout", Callable(self, "_on_exit_confirm"));
	self->add_child(exit_timer);

	
}

// --- HELPERS ---
void _play_hover(Caller* instance) {
	Control* self = GetSelf<Control>(instance);
	AudioStreamPlayer* sfx = Object::cast_to<AudioStreamPlayer>(self->find_child("HoverSFX", true, false));
	if (sfx) sfx->play();
}

void _play_click(Control* self) {
	AudioStreamPlayer* sfx = Object::cast_to<AudioStreamPlayer>(self->find_child("ClickSFX", true, false));
	if (sfx) sfx->play();
}

void _set_buttons_disabled(Control* self, bool disabled) {
	const char* btn_names[] = {"playBtn", "helpBtn", "creditsBtn", "exitBtn"};
	for (const char* name : btn_names) {
		Control* btn = Object::cast_to<Control>(self->find_child(name, true, false));
		if (btn) {
			btn->set_mouse_filter(disabled ? Control::MOUSE_FILTER_IGNORE : Control::MOUSE_FILTER_STOP);
		}
	}
}

// --- CUTSCENE LOGIC ---
void _on_play_pressed(Caller* instance) {
	Control* self = GetSelf<Control>(instance);
	_play_click(self);
	_set_buttons_disabled(self, true);
	
	if (bg_video) { bg_video->stop(); bg_video->set_visible(false); }
	if (menu_music) { menu_music->stop(); }

	if (game_title) game_title->set_visible(false);
	
	if (cutscene_player) {
		is_watching_cutscene = true;
		cutscene_player->set_visible(true);
		cutscene_player->play();
		
		if (skip_btn) {
			skip_btn->set_visible(true);
			skip_btn->move_to_front(); // Ensure skip is on top of video
		}
	} else {
		_start_game_level(self);
	}
}

void _on_skip_pressed(Caller* instance) {
	Control* self = GetSelf<Control>(instance);
	_play_click(self);
	_start_game_level(self);
}

void _on_cutscene_finished(Caller* instance) {
	_start_game_level(GetSelf<Control>(instance));
}

void _start_game_level(Control* self) {
	if (!self) return;
	if (cutscene_player) cutscene_player->stop();
	is_watching_cutscene = false;
	if (skip_btn) skip_btn->set_visible(false);
	self->get_tree()->change_scene_to_file("res://scenes/EasyLevel.tscn");
}

// --- PANEL LOGIC ---
void _on_help_pressed(Caller* instance) {
	Control* self = GetSelf<Control>(instance);
	_play_click(self);
	
	_set_buttons_disabled(self, true);
	
	if (credits_panel) credits_panel->set_visible(false);
	current_help_page = 1;
	_update_help_ui(self);
	
	// FIX: DRAW ORDER - Ensure Panels (which are inside an anchor) draw ON TOP of the Overlay
	if (overlay) {
		overlay->set_visible(true);
		overlay->move_to_front(); // 1. Overlay covers buttons
	}
	if (help_panel) {
		help_panel->set_visible(true);
		
		// CRITICAL: Pull the panel's parent container to the front so it covers the overlay
		Node* parent = help_panel->get_parent();
		if (parent && parent != self) {
			Control* parent_ctrl = Object::cast_to<Control>(parent);
			if (parent_ctrl) parent_ctrl->move_to_front();
		}
		
		help_panel->move_to_front(); // 2. Panel/Container covers Overlay
	}
	
	if (anim_player) anim_player->play("OpenHelp");
}

void _on_credits_pressed(Caller* instance) {
	Control* self = GetSelf<Control>(instance);
	_play_click(self);

	_set_buttons_disabled(self, true);
	
	if (help_panel) help_panel->set_visible(false);

	// FIX: DRAW ORDER - Ensure Panels draw ON TOP of the Overlay
	if (overlay) {
		overlay->set_visible(true);
		overlay->move_to_front(); // 1. Overlay covers buttons
	}
	if (credits_panel) {
		credits_panel->set_visible(true);
		
		// CRITICAL: Pull the panel's parent container to the front
		Node* parent = credits_panel->get_parent();
		if (parent && parent != self) {
			Control* parent_ctrl = Object::cast_to<Control>(parent);
			if (parent_ctrl) parent_ctrl->move_to_front();
		}
		
		credits_panel->move_to_front(); // 2. Panel/Container covers Overlay
	}

	if (anim_player) anim_player->play("OpenCredits");
}

void _on_close_panels(Caller* instance) {
	Control* self = GetSelf<Control>(instance);
	_play_click(self);

	_set_buttons_disabled(self, false);

	if (anim_player) anim_player->play("ClosePanel");
	else {
		if (help_panel) help_panel->set_visible(false);
		if (credits_panel) credits_panel->set_visible(false);
		if (overlay) overlay->set_visible(false);
	}
}

// --- HELP PAGINATION ---
void _update_help_ui(Control* self) {
	if (!help_panel) return;
	for (int i = 1; i <= max_help_pages; i++) {
		String page_name = "Page" + String::num_int64(i);
		Control* page = Object::cast_to<Control>(help_panel->find_child(page_name, true, false));
		if (page) page->set_visible(current_help_page == i);
	}
	Control* prev = Object::cast_to<Control>(help_panel->find_child("HelpPrev", true, false));
	Control* next = Object::cast_to<Control>(help_panel->find_child("HelpNext", true, false));
	if (prev) prev->set_visible(current_help_page > 1);
	if (next) next->set_visible(current_help_page < max_help_pages);
}

void _on_help_next(Caller* instance) {
	if (current_help_page < max_help_pages) {
		current_help_page++;
		_play_click(GetSelf<Control>(instance));
		_update_help_ui(GetSelf<Control>(instance));
	}
}

void _on_help_prev(Caller* instance) {
	if (current_help_page > 1) {
		current_help_page--;
		_play_click(GetSelf<Control>(instance));
		_update_help_ui(GetSelf<Control>(instance));
	}
}

// --- EXIT LOGIC (DELAYED) ---
void _on_exit_pressed(Caller* instance) {
	Control* self = GetSelf<Control>(instance);
	_play_click(self); 
	
	Timer* t = Object::cast_to<Timer>(self->find_child("ExitDelayTimer", true, false));
	if (t) t->start();
	else GetSelf<Control>(instance)->get_tree()->quit(); 
}

void _on_exit_confirm(Caller* instance) {
	GetSelf<Control>(instance)->get_tree()->quit();
}

void _on_video_finished(Caller* instance) {
	if (bg_video) bg_video->play(); 
}

JENOVA_SCRIPT_END
