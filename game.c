#include "game.h"
#include "voxel_space_map.h"
#include <raymath.h>
#include <rlgl.h>
#include <stdlib.h>

Game game_init(void) 
{
    Game game = {0};
    game.player_id = ENTITY_INVALID;
    return game;
}

Entity* create_entity(Game *game, EntityType type) 
{
    if (game->reg.count >= game->reg.capacity) 
    {
        size_t new_capacity = game->reg.capacity == 0 ? 256 : game->reg.capacity * 2;
        game->reg.entities = NOB_REALLOC(game->reg.entities, new_capacity * sizeof(*game->reg.entities));
        NOB_ASSERT(game->reg.entities);
        game->reg.capacity = new_capacity;
    }

    Entity *entity = &game->reg.entities[game->reg.count];
    memset(entity, 0, sizeof(Entity));

    entity->id = (uint32_t)(game->reg.count + 1);
    entity->active = true;
    entity->type = type;

    entity->transform = (TransformComponent){
        .position = {0, 0, 0},
        .rotation = {0, 0, 0},
        .scale = {1, 1, 1}
    };

    entity->mesh = (MeshComponent){
        .type = MESH_CUBE,
        .color = RED
    };
    
    if (type == ENTITY_PLAYER) {
        game->player_id = entity->id;
    }

    game->reg.count++;
    return entity;
}

Entity* create_entity_with_model(Game *game, EntityType type, const char* model_path) 
{
    Entity *entity = create_entity(game, type);
    
    Model m = LoadModel(model_path);
    if (m.meshCount == 0) {
        TraceLog(LOG_ERROR, "GAME: Failed to load model from %s", model_path);
    }
    entity->mesh = (MeshComponent){
        .type = MESH_MODEL,
        .color = WHITE,
        .model = m
    };
    
    return entity;
}

void remove_entity_at_index(Game *game, size_t index)
{
    if (index >= game->reg.count) return;

    Entity *entity = &game->reg.entities[index];
    
    // Cleanup resources
    // Don't unload models for scenery entities since they share the same model
    if (entity->mesh.type == MESH_MODEL && entity->type != ENTITY_SCENERY) {
        UnloadModel(entity->mesh.model);
    }

    // Swap with last element
    if (index < game->reg.count - 1) {
        game->reg.entities[index] = game->reg.entities[game->reg.count - 1];
    }

    game->reg.count--;

    // Resize array if it's mostly empty (e.g., less than 1/4 full)
    // We keep a minimum capacity of 256
    if (game->reg.count > 0 && game->reg.count < game->reg.capacity / 4 && game->reg.capacity > 256) {
        size_t new_capacity = game->reg.capacity / 2;
        if (new_capacity < 256) new_capacity = 256;
        
        Entity *new_entities = NOB_REALLOC(game->reg.entities, new_capacity * sizeof(Entity));
        if (new_entities) {
            game->reg.entities = new_entities;
            game->reg.capacity = new_capacity;
        }
    }
}

void remove_entity(Game *game, uint32_t id)
{
    for (size_t i = 0; i < game->reg.count; i++) {
        if (game->reg.entities[i].id == id) {
            remove_entity_at_index(game, i);
            break;
        }
    }
}

void spawn_projectile(Game *game, Vector3 position, Vector3 direction)
{
    Entity *proj = create_entity(game, ENTITY_PROJECTILE);
    proj->transform.position = position;
    proj->transform.scale = (Vector3){0.2f, 0.2f, 0.2f};
    proj->mesh.type = MESH_CUBE;
    proj->mesh.color = YELLOW;
    
    proj->projectile.direction = Vector3Normalize(direction);
    proj->projectile.speed = 100.0f;
    proj->projectile.lifetime = 3.0f;
}

void game_free(Game *game) 
{
    NOB_FREE(game->reg.entities);
    memset(game, 0, sizeof(Game));
}

void game_render(Game *game, Camera3D *camera)
{
    BeginMode3D(*camera);
    for (size_t i = 0; i < game->reg.count; ++i) 
    {
        Entity *entity = &game->reg.entities[i];
        if (!entity->active) continue;

        TransformComponent *t = &entity->transform;
        MeshComponent *m = &entity->mesh;
        EditorComponent *e = &entity->editor;
        
        Color color = m->color;
        if (e->is_selected) color = GREEN;
        else if (e->is_hovered) color = YELLOW;
        
        switch (m->type) 
        {
            case MESH_CUBE:
                DrawCube(t->position, t->scale.x, t->scale.y, t->scale.z, color);
                DrawCubeWires(t->position, t->scale.x, t->scale.y, t->scale.z, MAROON);
                break;
            case MESH_SPHERE:
                DrawSphere(t->position, t->scale.x, color);
                DrawSphereWires(t->position, t->scale.x, 16, 16, MAROON);
                break;
            case MESH_PLANE:
                DrawPlane(t->position, (Vector2){t->scale.x, t->scale.z}, color);
                break;
            case MESH_MODEL:
                DrawModel(m->model, t->position, t->scale.x, m->color);
                DrawModelWires(m->model, t->position, t->scale.x, PURPLE);
                break;

        }
    }

    EndMode3D();
}

