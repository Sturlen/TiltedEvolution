#pragma once

struct Party
{
    Party() {}

    bool operator==(const Party& aRhs) 
    {
        return JoinedPartyId == aRhs.JoinedPartyId;
    }

    bool operator!=(const Party& aRhs) 
    {
        return operator==(aRhs);
    }

    uint32_t JoinedPartyId;
    Player* Leader;
    Vector<Player*> Members;
};
