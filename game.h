#ifndef GAME_H
#define GAME_H

#include <stdint.h>
#include <stdbool.h>
#include <raylib.h>
#include "nob.h"

// --- ECS CORE ---

#define ENTITY_INVALID 0

typedef struct 
{
    Vector3 position;
    Vector3 rotation; // Euler angles in degrees
    Vector3 scale;
} TransformComponent;

typedef enum 
{
    MESH_CUBE,
    MESH_SPHERE,
    MESH_PLANE,
    MESH_MODEL
} MeshType;

typedef struct 
{
    MeshType type;
    Color color;
    Model model;
} MeshComponent;

typedef struct 
{
    bool is_selected;
    bool is_hovered;
} EditorComponent;

typedef enum
{
    ENTITY_NONE,
    ENTITY_PLAYER,
    ENTITY_PROJECTILE,
    ENTITY_SCENERY
} EntityType;

typedef struct
{
    Vector3 direction;
    float speed;
    float lifetime;
} ProjectileComponent;

typedef struct
{
    uint32_t id;
    bool active;
    EntityType type;
    TransformComponent transform;
    MeshComponent mesh;
    ProjectileComponent projectile;
    EditorComponent editor;
} Entity;

typedef struct 
{
    Entity *entities;
    size_t count;
    size_t capacity;
} Registry;

typedef struct
{
    Registry reg;
    uint32_t player_id;
} Game;

Game game_init(void);
Entity* create_entity(Game *game, EntityType type);
Entity* create_entity_with_model(Game *game, EntityType type, const char* model_path);
void remove_entity(Game *game, uint32_t id);
void spawn_projectile(Game *game, Vector3 position, Vector3 direction);
void game_free(Game *game);

typedef enum 
{
    GIZMO_NONE,
    GIZMO_X,
    GIZMO_Y,
    GIZMO_Z
} GizmoAxis;

typedef struct 
{
    GizmoAxis active_axis;
    float drag_start_t;
    Vector3 drag_entity_start_pos;
    Entity selected_entity;
} EditorState;

void game_render(Game *game, Camera3D *camera);
void game_update(Game *game, float timeDelta);
void handle_input(Game* game, float timeDelta);
void editor_update(Game *game, EditorState *editor, Camera3D *camera);
void system_editor_render(Registry *reg, EditorState *editor, Camera3D *camera);

// Procedurally spawn trees across the terrain
void spawn_trees_on_terrain(Game *game, Model tree_model, int count, unsigned int seed);

// Clear all scenery entities (trees, etc.)
void clear_scenery(Game *game);

#endif // GAME_H

