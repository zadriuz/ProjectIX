shader_type canvas_item;

uniform float radius : hint_range(0.0, 1.0) = 0.8;
uniform float softness : hint_range(0.0, 1.0) = 0.45;

void fragment() {
    // Get distance from the center of the screen
    float dist = distance(UV, vec2(0.5));
    
    // Calculate the alpha (transparency) based on that distance
    float alpha = smoothstep(radius, radius - softness, dist);
    
    // Draw black with the calculated transparency
    COLOR = vec4(0.0, 0.0, 0.0, 1.0 - alpha);
}