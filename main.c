#include <stdio.h>
#include <stdbool.h>
#include <raylib.h>
#include <raymath.h>
#include "game.h"
#include "camera.h"
#include "colors.h"
#include "plants.h"
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
    
    set_camera_target(player->transform.position);

    init_map();
    TraceLog(LOG_INFO, "Map initialized successfully");

    static int current_map = 0;
    TraceLog(LOG_INFO, "About to enter game loop");
    
    // Load tree model once (separate from entities, will be shared)
    TraceLog(LOG_INFO, "Loading tree model...");
    Model treeModel = LoadModel("resources/models/leaf_tree_-_ps1_low_poly.glb");
    if (treeModel.meshCount > 0) {
        TraceLog(LOG_INFO, "Tree model loaded successfully");
        // Spawn trees across the terrain
        spawn_trees_on_terrain(&game, treeModel, 50, current_map);
    } else {
        TraceLog(LOG_ERROR, "Failed to load tree model");
    }
    

    while (!WindowShouldClose())
    {
        // DEBUGGING
        //

        if(IsKeyReleased(KEY_M))
        {
            current_map = ((1 + get_current_map())  % NUM_MAPS); 
            nob_log(NOB_INFO, "Map Changed : %d" , current_map);
            
            // Clear existing trees
            clear_scenery(&game);
            
            // Change map
            change_map(current_map);
            
            // Respawn trees for new map
            spawn_trees_on_terrain(&game, treeModel, 50, current_map);
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
            
            DrawFPS(10, 10);
            char buf[256];
            sprintf(buf, "Selected map : %d ", get_current_map());
            DrawText(buf, 10, 30, 20, WHITE);
            sprintf(buf, "Entity count : %zu ", game.reg.count);
            DrawText(buf, 10, 70, 20, WHITE);
            
        EndDrawing();
    }

    // Cleanup
    UnloadModel(treeModel);
    CloseWindow();
    cleanup_map();
    game_free(&game);

    return 0;
}
