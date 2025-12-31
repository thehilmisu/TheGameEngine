#include <stdbool.h>
#include <raylib.h>
#include "game.h"
#include "camera.h"
#include "colors.h"
#include "voxel_space_map.h"

#define SCREEN_WIDTH    (1920)
#define SCREEN_HEIGHT   (1080)

int main(void)
{
    // Initialize Registry and Editor State
    Game game = game_init();
    EditorState editor = {0};
    editor.active_axis = GIZMO_NONE;
    editor.selected_entity.id = ENTITY_INVALID;

    // Add some initial entities
    create_entity(&game);
    game.reg.entities[0].transform.position = (Vector3){512, 150, 512};
    game.reg.entities[0].mesh.color = RED;

    init_camera();
    set_camera_target(game.reg.entities[0].transform.position);

    Color clear_color = CLEAR_COLOR;
    
    SetConfigFlags(FLAG_WINDOW_UNDECORATED);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "The Game Engine");
    SetTargetFPS(60);

    init_map();

    while (!WindowShouldClose())
    {
        // Update
        game_update(&game, GetFrameTime());
        set_camera_target(game.reg.entities[0].transform.position);
        update_camera();
        
        // Render
        BeginDrawing();
            ClearBackground(clear_color);
            
            render_map();
            
            game_render(&game, get_camera());
            //system_editor_render(&reg, &editor, get_camera());
            DrawFPS(10, 10);
            
        EndDrawing();
    }

    CloseWindow();
    cleanup_map();
    game_free(&game);

    return 0;
}
