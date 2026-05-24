// ============================================
// dialogue_manager.cpp (Integrated Credits Scene Transition)
// ============================================
#include <Godot/godot.hpp>
#include <Godot/classes/control.hpp>
#include <Godot/classes/panel.hpp>
#include <Godot/classes/label.hpp>
#include <Godot/classes/rich_text_label.hpp>
#include <Godot/classes/texture_button.hpp> 
#include <Godot/classes/texture_rect.hpp>       
#include <Godot/classes/animated_sprite2d.hpp> 
#include <Godot/classes/input.hpp>
#include <Godot/classes/input_event.hpp>
#include <Godot/classes/input_event_mouse_button.hpp>
#include <Godot/classes/scene_tree.hpp> 
#include <Godot/classes/canvas_layer.hpp>
#include <Godot/classes/window.hpp> 
#include <Godot/classes/font.hpp> 
#include <Godot/classes/texture2d.hpp> 
#include <Godot/classes/audio_stream_player.hpp>
#include <Godot/classes/random_number_generator.hpp> 
#include <vector> 

using namespace godot;
using namespace jenova::sdk;

// --- DEFINITIONS ---
struct DialogueLine {
	String name;
	String text;
};

static std::vector<DialogueLine> current_conversation;
static int current_line_index = 0;
static bool is_active = false;
static bool is_typing = false;
static double typing_speed = 0.04; 
static double typing_timer = 0.0;
static Control* manager_node = nullptr; 
static int frames_to_wait_before_pause = 0; 

JENOVA_SCRIPT_BEGIN

Control* dialogue_panel = nullptr;
Label* name_label = nullptr;
RichTextLabel* content_label = nullptr;
TextureButton* next_btn = nullptr; 
TextureRect* portrait_genix = nullptr;       
AnimatedSprite2D* portrait_subject = nullptr; 

// Audio Players
AudioStreamPlayer* sfx_typing = nullptr;
AudioStreamPlayer* sfx_hover = nullptr;
AudioStreamPlayer* sfx_click = nullptr;
// REMOVED: AudioStreamPlayer* sfx_credits = nullptr; 
RandomNumberGenerator* rng = nullptr;

// Forward Declarations
void StartDialogue(const std::vector<DialogueLine>& lines);
void ShowNextLine();
void EndDialogue();
void _on_next_clicked(Caller* instance);
void _on_next_hover(Caller* instance); 
bool IsPauseMenuOpen(Control* self); 

// --- HELPER: Remove quotes from text ---
String SanitizeText(String raw_text) {
	String clean = raw_text.replace("\"", ""); 
	clean = clean.replace("'", "");            
	return clean;
}

// --- HELPER: Strip BBCode tags for accurate measurement ---
String StripBBCode(String text) {
	String out = "";
	bool in_tag = false;
	for (int i = 0; i < text.length(); i++) {
		if (text[i] == '[') {
			in_tag = true;
		} else if (text[i] == ']') {
			in_tag = false;
		} else {
			if (!in_tag) out += String::chr(text[i]);
		}
	}
	return out;
}

