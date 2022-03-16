#pragma once

enum ClientOpcode : unsigned char
{
    kAuthenticationRequest = 0,
    kCancelAssignmentRequest,
    kAssignCharacterRequest,
    kClientReferencesMoveRequest,
    kEnterExteriorCellRequest,
    kEnterInteriorCellRequest,
    kClientRpcCalls,
    kRequestInventoryChanges,
    kRequestFactionsChanges,
    kRequestQuestUpdate,
    kPartyInviteRequest,
    kPartyAcceptInviteRequest,
    kPartyLeaveRequest,
    kPartyCreateRequest,
    kPartyChangeLeaderRequest,
    kPartyKickRequest,
    kRequestActorValueChanges,
    kRequestActorMaxValueChanges,
    kRequestHealthChangeBroadcast,
    kActivateRequest,
    kLockChangeRequest,
    kAssignObjectsRequest,
    kRequestSpawnData,
    kRequestDeathStateChange,
    kShiftGridCellRequest,
    kRequestOwnershipTransfer,
    kRequestOwnershipClaim,
    kRequestObjectInventoryChanges,
    kRequestCharacterInventoryChanges,
    kSpellCastRequest,
    kInterruptCastRequest,
    kAddTargetRequest,
    kProjectileLaunchRequest,
    kScriptAnimationRequest,
    kDrawWeaponRequest,
    kMountRequest,
    kNewPackageRequest,
    kRequestRespawn,
    kClientOpcodeMax
};

enum ServerOpcode : unsigned char
{
    kAuthenticationResponse = 0,
    kAssignCharacterResponse,
    kServerReferencesMoveRequest,
    kServerScriptUpdate,
    kServerTimeSettings,
    kCharacterSpawnRequest,
    kNotifyInventoryChanges,
    kNotifyFactionsChanges,
    kNotifyRemoveCharacter,
    kNotifyQuestUpdate,
    kNotifyPlayerList,
    kNotifyPartyInfo,
    kNotifyPartyInvite,
    kNotifyPartyJoined,
    kNotifyPartyLeft,
    kNotifyActorValueChanges,
    kNotifyActorMaxValueChanges,
    kNotifyHealthChangeBroadcast,
    kNotifySpawnData,
    kNotifyActivate,
    kNotifyLockChange,
    kAssignObjectsResponse,
    kNotifyDeathStateChange,
    kNotifyOwnershipTransfer,
    kNotifyObjectInventoryChanges,
    kNotifyCharacterInventoryChanges,
    kNotifySpellCast,
    kNotifyInterruptCast,
    kNotifyAddTarget,
    kNotifyProjectileLaunch,
    kNotifyScriptAnimation,
    kNotifyDrawWeapon,
    kNotifyMount,
    kNotifyNewPackage,
    kNotifyRespawn,
    kServerOpcodeMax
};
