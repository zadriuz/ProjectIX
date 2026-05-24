// ============================================
// capsule.cpp - FIXED VERSION
// ============================================
#include <Godot/godot.hpp>
#include <Godot/classes/area2d.hpp>
#include <Godot/classes/resource_loader.hpp>
#include <Godot/classes/packed_scene.hpp>
#include <Godot/classes/node2d.hpp>
#include <Godot/classes/animation_player.hpp>
#include <Godot/classes/collision_shape2d.hpp>
#include <Godot/variant/variant.hpp>

using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN

void _ready(Caller* instance)
{
	
}

void _on_body_entered(Caller* instance, Variant& body_v)
{
	

	Area2D* capsule = GetSelf<Area2D>(instance);
	if (!capsule) return;

	Object* obj = body_v;
	Node* body = Object::cast_to<Node>(obj);

	// Tell the player it collected a capsule
	if (body && body->has_method("collect_capsule")) {
		body->call("collect_capsule");
		
	}

	// Disable collision shape using call_deferred to avoid physics errors
	CollisionShape2D* shape = capsule->get_node<CollisionShape2D>(NodePath("CollisionShape2D"));
	if (shape) {
		shape->call_deferred("set_disabled", true);
	}

	// Spawn bounce effect
	Node* parent = capsule->get_parent();
	if (parent) {
		Ref<PackedScene> bounce_scene = ResourceLoader::get_singleton()->load("res://scenes/BounceEffect.tscn");
		if (bounce_scene.is_valid()) {
			Node* raw = bounce_scene->instantiate();
			Node2D* effect = Object::cast_to<Node2D>(raw);
			if (effect) {
				parent->add_child(effect);
				effect->set_position(capsule->get_position());

				AnimationPlayer* anim_player = effect->get_node<AnimationPlayer>(NodePath("AnimationPlayer"));
				if (anim_player) {
					anim_player->play("bounce");
					
				}
			}
		}
	}

	capsule->call_deferred("queue_free");
	
}

JENOVA_SCRIPT_END