void handle_input(Game *game, float timeDelta)
{
    static float pitch = 0.0f;
    static float roll = 0.0f;
    static float yaw = 0.0f;

    Entity *player = NULL;
    for (size_t i = 0; i < game->reg.count; i++) {
        if (game->reg.entities[i].id == game->player_id && game->reg.entities[i].active) {
            player = &game->reg.entities[i];
            break;
        }
    }

    if (!player) return;
    
    if (IsKeyDown(KEY_W)) {
        pitch += 0.6f;
        player->transform.position.y -=  timeDelta * 30.0f;
    }
    else if (IsKeyDown(KEY_S)) {
        pitch -= 0.6f;
        player->transform.position.y +=  timeDelta * 30.0f;
    }
    else{
        if (pitch > 0.3f) pitch -= 0.3f;
        else if (pitch < -0.3f) pitch += 0.3f;
    }

    if (IsKeyDown(KEY_A)) {
        roll -=1.0f;
        player->transform.position.x +=  timeDelta * 30.0f;
    }
    else if (IsKeyDown(KEY_D)) {
        roll += 1.0f;
        player->transform.position.x -=  timeDelta * 30.0f;
    }
    else
    {
        if (roll > 0.0f) roll -= 0.5f;
        else if (roll < 0.0f) roll += 0.5f;
    }

    if (IsKeyPressed(KEY_SPACE)) {
        // Calculate plane's actual forward direction based on pitch and roll
        Matrix mat = MatrixRotateXYZ((Vector3){ DEG2RAD*pitch, DEG2RAD*yaw, DEG2RAD*roll });
        // Forward vector in Matrix is Column 2 (Index 8, 9, 10)
        Vector3 forward = { mat.m8, mat.m9, mat.m10 }; 
        
        // Spawn slightly in front of the player to avoid clipping
        Vector3 spawnPos = player->transform.position;
        spawnPos = Vector3Add(spawnPos, Vector3Scale(forward, 2.0f));
        
        // Apply visual correction for the voxel map perspective
        // The camera is pitched down significantly (~26 deg), but the voxel map horizon 
        // implies a shallower pitch (~14 deg). We pitch the bullet down to match.
        forward.y -= 0.2f;
        
        spawn_projectile(game, spawnPos, forward);
    }

    player->mesh.model.transform = MatrixRotateXYZ((Vector3){ DEG2RAD*pitch, DEG2RAD*yaw, DEG2RAD*roll });
    
    // Constantly move player forward
    // player->transform.position.z += 10.0f * timeDelta;
}

void game_update(Game *game, float timeDelta)
{
    handle_input(game, timeDelta);

    for (size_t i = 0; i < game->reg.count; i++) {
        Entity *entity = &game->reg.entities[i];

        if (!entity->active) {
            remove_entity_at_index(game, i);
            i--; // Check the same index again as it now contains a new entity
            continue;
        }

        if (entity->type == ENTITY_PROJECTILE) {
            entity->transform.position = Vector3Add(entity->transform.position,                     
                Vector3Scale(entity->projectile.direction, entity->projectile.speed * timeDelta));
            entity->projectile.lifetime -= timeDelta;
            if (entity->projectile.lifetime <= 0) entity->active = false;
        }
    }
}


static void draw_gizmo(Vector3 pos, GizmoAxis active) 
{
    float len = 2.0f;
    float thickness = 0.1f;
    
    // X axis (Red)
    Color x_color = (active == GIZMO_X) ? YELLOW : RED;
    DrawLine3D(pos, (Vector3){pos.x + len, pos.y, pos.z}, x_color);
    DrawCube((Vector3){pos.x + len, pos.y, pos.z}, thickness*2, thickness*2, thickness*2, x_color);
    
    // Y axis (Green)
    Color y_color = (active == GIZMO_Y) ? YELLOW : GREEN;
    DrawLine3D(pos, (Vector3){pos.x, pos.y + len, pos.z}, y_color);
    DrawCube((Vector3){pos.x, pos.y + len, pos.z}, thickness*2, thickness*2, thickness*2, y_color);
    
    // Z axis (Blue)
    Color z_color = (active == GIZMO_Z) ? YELLOW : BLUE;
    DrawLine3D(pos, (Vector3){pos.x, pos.y, pos.z + len}, z_color);
    DrawCube((Vector3){pos.x, pos.y, pos.z + len}, thickness*2, thickness*2, thickness*2, z_color);
}

