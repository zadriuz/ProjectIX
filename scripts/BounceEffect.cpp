#include <Godot/godot.hpp>
#include <Godot/classes/node2d.hpp>
#include <Godot/classes/animation_player.hpp>
#include <Godot/variant/variant.hpp>

using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN

void _ready(Caller* instance)
{
	Node2D* effect = GetSelf<Node2D>(instance);
	if (effect) {
		AnimationPlayer* anim_player = effect->get_node<AnimationPlayer>(NodePath("AnimationPlayer"));
		if (anim_player) {
			anim_player->play("bounce");
		}
	}
}

void _on_animation_player_animation_finished(Caller* instance, Variant& anim_name_v)
{
	Node2D* effect = GetSelf<Node2D>(instance);
	if (effect) {
		effect->call_deferred("queue_free");
		
	}
}

JENOVA_SCRIPT_END
