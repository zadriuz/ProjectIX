// ============================================
// immunity_shield.cpp - CALLS SPECIFIC FUNCTION
// ============================================
#include <Godot/godot.hpp>
#include <Godot/classes/area2d.hpp>
#include <Godot/classes/node.hpp>
#include <Godot/variant/callable.hpp>

using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN

void _on_body_entered(Caller* instance, Variant& body_v);

void _ready(Caller* instance)
{
	Area2D* self = GetSelf<Area2D>(instance);
	if (!self) return;

	if (!self->is_connected("body_entered", Callable(self, "_on_body_entered"))) {
		self->connect("body_entered", Callable(self, "_on_body_entered"));
	}
}

void _on_body_entered(Caller* instance, Variant& body_v)
{
	Area2D* self = GetSelf<Area2D>(instance);
	Object* obj = body_v;
	Node* body = Object::cast_to<Node>(obj);
	
	// CHECK FOR SPECIFIC METHOD
	if (body && body->has_method("activate_immunity")) {
		
		
		body->call("activate_immunity");
		
		self->queue_free();
	}
}

JENOVA_SCRIPT_END
