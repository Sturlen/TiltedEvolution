#pragma once

struct Party
{
    Party() {}

    std::optional<uint32_t> JoinedPartyId;
    TiltedPhoques::Map<Player*, uint64_t> Invitations;
};
