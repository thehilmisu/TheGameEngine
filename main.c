#include <stdio.h>
#include <stdbool.h>
#include <raylib.h>
#include "game.h"
#include "camera.h"
#include "colors.h"
#include "voxel_space_map.h"

#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"

// #define SCREEN_WIDTH    (1920)
// #define SCREEN_HEIGHT   (1080)
#define SCREEN_WIDTH    (1080)
#define SCREEN_HEIGHT   (720)


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
    player = create_entity_with_model(&game, ENTITY_PLAYER, "resources/models/aircraft.glb");
    if (player) {
        player->transform.position = (Vector3){512, 150, 512};
        player->mesh.color = WHITE;
    }

    //set_camera_target(game.reg.entities[0].transform.position);
    set_camera_target(player->transform.position);

    init_map();

    static int current_map = 0;

    while (!WindowShouldClose())
    {
        // DEBUGGING
        //

        if(IsKeyReleased(KEY_M))
        {
            current_map = ((1 + get_current_map())  % NUM_MAPS); 
            nob_log(NOB_INFO, "Map Changed : %d" , current_map);
            change_map(current_map);
        }

        ////////////////////////////////////
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
            char buf[256];
            sprintf(buf, "Selected map : %d ", get_current_map());
            DrawText(buf, 10, 30, 20, WHITE);
            sprintf(buf, "Entity count : %zu ", game.reg.count);
            DrawText(buf, 10, 50, 20, WHITE);
            
        EndDrawing();
    }

    CloseWindow();
    cleanup_map();
    game_free(&game);

    return 0;
}
