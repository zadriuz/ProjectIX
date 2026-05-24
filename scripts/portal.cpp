// ============================================
// portal.cpp - Final Production (Direct Transition)
// ============================================
#include <Godot/godot.hpp>
#include <Godot/classes/area2d.hpp>
#include <Godot/classes/node2d.hpp>
#include <Godot/classes/scene_tree.hpp>
#include <Godot/variant/callable.hpp>
#include <Godot/variant/string_name.hpp>
#include <Godot/variant/string.hpp>
#include <Godot/variant/color.hpp>

using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN

JENOVA_PROPERTY(String, next_level_path, "res://scenes/MediumLevel.tscn");

void _on_body_entered(Caller* instance, Variant& body_v);

void _ready(Caller* instance)
{
	Area2D* self = GetSelf<Area2D>(instance);
	if (!self) return;

	// Alpha Pre-warm (Fixes Lag)
	self->set_visible(true); 
	self->set_modulate(Color(1, 1, 1, 0)); 
	self->set_monitoring(true);
	self->set_monitorable(true);
	
	if (!self->is_connected("body_entered", Callable(self, "_on_body_entered"))) {
		self->connect("body_entered", Callable(self, "_on_body_entered"));
	}
	
	self->set_meta("is_activated", false);
	self->set_meta("transition_active", false);
	
}

void activate(Caller* instance)
{
	Area2D* self = GetSelf<Area2D>(instance);
	if (!self) return;
	
	if (self->has_meta("is_activated") && (bool)self->get_meta("is_activated")) return;
	self->set_meta("is_activated", true);

	// VISUALS ON
	self->set_modulate(Color(1, 1, 1, 1)); 

}

void _on_body_entered(Caller* instance, Variant& body_v)
{
	Area2D* self = GetSelf<Area2D>(instance);
	if (!self) return;
	
	if (!self->has_meta("is_activated") || !(bool)self->get_meta("is_activated")) return;
	if (self->has_meta("transition_active") && (bool)self->get_meta("transition_active")) return;
	
	Object* obj = body_v;
	Node* body = Object::cast_to<Node>(obj);
	if (!body) return;
	
	String body_name = body->get_name();
	bool is_player = body->is_in_group("Player") || body_name.contains("SubjectIX");
	
	if (is_player) 
	{
		self->set_meta("transition_active", true);
	

		SceneTree* tree = self->get_tree();
		if (!tree || !tree->get_current_scene()) return;

		String current_name = tree->get_current_scene()->get_name();
		String target = "";

		// --- AUTOMATIC LEVEL CHAINING ---
		if (current_name == "EasyLevel") {
			target = "res://scenes/MediumLevel.tscn";
		
		}
		else if (current_name == "MediumLevel") {
			target = "res://scenes/HardLevel.tscn";
			
		}
		else if (current_name == "HardLevel") {
			target = "res://scenes/ending_cutscene.tscn";
			
		}
		else {
			target = next_level_path;
		
		}

		if (target != "") {
			// STANDARD TRANSITION: Defer the scene change call directly
			// This allows the current physics frame to finish before the tree is modified.
			tree->call_deferred("change_scene_to_file", target);
		} else {
			
			self->set_meta("transition_active", false);
		}
	}
}

JENOVA_SCRIPT_END