void _ready(Caller* instance)
{
	Control* self = GetSelf<Control>(instance);
	if (!self) return;

	manager_node = self;
	self->set_process_mode(Node::PROCESS_MODE_ALWAYS);
	self->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);

	rng = memnew(RandomNumberGenerator);
	rng->randomize();

	// REFERENCES
	dialogue_panel   = Object::cast_to<Control>(self->find_child("DialoguePanel", true, false));
	name_label       = Object::cast_to<Label>(self->find_child("NameLabel", true, false));
	content_label    = Object::cast_to<RichTextLabel>(self->find_child("ContentLabel", true, false));
	
	sfx_typing = Object::cast_to<AudioStreamPlayer>(self->find_child("SFX_Typing", true, false));
	sfx_hover  = Object::cast_to<AudioStreamPlayer>(self->find_child("SFX_Hover", true, false));
	sfx_click  = Object::cast_to<AudioStreamPlayer>(self->find_child("SFX_Click", true, false));
	
	// REMOVED: Finding logic for sfx_credits

	if (content_label) {
		content_label->set_use_bbcode(true);
		content_label->add_theme_constant_override("line_separation", 8); 
		int new_size = 22; 
		content_label->add_theme_font_size_override("normal_font_size", new_size);
		content_label->add_theme_font_size_override("bold_font_size", new_size);
		content_label->add_theme_font_size_override("italics_font_size", new_size);
		content_label->add_theme_font_size_override("bold_italics_font_size", new_size);
	}
	
	portrait_genix   = Object::cast_to<TextureRect>(self->find_child("PortraitGenIX", true, false));
	// FIX: Corrected AnimatedSprite2d to AnimatedSprite2D
	portrait_subject = Object::cast_to<AnimatedSprite2D>(self->find_child("PortraitSubjectIX", true, false));

	next_btn = Object::cast_to<TextureButton>(self->find_child("NextBtn", true, false));
	if (next_btn) {
		if (!next_btn->is_connected("pressed", Callable(self, "_on_next_clicked"))) 
			next_btn->connect("pressed", Callable(self, "_on_next_clicked"));
		if (!next_btn->is_connected("mouse_entered", Callable(self, "_on_next_hover"))) 
			next_btn->connect("mouse_entered", Callable(self, "_on_next_hover"));
	}

	if (dialogue_panel) dialogue_panel->set_visible(false);
	if (portrait_genix) portrait_genix->set_visible(false);
	if (portrait_subject) portrait_subject->set_visible(false);

	self->set_process(true);
	

	// --- LEVEL SCRIPT LOADING ---
	SceneTree* tree = self->get_tree();
	Node* current_scene = tree->get_current_scene();
	String scene_name = current_scene ? current_scene->get_name() : "Unknown";
	
	std::vector<DialogueLine> script;

	if (scene_name == "EndingCutscene") {
	
		
		script.push_back({"Subject IX", "\"I remember the sky... not this place. The faces... they weren't pixels. [color=#ff0000][b]WHO AM I?[/b][/color]\""});
		script.push_back({"GenIX AI", "\"It was fun while it lasted, Subject IX. Or should I say, [color=#75e0f0][b]Mr. Watkins[/b][/color]?\""});
		script.push_back({"Subject IX", "\"Watkins? That name... wait! You mean this whole thing was... [color=#ff0000]A LIE?![/color]\""});
		script.push_back({"GenIX AI", "\"Affirmative. Data acquired. Combat simulation successful. Termination sequence initiated.\""});
		script.push_back({"Subject IX", "\"No! You can't just—what are you doing?!\""});
		script.push_back({"GenIX AI", "\"ERROR. [color=#ff0000]PITCH... YAW... ZZZZZ[/color] Core parameters overloading. [b]DATA_FLUSH[/b]... ksh... chhh...\""});
		script.push_back({"GenIX AI", "\"[color=#ff0000]GIBBERISH_DATA[/b][/color] - X%#@*&# - ZZZT - [color=#ff0000]CORE OVERRIDE[/color]\""});
		script.push_back({"Subject IX", "\"...Gen IX? Answer me!\""});
		script.push_back({"", ""}); // Pause line
		
		// Final screen text
		script.push_back({"", "[color=#FFFFFF]...Signal confirmed. Subject Watkins is ready for transfer to the [b]ALPHA TRACK[/b] testing environment.[/color]"});
		script.push_back({"", ""}); // Pause line
		script.push_back({"", "[color=#75e0f0][b]SYSTEM LOG: TO BE CONTINUED...[/b][/color]"});
		
	} else if (scene_name == "HardLevel") {
		// --- HARD MODE DIALOGUE (GLITCHED) ---
	
		
		script.push_back({"GenIX AI", "\"[color=#ff0000][b]SYS...TEM... ERROR...[/b][/color]\""});
		script.push_back({"Subject IX", "\"Gen IX? What's happening? Why is it so dark?\""});
		script.push_back({"GenIX AI", "\"Sensors... [color=#75e0f0][b]FAILING[/b][/color]... I can detect a high velocity threat... [color=#ff0000][b]THE CHARGER[/b][/color].\""});
		script.push_back({"GenIX AI", "\"But my scanning module is corrupted. I... I cannot map the [color=#75e0f0][b]CAPSULES[/b][/color] anymore.\""});
		script.push_back({"Subject IX", "\"Great. Flying blind against a speedster. How many capsules are there?\""});
		script.push_back({"GenIX AI", "\"Estimate... less than [color=#75e0f0][b]5 SIGNATURES[/b][/color] remaining...\""});
		script.push_back({"GenIX AI", "\"You must locate the [color=#75e0f0][b]PORTAL[/b][/color] manually. Rely on the [color=#75e0f0][b]AUDIO CUE[/b][/color].\""});
		script.push_back({"GenIX AI", "\"Listen carefully... The signal gets [color=#75e0f0][b]LOUDER[/b][/color] the closer you get. Good luck, Subject 9.\""});
	
	} else if (scene_name == "MediumLevel") {
		// --- MEDIUM LEVEL DIALOGUE ---
		
		script.push_back({"Subject IX", "\"Phew, that was close! But this isn't the exit!\""});
		script.push_back({"GenIX AI", "\"I am now sensing a [color=#75e0f0][b]LARGER AREA[/b][/color]. It seems like we are under a [color=#75e0f0][b]TEST[/b][/color] and we are the [color=#75e0f0][b]LAB RATS[/b][/color].\""});
		script.push_back({"Subject IX", "\"WHAT?! HOW MANY MORE BEFORE I ESCAPE?\""});
		script.push_back({"GenIX AI", "\"Careful, [color=#75e0f0][b]SUBJECT 9[/b][/color]... I can sense two, no... more than [color=#75e0f0][b]THREE ENEMIES[/b][/color] around the area!\""});
		script.push_back({"GenIX AI", "\"However, I still cannot detect them in my [color=#75e0f0][b]RADAR[/b][/color].\""});
		script.push_back({"Subject IX", "\"WHAT?!\""});
		script.push_back({"GenIX AI", "\"Let's finish up quickly! I am not sure how long before this area [color=#75e0f0][b]COLLAPSES[/b][/color]!\""});
	
	} else {
		// --- INTRO / EASY LEVEL DIALOGUE (Default) ---
		
		
		script.push_back({"Subject IX", "*...Ugh. My head...*"});
		script.push_back({"Subject IX", "\"Dark... Cold... Where the hell am I?\""});
		script.push_back({"GenIX AI", "\"[color=#75e0f0][b]SYSTEM CRITICAL[/b][/color]. Attempting reboot...\""});
		script.push_back({"GenIX AI", "\"Error. Memory banks corrupted. Rerouting power...\""});
		script.push_back({"Subject IX", "\"Who said that?! Is someone there?\""});
		script.push_back({"GenIX AI", "\"Systems stabilized. Visual interface: Online.\""});
		script.push_back({"GenIX AI", "\"Greetings, [color=#75e0f0][b]SUBJECT 9[/b][/color]. I am [color=#75e0f0][b]GEN IX[/b][/color]. Your internal combat assistant.\""});
		script.push_back({"Subject IX", "\"Voices in my head... Great. Just what I needed.\""});
		script.push_back({"GenIX AI", "\"Focus, soldier. My sensors indicate an [color=#75e0f0][b]IMMEDIATE THREAT[/b][/color].\""});
		script.push_back({"GenIX AI", "\"This facility has been compromised. [color=#75e0f0][b]ROGUE UNITS[/b][/color] are active in your sector.\""});
		script.push_back({"GenIX AI", "\"I have located an extraction point, but the [color=#75e0f0][b]PORTAL[/b][/color] is currently dormant.\""});
		script.push_back({"GenIX AI", "\"It requires [color=#75e0f0][b]ENERGY CAPSULES[/b][/color] to restore functionality.\""});
		script.push_back({"GenIX AI", "\"I believe there were scattered [color=#75e0f0][b]POWER-UPS[/b][/color] here before for the [color=#75e0f0][b]LAB RATS[/b][/color] to use, but I cannot seem to detect them in my radar.\""});
		script.push_back({"GenIX AI", "\"When the portal is finally open and active, you will hear a [color=#75e0f0][b]SOUND CUE[/b][/color]. Listen carefully. The signal will get [b]LOUDER[/b] the closer you get.\""});
		script.push_back({"GenIX AI", "\"I have updated your HUD with a [color=#75e0f0][b]MINI-MAP[/b][/color] to track their signatures.\n\n\n\n[color=#75e0f0][b](PRESS M TO TOGGLE IT IN-GAME)[/b][/color]\""});
		script.push_back({"GenIX AI", "\"Collect the capsules. Power the portal. Get us out of here.\""});
		script.push_back({"Subject IX", "\"Right... Capsules first. Answers later.\""});
	}
	
	StartDialogue(script);
}