static GizmoAxis check_gizmo_collision(Ray ray, Vector3 pos) 
{
    float len = 2.0f;
    float thickness = 0.2f;
    
    // Check collision with handles (cubes at the ends)
    BoundingBox box_x = {(Vector3){pos.x + len - thickness, pos.y - thickness, pos.z - thickness}, (Vector3){pos.x + len + thickness, pos.y + thickness, pos.z + thickness}};
    BoundingBox box_y = {(Vector3){pos.x - thickness, pos.y + len - thickness, pos.z - thickness}, (Vector3){pos.x + thickness, pos.y + len + thickness, pos.z + thickness}};
    BoundingBox box_z = {(Vector3){pos.x - thickness, pos.y - thickness, pos.z + len - thickness}, (Vector3){pos.x + thickness, pos.y + thickness, pos.z + len + thickness}};
    
    if (GetRayCollisionBox(ray, box_x).hit) return GIZMO_X;
    if (GetRayCollisionBox(ray, box_y).hit) return GIZMO_Y;
    if (GetRayCollisionBox(ray, box_z).hit) return GIZMO_Z;

    // Check collision with the lines (approximate with slightly thicker boxes)
    BoundingBox line_x = {(Vector3){pos.x, pos.y - thickness/2, pos.z - thickness/2}, (Vector3){pos.x + len, pos.y + thickness/2, pos.z + thickness/2}};
    BoundingBox line_y = {(Vector3){pos.x - thickness/2, pos.y, pos.z - thickness/2}, (Vector3){pos.x + thickness/2, pos.y + len, pos.z + thickness/2}};
    BoundingBox line_z = {(Vector3){pos.x - thickness/2, pos.y - thickness/2, pos.z}, (Vector3){pos.x + thickness/2, pos.y + thickness/2, pos.z + len}};

    if (GetRayCollisionBox(ray, line_x).hit) return GIZMO_X;
    if (GetRayCollisionBox(ray, line_y).hit) return GIZMO_Y;
    if (GetRayCollisionBox(ray, line_z).hit) return GIZMO_Z;
    
    return GIZMO_NONE;
}

// Find the point on axis line closest to the mouse ray
static float get_axis_drag_t(Ray ray, Vector3 pos, Vector3 axis) {
    Vector3 p_minus_o = Vector3Subtract(pos, ray.position);
    Vector3 a_cross_d = Vector3CrossProduct(axis, ray.direction);
    float denom = Vector3LengthSqr(a_cross_d);
    if (denom < 0.0001f) return 0.0f;
    
    Vector3 d_cross_pmo = Vector3CrossProduct(ray.direction, p_minus_o);
    return Vector3DotProduct(d_cross_pmo, a_cross_d) / denom;
}

void editor_update(Game *game, EditorState *editor, Camera3D *camera) {
    Ray ray = GetScreenToWorldRay(GetMousePosition(), *camera);
    Registry *reg = &game->reg;
    
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) 
    {
        bool hit_gizmo = false;
        if (editor->selected_entity.id != ENTITY_INVALID) 
        {
            size_t idx = 0;
            bool found = false;
            for (size_t i = 0; i < reg->count; ++i) 
            {
                if (reg->entities[i].id == editor->selected_entity.id) 
                {
                    idx = i;
                    found = true;
                    break;
                }
            }
            
            if (found) 
            {
                GizmoAxis axis = check_gizmo_collision(ray, reg->entities[idx].transform.position);
                if (axis != GIZMO_NONE) {
                    editor->active_axis = axis;
                    Vector3 axis_vec = {0};
                    if (axis == GIZMO_X) axis_vec = (Vector3){1, 0, 0};
                    if (axis == GIZMO_Y) axis_vec = (Vector3){0, 1, 0};
                    if (axis == GIZMO_Z) axis_vec = (Vector3){0, 0, 1};
                    
                    editor->drag_entity_start_pos = reg->entities[idx].transform.position;
                    editor->drag_start_t = get_axis_drag_t(ray, editor->drag_entity_start_pos, axis_vec);
                    hit_gizmo = true;
                }
            }
        }
        
        if (!hit_gizmo) 
        {
            editor->selected_entity.id = ENTITY_INVALID;
            editor->active_axis = GIZMO_NONE;
            for (size_t i = 0; i < reg->count; ++i) {
                TransformComponent *t = &reg->entities[i].transform;
                EditorComponent *e = &reg->entities[i].editor;
                
                BoundingBox box = {
                    (Vector3){ t->position.x - t->scale.x/2, t->position.y - t->scale.y/2, t->position.z - t->scale.z/2 },
                    (Vector3){ t->position.x + t->scale.x/2, t->position.y + t->scale.y/2, t->position.z + t->scale.z/2 }
                };
                
                RayCollision collision = GetRayCollisionBox(ray, box);
                if (collision.hit) {
                    e->is_selected = true;
                    editor->selected_entity = reg->entities[i];
                    break; // Select the first one hit
                } else {
                    e->is_selected = false;
                }
            }
        }
    }
    
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) 
    {
        editor->active_axis = GIZMO_NONE;
    }
    
    if (editor->active_axis != GIZMO_NONE && editor->selected_entity.id != ENTITY_INVALID) 
    {
        size_t idx = 0;
        bool found = false;
        for (size_t i = 0; i < reg->count; ++i) {
            if (reg->entities[i].id == editor->selected_entity.id) {
                idx = i;
                found = true;
                break;
            }
        }
        
        if (found) 
        {
            Vector3 axis_vec = {0};
            if (editor->active_axis == GIZMO_X) axis_vec = (Vector3){1, 0, 0};
            if (editor->active_axis == GIZMO_Y) axis_vec = (Vector3){0, 1, 0};
            if (editor->active_axis == GIZMO_Z) axis_vec = (Vector3){0, 0, 1};
            
            float current_t = get_axis_drag_t(ray, editor->drag_entity_start_pos, axis_vec);
            float diff = current_t - editor->drag_start_t;
            
            reg->entities[idx].transform.position = Vector3Add(editor->drag_entity_start_pos, Vector3Scale(axis_vec, diff));
        }
    }
}

