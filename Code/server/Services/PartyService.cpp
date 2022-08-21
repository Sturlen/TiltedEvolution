#include <Services/PartyService.h>
#include <Components.h>
#include <GameServer.h>
#include <Party.h>

#include <Events/PlayerJoinEvent.h>
#include <Events/PlayerLeaveEvent.h>
#include <Events/UpdateEvent.h>

#include <Messages/NotifyPlayerList.h>
#include <Messages/NotifyPartyInfo.h>
#include <Messages/NotifyPartyInvite.h>
#include <Messages/PartyInviteRequest.h>
#include <Messages/PartyAcceptInviteRequest.h>
#include <Messages/PartyLeaveRequest.h>
#include <Messages/NotifyPartyJoined.h>
#include <Messages/NotifyPartyLeft.h>
#include <Messages/PartyCreateRequest.h>
#include <Messages/PartyChangeLeaderRequest.h>
#include <Messages/PartyKickRequest.h>
#include <Messages/NotifyPlayerJoined.h>

PartyService::PartyService(World& aWorld, entt::dispatcher& aDispatcher) noexcept
    : m_world(aWorld)
    , m_updateEvent(aDispatcher.sink<UpdateEvent>().connect<&PartyService::OnUpdate>(this))
    , m_playerJoinConnection(aDispatcher.sink<PlayerJoinEvent>().connect<&PartyService::OnPlayerJoin>(this))
    , m_playerLeaveConnection(aDispatcher.sink<PlayerLeaveEvent>().connect<&PartyService::OnPlayerLeave>(this))
    , m_partyInviteConnection(aDispatcher.sink<PacketEvent<PartyInviteRequest>>().connect<&PartyService::OnPartyInvite>(this))
    , m_partyAcceptInviteConnection(aDispatcher.sink<PacketEvent<PartyAcceptInviteRequest>>().connect<&PartyService::OnPartyAcceptInvite>(this))
    , m_partyLeaveConnection(aDispatcher.sink<PacketEvent<PartyLeaveRequest>>().connect<&PartyService::OnPartyLeave>(this))
    , m_partyCreateConnection(aDispatcher.sink<PacketEvent<PartyCreateRequest>>().connect<&PartyService::OnPartyCreate>(this))
    , m_partyChangeLeaderConnection(aDispatcher.sink<PacketEvent<PartyChangeLeaderRequest>>().connect<&PartyService::OnPartyChangeLeader>(this)),
      m_partyKickConnection(aDispatcher.sink<PacketEvent<PartyKickRequest>>().connect<&PartyService::OnPartyKick>(this))
{
}

const Party* PartyService::GetById(uint32_t aId) const noexcept
{
    auto itor = m_parties.find(aId);
    if (itor != std::end(m_parties))
        return &itor->second;

    return nullptr;
}

bool PartyService::IsPlayerInParty(Player* const apPlayer) const noexcept
{
    return apPlayer->GetParty();
}

bool PartyService::IsPlayerLeader(Player* const apPlayer) noexcept
{
    auto inviterParty = apPlayer->GetParty();
    if (inviterParty)
    {
        Party& party = m_parties[inviterParty->JoinedPartyId];
        return party.LeaderPlayerId == apPlayer->GetId();
    }

    return false;
}

Party* PartyService::GetPlayerParty(Player* const apPlayer) noexcept
{
    auto inviterParty = apPlayer->GetParty();
    if (inviterParty)
    {
        return &m_parties[inviterParty->JoinedPartyId];
    }

    return nullptr;
}

void PartyService::OnUpdate(const UpdateEvent& acEvent) noexcept
{
    const auto cCurrentTick = GameServer::Get()->GetTick();
    if (m_nextInvitationExpire > cCurrentTick)
        return;

    // Only expire once every 10 seconds
    m_nextInvitationExpire = cCurrentTick + 10000;

}

void PartyService::OnPartyCreate(const PacketEvent<PartyCreateRequest>& acPacket) noexcept
{
    Player *const player = acPacket.pPlayer;

    spdlog::debug("[PartyService]: Received request to create party");

    CreateParty(player);
}

void PartyService::OnPartyChangeLeader(const PacketEvent<PartyChangeLeaderRequest>& acPacket) noexcept
{
    auto& message = acPacket.Packet;
    Player* const player = acPacket.pPlayer;
    Player* const pNewLeader = m_world.GetPlayerManager().GetById(message.PartyMemberPlayerId);

    spdlog::debug("[PartyService]: Received request to change party leader to {}", message.PartyMemberPlayerId);

    auto inviterParty = player->GetParty();
    if (inviterParty)
    {
        Party& party = m_parties[inviterParty->JoinedPartyId];
        if (party.LeaderPlayerId == player->GetId())
        {
            for (auto& pPlayer : party.Members)
            {
                if (pPlayer->GetId() == pNewLeader->GetId())
                {
                    party.LeaderPlayerId = pPlayer->GetId();
                    spdlog::debug("[PartyService]: Changed party leader to {}, updating party members.", party.LeaderPlayerId);
                    BroadcastPartyInfo(inviterParty->JoinedPartyId);
                    break;
                }
            }
        }
    }
}

