#pragma once

#include "Message.h"

#include <Structs/TimeModel.h>

struct ServerTimeSettings final : ServerMessage
{
    static constexpr ServerOpcode Opcode = kServerTimeSettings;

    ServerTimeSettings() : ServerMessage(Opcode)
    {
    }

    void SerializeRaw(TiltedPhoques::Buffer::Writer &aWriter) const noexcept override;
    void DeserializeRaw(TiltedPhoques::Buffer::Reader &aReader) noexcept override;

    bool operator==(const ServerTimeSettings &achRhs) const noexcept
    {
        return DateTime == achRhs.DateTime && GetOpcode() == achRhs.GetOpcode();
    }

    TimeModel DateTime;
};
