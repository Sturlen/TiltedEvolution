#pragma once

struct Party
{
    Party() {}

    uint32_t JoinedPartyId;
    uint32_t LeaderPlayerId;
    Vector<Player*> Members;
};