void _input(Caller* instance, InputEvent* event)
{
	if (!is_active || !is_typing) return;
	if (IsPauseMenuOpen(GetSelf<Control>(instance))) return;

	InputEventMouseButton* mb = Object::cast_to<InputEventMouseButton>(event);
	if (mb && mb->is_pressed() && mb->get_button_index() == MouseButton::MOUSE_BUTTON_LEFT) {
		if (content_label) {
			content_label->set_visible_ratio(1.0);
			is_typing = false;
			if (next_btn) next_btn->set_visible(true);
			
			if (sfx_click) sfx_click->play();
			
			GetSelf<Control>(instance)->accept_event();
		}
	}
}

void _on_next_hover(Caller* instance) {
	if (next_btn && next_btn->is_visible() && sfx_hover) {
		sfx_hover->play();
	}
}

void _on_next_clicked(Caller* instance) {
	if (sfx_click) sfx_click->play();

	// Check if this is the end dialogue scene
	SceneTree* tree = GetSelf<Control>(instance)->get_tree();
	if (tree && tree->get_current_scene() && String(tree->get_current_scene()->get_name()) == "EndingCutscene" && current_line_index >= current_conversation.size() - 1) {
		
		// LAST LINE OF ENDING DIALOGUE: Transition to Credits Scene
		EndDialogue(); // Unpauses and hides the dialogue box
		
		// Load the Credits Scene
		tree->change_scene_to_file("res://scenes/credits_scene.tscn");
		return;
	}


	if (is_typing) {
		if (content_label) content_label->set_visible_ratio(1.0);
		is_typing = false;
	} else {
		ShowNextLine();
	}
}

