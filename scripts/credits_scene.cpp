// ============================================
// credits_scene.cpp - Timed Credits Sequence (Final Version)
// ============================================
#include <Godot/godot.hpp>
#include <Godot/classes/control.hpp>
#include <Godot/classes/label.hpp>
#include <Godot/classes/texture_rect.hpp>
#include <Godot/classes/audio_stream_player.hpp>
#include <Godot/classes/scene_tree.hpp>
#include <Godot/classes/timer.hpp>
#include <Godot/classes/node.hpp> 
#include <Godot/classes/canvas_item.hpp> // Required for visibility
#include <Godot/variant/string.hpp> 

using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN

// UI References
Control* self_node = nullptr;
AudioStreamPlayer* credits_music = nullptr;
Timer* transition_timer = nullptr;
int current_credit_step = 0;

// List of credit nodes to display sequentially
// FIX: Targeting Parent Nodes to ensure the whole group (Title + Name) is shown
const char* CREDIT_NODES[] = {
	"LeadDev",      // 0. Lead Developer Group
	"Dev",          // 1. Developer Group
	"Designers",    // 2. Designers Group
	"Contributor",  // 3. Contributor Group
	"GameLogo",     // 4. Game Logo
	nullptr         // Sentinel value
};

// --- SETTINGS ---
constexpr float DISPLAY_TIME = 5.0f; // Display time for each name/group (5 seconds)
constexpr float LOGO_TIME = 10.0f;   // Final hold time for logo (10 seconds)

// Forward Declarations
void _on_transition_timer_timeout(Caller* instance);
void start_credits_sequence(Control* self);
void show_next_credit_step(Control* self);

void _ready(Caller* instance)
{
	self_node = GetSelf<Control>(instance);
	if (!self_node) return;

	// Ensure game is unpaused for music/timer processing
	SceneTree* tree = self_node->get_tree();
	if (tree) tree->set("paused", false);

	// Get References
	credits_music = Object::cast_to<AudioStreamPlayer>(self_node->find_child("CreditsMusic", true, false));
	transition_timer = Object::cast_to<Timer>(self_node->find_child("TransitionTimer", true, false));
	
	// Connect Timer
	if (transition_timer) {
		if (!transition_timer->is_connected("timeout", Callable(self_node, "_on_transition_timer_timeout"))) {
			transition_timer->connect("timeout", Callable(self_node, "_on_transition_timer_timeout"));
		}
	}

	// Configure visibility: Hide parents, Show children
	int i = 0;
	while (CREDIT_NODES[i] != nullptr) {
		Control* node = Object::cast_to<Control>(self_node->get_node_or_null(NodePath(CREDIT_NODES[i])));
		if (node) {
			// 1. Force all children (the names) to be VISIBLE
			// This fixes the issue where children set to "Visible: Off" in editor remain hidden
			// even when the parent is shown.
			TypedArray<Node> children = node->get_children();
			for (int j = 0; j < children.size(); j++) {
				CanvasItem* child = Object::cast_to<CanvasItem>(children[j]);
				if (child) child->set_visible(true);
			}

			// 2. Hide the parent node itself (it will be revealed by the timer)
			node->set_visible(false);
		}
		i++;
	}

	start_credits_sequence(self_node);

}

void start_credits_sequence(Control* self) {
	if (credits_music) {
		credits_music->play();
		
	}
	current_credit_step = 0;
	show_next_credit_step(self);
}

void show_next_credit_step(Control* self) {
	// The array now only has 5 elements (0-4). End when current_credit_step exceeds 4.
	if (current_credit_step > 4) {
		// We have passed all the content; transition now.
		if (credits_music) credits_music->stop();
		self->get_tree()->call_deferred("change_scene_to_file", "res://scenes/main_menu.tscn");
		return;
	}

	// --- Step 1: Hide Previous Element ---
	if (current_credit_step > 0) {
		// Hides the element that was just displayed in the previous step
		Control* prev_node = Object::cast_to<Control>(self->get_node_or_null(NodePath(CREDIT_NODES[current_credit_step - 1])));
		if (prev_node) prev_node->set_visible(false);
	}
	
	// --- Step 2: Show Current Element ---
	String current_path = CREDIT_NODES[current_credit_step];
	Control* current_node = Object::cast_to<Control>(self->get_node_or_null(NodePath(current_path)));
	
	if (current_node) {
		current_node->set_visible(true);
	
	} 
	
	// --- Step 3: Determine Next Timer Duration ---
	float duration = DISPLAY_TIME;
	
	if (current_credit_step == 4) {
		// Index 4 is the Game Logo
		duration = LOGO_TIME;
	}

	// --- Step 4: Advance Step and Start Timer ---
	if (transition_timer) {
		transition_timer->set_wait_time(duration);
		transition_timer->start();
	}
	
	current_credit_step++;
}

void _on_transition_timer_timeout(Caller* instance) {
	Control* self = GetSelf<Control>(instance);
	
	// If current_credit_step is now 5, it means the Logo (index 4) timer finished.
	if (current_credit_step > 4) {
		// If the logo finished its 10-second run, transition back to main menu
		if (credits_music) credits_music->stop();
		self->get_tree()->call_deferred("change_scene_to_file", "res://scenes/main_menu.tscn");
	} else {
		// Continue the normal sequence
		show_next_credit_step(self);
	}
}

JENOVA_SCRIPT_END
