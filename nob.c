#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"

Cmd cmd = {0};

#define BUILD_FOLDER  "build/"

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);

   if (!nob_mkdir_if_not_exists(BUILD_FOLDER)) return 1;

    cmd_append(&cmd, "clang");
    cmd_append(&cmd, "-framework", "CoreVideo");
    cmd_append(&cmd, "-framework", "IOKit");
    cmd_append(&cmd, "-framework", "Cocoa");
    cmd_append(&cmd, "-framework", "GLUT");
    cmd_append(&cmd, "-framework", "OpenGL");
    cmd_append(&cmd, "-I./raylib-5.5_macos/include/");
    cmd_append(&cmd, "-o", BUILD_FOLDER"main", "main.c", "engine.c");
    cmd_append(&cmd, "./raylib-5.5_macos/lib/libraylib.a");
    cmd_append(&cmd, "-lm");
    
    if (!cmd_run(&cmd)) return 1;
    
    return 0;
}
