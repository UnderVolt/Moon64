#ifndef SM64Utils_H
#define SM64Utils_H

#include "utils.h"
#include "extractor/sound.h"

namespace sm64 = MoonUtils;

void compile_sound(std::string sequences, std::string soundbanks, std::string dir){
    const char* basedir = dir.c_str();
    assemble_banks(soundbanks, dir);

    sm64::exec(sm64::format(
        "as -I include -I./assets/sound/sequences/ --defsym AVOID_UB=1 -MD %s/sound/sequences/us/00_sound_player.d -o %s/sound/sequences/us/00_sound_player.o %s/00_sound_player.s",
        basedir, basedir, sequences.c_str()
    ));

    sm64::exec(sm64::format(
        "objcopy -j .rodata %s/sound/sequences/us/00_sound_player.o -O binary %s/sound/sequences/us/00_sound_player.m64",
        basedir, basedir
    ));

    assemble_m64(soundbanks, dir);

    sm64::rm(sm64::join(dir, "sound/samples"));
    sm64::rm(sm64::join(dir, "sound/sequences"));
}

#endif