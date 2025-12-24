#include <stdbool.h>
#include <raylib.h>
#include "engine.h"

#define SCREEN_WIDTH    (800)
#define SCREEN_HEIGHT   (450)

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


    Camera3D camera = {0};
    camera.position = (Vector3){ 10.0f, 10.0f, 10.0f };
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    Color clear_color = { .r = 0xCD, .g = 0xCD, .b = 0xCD, .a = 0xFF };
    
    SetConfigFlags(FLAG_WINDOW_UNDECORATED);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "ECS Engine + Gizmos");
    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        // Update
        if (IsKeyPressed(KEY_Q)) break;
        
        system_editor_update(&reg, &editor, camera);

        // Render
        BeginDrawing();
            ClearBackground(clear_color);
            
            system_render(&reg, camera);
            system_editor_render(&reg, &editor, camera);
            
            DrawFPS(10, 10);
            DrawText("Press Q to Quit", 10, 30, 20, DARKGRAY);
            DrawText("Click objects to select, drag gizmo to move", 10, 50, 20, DARKGRAY);
            
        EndDrawing();
    }

    CloseWindow();
    registry_free(&reg);

    return 0;
}