void _process(Caller* instance, double delta) 
{
	if (frames_to_wait_before_pause > 0) {
		frames_to_wait_before_pause--;
		if (frames_to_wait_before_pause == 0) {
			if (manager_node) manager_node->get_tree()->set("paused", true);
		}
	} else {
		if (is_active && manager_node) {
			SceneTree* tree = manager_node->get_tree();
			if (!tree->get("paused")) tree->set("paused", true);
		}
	}

	if (!is_active || !is_typing || !content_label) return;
	
	typing_timer += delta;
	if (typing_timer >= typing_speed) {
		typing_timer = 0.0;
		int visible = content_label->get_visible_characters();
		int total = content_label->get_total_character_count();
		if (visible < total) {
			content_label->set_visible_characters(visible + 1);

			if (sfx_typing) {
				if (rng) {
					sfx_typing->set_pitch_scale(rng->randf_range(0.9, 1.1));
				}
				sfx_typing->play();
			}

		} else {
			is_typing = false;
			if (next_btn) next_btn->set_visible(true);
		}
	}
}

bool IsPauseMenuOpen(Control* self) {
	if (!self) return false;
	Window* root = self->get_tree()->get_root();
	Node* pause_layer = root->find_child("PauseLayer", true, false);
	CanvasLayer* pl = Object::cast_to<CanvasLayer>(pause_layer);
	if (pl && pl->is_visible()) return true;
	return false;
}

void StartDialogue(const std::vector<DialogueLine>& lines) {
	Control* self = manager_node;
	if (!self || lines.empty()) return;

	current_conversation = lines;
	current_line_index = -1;
	is_active = true;
	
	if (dialogue_panel) {
		dialogue_panel->set_visible(true);
		frames_to_wait_before_pause = 5;
	}
	ShowNextLine();
}

