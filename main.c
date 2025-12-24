#include <stdbool.h>
#include <raylib.h>
#include "engine.h"
#include "camera.h"
#include "colors.h"
#include "voxel_space_map.h"

#define SCREEN_WIDTH    (1920)
#define SCREEN_HEIGHT   (1080)

int main(void)
{
    // Initialize Registry and Editor State
    Registry reg = registry_init();
    EditorState editor = {0};
    editor.active_axis = GIZMO_NONE;
    editor.selected_entity.id = ENTITY_INVALID;

    // Add some initial entities
    Entity e1 = registry_create_entity(&reg);
    reg.entities[0].transform.position = (Vector3){0, 0, 0};
    reg.entities[0].mesh.color = RED;

    Entity e2 = registry_create_entity(&reg);
    reg.entities[1].transform.position = (Vector3){5, 0, 0};
    reg.entities[1].mesh.color = BLUE;

    init_camera();
    init_map();

    Color clear_color = CLEAR_COLOR;
    
    SetConfigFlags(FLAG_WINDOW_UNDECORATED);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "The Game Engine");
    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        // Update
        if (IsKeyPressed(KEY_Q)) break;
        
        update_camera();
        system_editor_update(&reg, &editor, get_camera());
    
        
        // Render
        BeginDrawing();
            ClearBackground(clear_color);
            draw_map();
            
            system_render(&reg, get_camera());
            system_editor_render(&reg, &editor, get_camera());
            
            DrawFPS(10, 10);
            DrawText("Press Q to Quit", 10, 30, 20, DARKGRAY);
            DrawText("Click objects to select, drag gizmo to move", 10, 50, 20, DARKGRAY);
            
        EndDrawing();
    }

    CloseWindow();
    registry_free(&reg);

    return 0;
}