void PartyService::OnPartyKick(const PacketEvent<PartyKickRequest>& acPacket) noexcept
{
    auto& message = acPacket.Packet;
    Player* const player = acPacket.pPlayer;
    Player* const pKick = m_world.GetPlayerManager().GetById(message.PartyMemberPlayerId);

    auto inviterParty = player->GetParty();
    if (inviterParty)
    {
        Party& party = m_parties[inviterParty->JoinedPartyId];
        if (party.LeaderPlayerId == player->GetId())
        {
            spdlog::debug("[PartyService]: Kicking player {} from party", pKick->GetId());
            RemovePlayerFromParty(pKick);
            BroadcastPlayerList(pKick);
        }
    }
}

void PartyService::OnPlayerJoin(const PlayerJoinEvent& acEvent) const noexcept
{
    BroadcastPlayerList();

    NotifyPlayerJoined notify{};
    notify.PlayerId = acEvent.pPlayer->GetId();
    notify.Username = acEvent.pPlayer->GetUsername();

    notify.WorldSpaceId = acEvent.WorldSpaceId;
    notify.CellId = acEvent.CellId;

    notify.Level = acEvent.pPlayer->GetLevel();

    spdlog::debug("[Party] New notify player {:x} {}", notify.PlayerId, notify.Username.c_str());

    GameServer::Get()->SendToPlayers(notify, acEvent.pPlayer);
}

void PartyService::OnPartyInvite(const PacketEvent<PartyInviteRequest>& acPacket) noexcept
{
    auto& message = acPacket.Packet;

    // Make sure the player we invite exists
    Player* const pInvitee = m_world.GetPlayerManager().GetById(message.PlayerId);
    Player* const pInviter = acPacket.pPlayer;

    // If both players are available and they are different
    if (pInvitee && pInvitee != pInviter)
    {
        auto inviterParty = pInviter->GetParty();
        auto inviteeParty = pInvitee->GetParty();

        spdlog::debug("[PartyService]: Got party invite from {}", pInviter->GetId());

        if (!inviterParty)
        {
            spdlog::debug("[PartyService]: Inviter not in party, cancelling invite.");
            return;
        }
        else if (inviteeParty)
        {
            spdlog::debug("[PartyService]: Invitee in party already, cancelling invite.");
            return;
        }

        auto& party = m_parties[inviterParty->JoinedPartyId];
        if (party.LeaderPlayerId != pInviter->GetId())
        {
            spdlog::debug("[PartyService]: Inviter not party leader, cancelling invite.");
            return;
        }

        // Expire in 60 seconds
        const auto cExpiryTick = GameServer::Get()->GetTick() + 60000;

        NotifyPartyInvite notification;
        notification.InviterId = inviterParty->JoinedPartyId;
        notification.ExpiryTick = cExpiryTick;

        spdlog::debug("[PartyService]: Sending party invite to {}", pInvitee->GetId());
        pInvitee->Send(notification);
    }
}

void PartyService::OnPartyAcceptInvite(const PacketEvent<PartyAcceptInviteRequest>& acPacket) noexcept
{
    auto& message = acPacket.Packet;

    Player* const pInviter = m_world.GetPlayerManager().GetById(message.InviterId);
    Player* pSelf = acPacket.pPlayer;

    spdlog::debug("[PartyService]: Got party accept request from {}", pSelf->GetId());

    if (pInviter && pInviter != pSelf)
    {
        auto selfParty = pSelf->GetParty();
        auto inviterParty = pInviter->GetParty();

        spdlog::debug("[PartyService]: Invite found, processing.");
        if (!inviterParty)
        {
            spdlog::error("[PartyService]: Inviter not in party. Cancelling.");
            return;
        }

        if (selfParty)
        {
            spdlog::error("[PartyService]: Cannot accept invite while in a party. Cancelling.");
            return;
        }

        auto partyId = inviterParty->JoinedPartyId;

        AddPlayerToParty(pSelf, partyId);
    }
}

void PartyService::OnPartyLeave(const PacketEvent<PartyLeaveRequest>& acPacket) noexcept
{
    RemovePlayerFromParty(acPacket.pPlayer);
}

void PartyService::OnPlayerLeave(const PlayerLeaveEvent& acEvent) noexcept
{
    RemovePlayerFromParty(acEvent.pPlayer);
    BroadcastPlayerList(acEvent.pPlayer);
}

