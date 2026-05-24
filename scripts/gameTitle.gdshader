shader_type canvas_item;

// Settings to tweak the intensity
uniform float shake_power : hint_range(0.0, 0.1) = 0.005;
uniform float shake_rate : hint_range(0.0, 1.0) = 0.2;
uniform float shake_speed = 5.0;
uniform float shake_block_size = 30.5;

// Define our custom neon colors
const vec4 COLOR_RED = vec4(1.0, 0.0, 0.2, 1.0); // Neon Red
const vec4 COLOR_BLUE = vec4(0.0, 0.8, 1.0, 1.0); // Neon Blue (Cyan)

float random(float seed) {
	return fract(543.2543 * sin(dot(vec2(seed, seed), vec2(3525.46, -54.3415))));
}

void fragment() {
	// Decide if we should glitch this frame
	float enable_shift = float(random(trunc(TIME * shake_speed)) < shake_rate);

	// Calculate the jittered position
	vec2 fixed_uv = UV;
	fixed_uv.x += (random((trunc(UV.y * shake_block_size) / shake_block_size) + TIME) - 0.5) * shake_power * enable_shift;

	// Get the original image color at the new position
	vec4 original_color = texture(TEXTURE, fixed_uv);
	
	// --- THE NEW COLOR LOGIC ---
	
	// Create the Red ghost
	vec4 red_ghost = texture(TEXTURE, fixed_uv + vec2(shake_power, 0.0));
	red_ghost *= COLOR_RED; // Tint it neon red
	red_ghost.a = original_color.a; // Keep original transparency

	// Create the Blue ghost
	vec4 blue_ghost = texture(TEXTURE, fixed_uv + vec2(-shake_power, 0.0));
	blue_ghost *= COLOR_BLUE; // Tint it neon blue
	blue_ghost.a = original_color.a; // Keep original transparency

	// Mix them: Original image + flashes of Red and Blue
	vec4 final_color = original_color;
	// Add red flash if glitching
	final_color = mix(final_color, final_color + red_ghost, enable_shift * 0.5);
	// Add blue flash if glitching
	final_color = mix(final_color, final_color + blue_ghost, enable_shift * 0.5);
	
	COLOR = final_color;
}