#ifndef HOMEBREW_GAMES_H
#define HOMEBREW_GAMES_H

#include <vector>
#include "usbloader/disc.h"

// File-backed applications are a source of their own, never NAND channels.
namespace HomebrewGames
{
    std::vector<discHdr> &Headers();
    int Launch(const discHdr *header);
}
#endif