std::optional<uint32_t> PartyService::CreateParty(Player* apLeader) noexcept
{
    spdlog::info("[PartyService]: Creating new party.");

    if (!apLeader)
    {
        spdlog::error("[PartyService]: Leader arg is nullptr");
        return {};
    }

    auto leaderId = apLeader->GetId();

    if (apLeader->GetParty())
    {
        spdlog::error("[PartyService]: Player '{}' is already in a party", leaderId);
        return {};
    }

    uint32_t partyId = m_nextId++;
    Party& party = m_parties[partyId];
    party.JoinedPartyId = partyId;
    party.Members.push_back(apLeader);
    party.LeaderPlayerId = leaderId;

    apLeader->SetParty(&party);

    spdlog::debug("[PartyService]: Created party for {}", leaderId);
    SendPartyJoinedEvent(party, apLeader);

    return partyId;
}

bool PartyService::AddPlayerToParty(Player* apPlayer, uint32_t aPartyId) noexcept
{
    spdlog::info("[PartyService]: Adding player to party.");
    if (!apPlayer)
    {
        spdlog::error("[PartyService]: Player arg is nullptr");
        return false;
    }

    if (apPlayer->GetParty())
    {
        spdlog::error("[PartyService]: Player is already in a party");
        return false;
    }

    auto& party = m_parties[aPartyId];

    party.Members.push_back(apPlayer);
    apPlayer->SetParty(&party);

    spdlog::info("[PartyService]: Added invitee to party, sending events");
    SendPartyJoinedEvent(party, apPlayer);
    BroadcastPartyInfo(aPartyId);

    return true;
}

void PartyService::RemovePlayerFromParty(Player* apPlayer) noexcept
{
    auto pParty = apPlayer->GetParty();
    spdlog::debug("[PartyService]: Removing player from party.");

    if (pParty)
    {
        auto id = pParty->JoinedPartyId;

        Party& party = m_parties[id];
        auto& members = party.Members;

        auto itr = std::remove(members.begin(), members.end(), apPlayer);
        members.erase(itr, members.end());

        if (members.empty())
        {
            m_parties.erase(id);
        }
        else
        {
            if (party.LeaderPlayerId == apPlayer->GetId())
            {
                party.LeaderPlayerId = members.at(0)->GetId(); // Reassign party leader
                spdlog::debug("[PartyService]: Leader left, reassigned party leader to {}", party.LeaderPlayerId);
            }
            spdlog::debug("[PartyService]: Updating other party players of removal.");
            BroadcastPartyInfo(id);
        }

        apPlayer->SetParty(nullptr);

        spdlog::debug("[PartyService]: Sending party left event to player.");
        NotifyPartyLeft leftMessage;
        apPlayer->Send(leftMessage);
    }
}

void PartyService::BroadcastPlayerList(Player* apPlayer) const noexcept
{
    auto pIgnoredPlayer = apPlayer;
    for (auto pSelf : m_world.GetPlayerManager())
    {
        if (pIgnoredPlayer == pSelf)
            continue;

        NotifyPlayerList playerList;
        for (auto pPlayer : m_world.GetPlayerManager())
        {
            if (pSelf == pPlayer)
                continue;

            if (pIgnoredPlayer == pPlayer)
                continue;

            playerList.Players[pPlayer->GetId()] = pPlayer->GetUsername();
        }

        pSelf->Send(playerList);
    }
}

void PartyService::BroadcastPartyInfo(uint32_t aPartyId) const noexcept
{
    auto itor = m_parties.find(aPartyId);
    if (itor == std::end(m_parties))
        return;

    auto& party = itor->second;
    auto& members = party.Members;

    NotifyPartyInfo message;
    message.LeaderPlayerId = party.LeaderPlayerId;

    for (auto pPlayer : members)
    {
        message.PlayerIds.push_back(pPlayer->GetId());
    }

    for (auto pPlayer : members)
    {
        message.IsLeader = pPlayer->GetId() == party.LeaderPlayerId;
        pPlayer->Send(message);
    }
}

void PartyService::SendPartyJoinedEvent(Party& aParty, Player* aPlayer) noexcept
{
    NotifyPartyJoined joinedMessage;
    joinedMessage.LeaderPlayerId = aParty.LeaderPlayerId;
    joinedMessage.IsLeader = aParty.LeaderPlayerId == aPlayer->GetId();
    for (auto pPlayer : aParty.Members)
    {
        joinedMessage.PlayerIds.push_back(pPlayer->GetId());
    }
    spdlog::debug("[PartyService]: Sending party join event to player");
    aPlayer->Send(joinedMessage);
}
