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
    Entity* player = NULL;

    // Initialize Registry and Editor State
    Game game = game_init();
    EditorState editor = {0};
    editor.active_axis = GIZMO_NONE;
    editor.selected_entity.id = ENTITY_INVALID;

    init_camera();

    Color clear_color = CLEAR_COLOR;
    
    SetConfigFlags(FLAG_WINDOW_UNDECORATED);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "The Game");
    SetTargetFPS(60);

    // Add some initial entities (MUST BE AFTER InitWindow for models to load)
    create_entity_with_model(&game, "resources/models/aircraft.glb");
    game.reg.entities[0].transform.position = (Vector3){512, 150, 512};
    game.reg.entities[0].mesh.color = WHITE;
    player = &game.reg.entities[0];

    //set_camera_target(game.reg.entities[0].transform.position);
    set_camera_target(player->transform.position);

    init_map();

    while (!WindowShouldClose())
    {
        // Update
        game_update(&game, GetFrameTime());
        //set_camera_target(game.reg.entities[0].transform.position);
        set_camera_target(player->transform.position);
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
