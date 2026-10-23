//
// hand_admin_demo.cpp - read EtherCAT board information, device information
// and protection temperature through SDO.
//
// This example is read-only and does not drive the motors.
//

#include <cstdio>
#include <memory>
#include <string>
#include <vector>

#include "DexHand.h"

using namespace DexRobot;
using namespace DexRobot::Dex021;

namespace
{

const char* moduleName(const HandModule module)
{
    switch (module)
    {
        case HandModule::Thumb:      return "thumb";
        case HandModule::FourFinger: return "four-finger";
        default:                     return "unknown";
    }
}

const char* partName(const FingerPart part)
{
    switch (part)
    {
        case FingerPart::ThumbDistal:        return "thumb-distal";
        case FingerPart::ThumbBase:          return "thumb-base";
        case FingerPart::IndexMcp:           return "index-mcp";
        case FingerPart::MiddleMcp:          return "middle-mcp";
        case FingerPart::RingMcp:            return "ring-mcp";
        case FingerPart::LittleMcp:          return "little-mcp";
        case FingerPart::IndexMiddleDistal:  return "index-middle-distal";
        case FingerPart::RingLittleDistal:   return "ring-little-distal";
        default:                             return "unknown";
    }
}

}   // namespace

int main()
{
    const std::vector<std::string> interfaces = DexHand::scanDexInterfaces(AdapterType::EC_SOEM);
    if (interfaces.empty())
    {
        std::printf("No adapter with EtherCAT slaves found.\n"
            "  - check the hand is powered and the cable is connected\n"
            "  - if windows plwase check Npcap is installed with WinPcap API compatible mode\n"
            "  - if ubuntu please run this program as Administrator\n");
        return 1;
    }

    std::printf("adapter: %s\n", interfaces.front().c_str());

    const auto hand = std::dynamic_pointer_cast<DexHand_Pro>(
        DexHand::createInstance(ProductType::DX021_PRO,
                                AdapterType::EC_SOEM,
                                interfaces.front()));

    if (!hand)
    {
        std::printf("failed to create DexHand_Pro instance.\n");
        return 2;
    }

    if (!hand->connect(true))
    {
        std::printf("connect failed [%s]: %s\n",
                    toString(hand->errorCode()),
                    hand->errorMessage().c_str());
        return 3;
    }

    std::vector<EcInfo> ecInfos;
    if (hand->readEcInfo(ecInfos))
    {
        std::printf("EtherCAT boards:\n");
        for (const EcInfo& info : ecInfos)
        {
            if (!info.valid)
                continue;

            std::printf("  %-12s sysId=0x%04X app=%u sn=%u temp=%.2f C admin=%d\n",
                        moduleName(info.module),
                        static_cast<unsigned>(info.sysId),
                        static_cast<unsigned>(info.appVer),
                        static_cast<unsigned>(info.sn),
                        info.curTemp / 100.0,
                        info.adminOn ? 1 : 0);
        }
    }
    else
    {
        std::printf("readEcInfo failed: %s\n", hand->errorMessage().c_str());
    }

    std::printf("devices:\n");
    for (uint8_t i = 0; i < static_cast<uint8_t>(FingerPart::Count); ++i)
    {
        const auto part = static_cast<FingerPart>(i);

        DeviceInfo info{};
        if (!hand->readDeviceInfo(part, info) || !info.valid)
        {
            std::printf("  %-20s not present\n", partName(part));
            continue;
        }

        uint16_t protectionTemp = 0;
        hand->readProtectionTemperature(part, protectionTemp);

        std::printf("  %-20s sysId=0x%04X app=%u sn=%u temp=%.2f C protect=%.2f C\n",
                    partName(part),
                    static_cast<unsigned>(info.sysId),
                    static_cast<unsigned>(info.appVer),
                    static_cast<unsigned>(info.sn),
                    info.curTemp / 100.0,
                    protectionTemp / 100.0);
    }

    hand->disconnect();
    std::printf("done.\n");
    return 0;
}
