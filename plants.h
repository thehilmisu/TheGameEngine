#ifndef PLANTS_H
#define PLANTS_H

#include <stdint.h>
#include <raylib.h>

// L-system string generator
// Returns a heap-allocated string that must be freed by the caller
char* lsystem(uint32_t iterations, const char* axiom, const char* rule);

// Create a plant model from an L-system string
Model create_plant_from_str(const char* str, float angle, float length, float thickness, float decreaseAmt, unsigned int detail);

// Create specific tree models
Model create_pine_tree_model(unsigned int detail);
Model create_tree_model(unsigned int detail);

#endif
