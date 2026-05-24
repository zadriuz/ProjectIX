shader_type canvas_item;

void fragment() {
    // 1. Setup Coordinates: Normalized coordinates from -1.0 to 1.0 (center is 0.0)
    vec2 centered_uv = (UV * 2.0) - 1.0; 

    // 2. Calculate Distance from Center
    float distance_from_center = length(centered_uv);

    // 3. Define Parameters
    float map_radius = 0.98;         // The main radius of the map content
    float border_thickness = 0.05;   // How thick the border should be

    // 4. Define the Outer Edge of the Border
    float outer_radius = map_radius; 
    float inner_radius = map_radius - border_thickness; 

    // 5. Check for Transparency (Outside the Circle)
    if (distance_from_center > outer_radius) {
        // Pixel is completely outside the defined map circle.
        discard;
    } 
    // 6. Check for Border Zone (Draw the new color: #94b0c2)
    else if (distance_from_center > inner_radius && distance_from_center <= outer_radius) {
        // Draw the border using the normalized hex color values (R, G, B, A)
        COLOR = vec4(0.580, 0.690, 0.760, 1.0); // New color: #94b0c2
    }
    // 7. Draw Map Content
    else {
        // Pixel is inside the main map area.
        COLOR = texture(TEXTURE, UV);
    }
}