void system_editor_render(Registry *reg, EditorState *editor, Camera3D *camera) {
    if (editor->selected_entity.id != ENTITY_INVALID) 
    {
        size_t idx = 0;
        bool found = false;
        for (size_t i = 0; i < reg->count; ++i) 
        {
            if (reg->entities[i].id == editor->selected_entity.id) 
            {
                idx = i;
                found = true;
                break;
            }
        }
        if (found) 
        {
            BeginMode3D(*camera);
            draw_gizmo(reg->entities[idx].transform.position, editor->active_axis);
            EndMode3D();
        }
    }
}

void clear_scenery(Game *game)
{
    // Remove all scenery entities
    for (size_t i = 0; i < game->reg.count; i++) {
        Entity *entity = &game->reg.entities[i];
        if (entity->type == ENTITY_SCENERY) {
            remove_entity_at_index(game, i);
            i--; // Check the same index again
        }
    }
}

void spawn_trees_on_terrain(Game *game, Model tree_model, int count, unsigned int seed)
{
    TraceLog(LOG_INFO, "GAME: Spawning %d trees on terrain (seed: %u)", count, seed);
    
    srand(seed);
    
    // Map bounds (assuming 1024x1024 heightmap)
    const float map_size = 1024.0f;
    const float min_height = 1.0f;  // Don't place trees in water
    const float max_height = 10.0f; // Don't place trees on mountains
    const float min_spacing = 30.0f; // Minimum distance between trees
    
    int placed = 0;
    int attempts = 0;
    const int max_attempts = count * 10; // Give up after too many failed attempts
    
    // Keep track of placed tree positions for spacing check
    Vector3 *tree_positions = (Vector3*)malloc(count * sizeof(Vector3));
    
    while (placed < count && attempts < max_attempts) {
        attempts++;
        
        // Random position on map
        float x = ((float)rand() / RAND_MAX) * map_size;
        float z = ((float)rand() / RAND_MAX) * map_size;
        
        // Get terrain height at this position
        float height = get_terrain_height(x, z);
        
        // Check if height is suitable
        if (height < min_height || height > max_height) {
            continue;
        }
        
        // Check spacing with other trees
        bool too_close = false;
        for (int i = 0; i < placed; i++) {
            float dx = x - tree_positions[i].x;
            float dz = z - tree_positions[i].z;
            float dist_sq = dx*dx + dz*dz;
            if (dist_sq < min_spacing * min_spacing) {
                too_close = true;
                break;
            }
        }
        
        if (too_close) {
            continue;
        }
        
        // Place the tree!
        Entity *tree = create_entity(game, ENTITY_SCENERY);
        tree->mesh.type = MESH_MODEL;
        tree->mesh.model = tree_model; // Copy the model reference
        tree->mesh.color = WHITE;
        
        // Position at terrain height
        tree->transform.position = (Vector3){ x, height, z };
        // tree->transform.position = (Vector3){ 512, 150, 550 };
        
        // Random rotation around Y axis
        tree->transform.rotation.y = ((float)rand() / RAND_MAX) * 360.0f;
        
        // Random scale variation (0.8 to 1.2)
        float scale = 0.8f + ((float)rand() / RAND_MAX) * 0.4f;
        tree->transform.scale = (Vector3){ scale, scale, scale };
        
        // Store position
        tree_positions[placed] = tree->transform.position;
        placed++;
    }
    
    // free(tree_positions);    
    TraceLog(LOG_INFO, "GAME: Successfully placed %d trees (attempts: %d)", placed, attempts);
}
