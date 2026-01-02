#include "game.h"
#include <raymath.h>

Game game_init(void) 
{
    Game game = {0};
    return game;
}

Entity create_entity(Game *game) 
{
    Entity entity = {0};

    entity.id = (uint32_t)(game->reg.count + 1);
    
    if (game->reg.count >= game->reg.capacity) 
    {
        size_t new_capacity = game->reg.capacity == 0 ? 256 : game->reg.capacity * 2;
        game->reg.entities = NOB_REALLOC(game->reg.entities, new_capacity * sizeof(*game->reg.entities));
        NOB_ASSERT(game->reg.entities);
        game->reg.capacity = new_capacity;
    }

    TransformComponent transform = {
        .position = {0, 0, 0},
        .rotation = {0, 0, 0},
        .scale = {1, 1, 1}
    };
    
    MeshComponent mesh = {
        .type = MESH_CUBE,
        .color = RED
    };
    
    EditorComponent editor = {
        .is_selected = false,
        .is_hovered = false
    };
    
    entity.transform = transform;
    entity.mesh = mesh;
    entity.editor = editor;
    game->reg.entities[game->reg.count] = entity;
    game->reg.count++;
    
    return entity;
}

Entity create_entity_with_model(Game *game, const char* model_path) 
{
    Entity entity = {0};

    entity.id = (uint32_t)(game->reg.count + 1);
    
    if (game->reg.count >= game->reg.capacity) 
    {
        size_t new_capacity = game->reg.capacity == 0 ? 256 : game->reg.capacity * 2;
        game->reg.entities = NOB_REALLOC(game->reg.entities, new_capacity * sizeof(*game->reg.entities));
        NOB_ASSERT(game->reg.entities);
        game->reg.capacity = new_capacity;
    }

    TransformComponent transform = {
        .position = {0, 0, 0},
        .rotation = {0, 0, 0},
        .scale = {1, 1, 1}
    };
    
    Model m = LoadModel(model_path);
    if (m.meshCount == 0) {
        TraceLog(LOG_ERROR, "GAME: Failed to load model from %s", model_path);
    }
    MeshComponent mesh = {
        .type = MESH_MODEL,
        .color = WHITE,
        .model = m
    };
    
    EditorComponent editor = {
        .is_selected = false,
        .is_hovered = false
    };
    
    entity.transform = transform;
    entity.mesh = mesh;
    entity.editor = editor;
    game->reg.entities[game->reg.count] = entity;
    game->reg.count++;
    
    return entity;
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
        TransformComponent *t = &game->reg.entities[i].transform;
        MeshComponent *m = &game->reg.entities[i].mesh;
        EditorComponent *e = &game->reg.entities[i].editor;
        
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
                DrawModel(m->model, t->position, t->scale.x, WHITE);
                break;

        }
    }

    //DrawGrid(10, 1.0f);
    EndMode3D();
}

float pitch = 0.0f;
float roll = 0.0f;
float yaw = 0.0f;

void game_update(Game *game, float timeDelta)
{
    Entity *player = &game->reg.entities[0];
    
    if (IsKeyDown(KEY_W)) {
        pitch += 0.6f;
    }
    else if (IsKeyDown(KEY_S)) {
        pitch -= 0.6f;
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

    if (IsKeyDown(KEY_Z)) {
       // position.y += 10.0 * timeDelta;
    }
    if (IsKeyDown(KEY_X)) {
       // position.y -= 10.0 * timeDelta;
    }

    player->mesh.model.transform = MatrixRotateXYZ((Vector3){ DEG2RAD*pitch, DEG2RAD*yaw, DEG2RAD*roll });
    
    // Constantly move player forward
    player->transform.position.z += 10.0f * timeDelta;

    printf("%f, %f", pitch, roll);

}

// Editor related functions. will be moved

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