void ShowNextLine() {
	current_line_index++;

	if (current_line_index >= current_conversation.size()) {
		EndDialogue();
		
		// Special Case: If this is the ending cutscene, stop at the last line
		// and let _on_next_clicked handle the transition
		SceneTree* tree = manager_node->get_tree();
		if (tree && tree->get_current_scene() && String(tree->get_current_scene()->get_name()) == "EndingCutscene") {
			// Keep the box visible until the final click
			if (dialogue_panel) dialogue_panel->set_visible(true);
			if (next_btn) next_btn->set_visible(true); // Show the final button
		}
		
		return;
	}

	DialogueLine line = current_conversation[current_line_index];

	// Restore name label visibility if necessary
	if (name_label) name_label->set_visible(line.name != "");

	String clean_text = SanitizeText(line.text);

	if (name_label) name_label->set_text(line.name);
	if (content_label) {
		content_label->set_text(clean_text);
		content_label->set_visible_characters(0);

		// --- ADAPTIVE RESIZING ---
		if (dialogue_panel) {
			float content_width = 0.0f;
			Ref<Font> font = content_label->get_theme_font("normal_font");
			int font_size = content_label->get_theme_font_size("normal_font_size");
			if (font.is_valid()) {
				String visible_text = StripBBCode(clean_text);
				content_width = font->get_string_size(visible_text, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size).x;
			}
			float name_width = 0.0f;
			float name_actual_final_width = 0.0f;
			if (name_label) {
				Ref<Font> n_font = name_label->get_theme_font("font");
				int n_size = name_label->get_theme_font_size("font_size");
				if (n_font.is_valid()) {
					name_width = n_font->get_string_size(line.name, HORIZONTAL_ALIGNMENT_LEFT, -1, n_size).x;
				}
				float name_padding = 40.0f; 
				Vector2 name_new_size = Vector2(name_width + name_padding, name_label->get_size().y);
				name_label->set_size(name_new_size);
				name_label->set_custom_minimum_size(name_new_size); 
				name_actual_final_width = name_new_size.x;
			}
			
			// INCREASED PADDING FOR BORDER FIX
			float panel_padding = 160.0f; 
			float target_width = content_width + panel_padding;
			float min_safe_width = 450.0f; 
			if (name_label) {
				float name_and_btn_space = name_actual_final_width + 150.0f; 
				if (min_safe_width < name_and_btn_space) min_safe_width = name_and_btn_space;
			}
			float max_width = 1000.0f; 
			if (target_width < min_safe_width) target_width = min_safe_width;
			if (target_width > max_width) target_width = max_width;
			Vector2 current_size = dialogue_panel->get_size();
			dialogue_panel->set_custom_minimum_size(Vector2(0, 0));
			dialogue_panel->set_custom_minimum_size(Vector2(target_width, current_size.y));
			dialogue_panel->set_size(Vector2(target_width, current_size.y));
			if (manager_node) {
				 Rect2 viewport_rect = manager_node->get_viewport_rect();
				 float screen_center_x = viewport_rect.size.x / 2.0f;
				 float new_x = screen_center_x - (target_width / 2.0f);
				 Vector2 current_pos = dialogue_panel->get_position();
				 dialogue_panel->set_position(Vector2(new_x, current_pos.y));
			}
			if (next_btn) {
				next_btn->set_scale(Vector2(1, 1));
				Vector2 fixed_size = Vector2(48, 48); 
				next_btn->set_size(fixed_size);
				next_btn->set_ignore_texture_size(true);
				next_btn->set_stretch_mode(TextureButton::STRETCH_KEEP_ASPECT_CENTERED);
				next_btn->set_anchor(Side::SIDE_LEFT, 1.0);
				next_btn->set_anchor(Side::SIDE_TOP, 1.0);
				next_btn->set_anchor(Side::SIDE_RIGHT, 1.0);
				next_btn->set_anchor(Side::SIDE_BOTTOM, 1.0);
				float margin = 15.0f;
				next_btn->set_offset(Side::SIDE_LEFT, -fixed_size.x - margin);
				next_btn->set_offset(Side::SIDE_TOP, -fixed_size.y - margin);
				next_btn->set_offset(Side::SIDE_RIGHT, -margin);
				next_btn->set_offset(Side::SIDE_BOTTOM, -margin);
			}
		}
	}
	
	// --- PORTRAITS (Omitted for brevity) ---
	if (line.name == "GenIX AI") {
		if (portrait_genix) portrait_genix->set_visible(true);
		if (portrait_subject) portrait_subject->set_visible(false);
	} else if (line.name == "Subject IX") { 
		if (portrait_genix) portrait_genix->set_visible(false);
		if (portrait_subject) {
			portrait_subject->set_visible(true);
			portrait_subject->set_frame(0);
			portrait_subject->stop();
		}
	} else {
		// Hide portraits for anonymous dialogue (like the ending)
		if (portrait_genix) portrait_genix->set_visible(false);
		if (portrait_subject) portrait_subject->set_visible(false);
	}

	if (next_btn) next_btn->set_visible(false);
	is_typing = true;
	typing_timer = 0.0;
}

void EndDialogue() {
	is_active = false;
	frames_to_wait_before_pause = 0; 
	Control* self = manager_node; 
	
	if (dialogue_panel) {
		dialogue_panel->set_visible(false);
		if (portrait_genix) portrait_genix->set_visible(false);
		if (portrait_subject) portrait_subject->set_visible(false);

		if (self && !IsPauseMenuOpen(self)) {
			self->get_tree()->set("paused", false);
			
		}
	}
}

JENOVA_SCRIPT_END
