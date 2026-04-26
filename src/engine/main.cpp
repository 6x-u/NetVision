/*
 * NetVision - main.cpp
 * Developer: MERO:TG@QP4RM
 */

#include "nv_engine.hpp"
#include <cstdio>

int main(int argc, char **argv)
{
    nv::Engine engine;

    if (engine.init(1280, 720, "NetVision - Network Visualization") != 0) {
        fprintf(stderr, "Failed to initialize NetVision engine\n");
        return 1;
    }

    engine.run();
    engine.shutdown();
    return 0;
}
