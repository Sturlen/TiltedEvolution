#pragma once

struct Party
{
    Party() {}

    uint32_t JoinedPartyId;
    Player* Leader;
    Vector<Player*> Members;
};
