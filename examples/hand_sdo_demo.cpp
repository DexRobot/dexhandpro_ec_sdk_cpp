//
// hand_sdo_demo.cpp - read EtherCAT board and device information through SDO.
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
        case FingerPart::ThumbDist:          return "thumb-distal";
        case FingerPart::ThumbProx:          return "thumb-prox";
        case FingerPart::IndexProx:          return "index-prox";
        case FingerPart::MiddleProx:         return "middle-prox";
        case FingerPart::RingProx:           return "ring-prox";
        case FingerPart::LittleProx:         return "little-prox";
        case FingerPart::IndexMiddleDistal:  return "index-middle-distal";
        case FingerPart::RingLittleDistal:   return "ring-little-distal";
        default:                             return "unknown";
    }
}

}   // namespace

int main()
{
    const std::vector<InterfaceInfo> interfaces = DexHand::scanDexInterfaces(AdapterType::EC_SOEM);
    if (interfaces.empty())
    {
        std::printf("No adapter with EtherCAT slaves found.\n"
            "  - check the hand is powered and the cable is connected\n"
            "  - Windows: install Npcap (WinPcap API compatible mode) and run as Administrator\n"
            "  - Linux: run as root or grant CAP_NET_RAW to this program\n");
        return 1;
    }

    std::printf("adapter: %s (%s)\n",
                interfaces.front().name.c_str(),
                interfaces.front().description.c_str());

    const auto hand = std::make_shared<DexHand_Pro>(
        AdapterType::EC_SOEM, interfaces.front().name);

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

        std::printf("  %-20s sysId=0x%04X app=%u sn=%u temp=%.2f C protect=%.2f C\n",
                    partName(part),
                    static_cast<unsigned>(info.sysId),
                    static_cast<unsigned>(info.appVer),
                    static_cast<unsigned>(info.sn),
                    info.curTemp / 100.0,
                    info.procTemp / 100.0);
    }

    hand->disconnect();
    std::printf("done.\n");
    return 0;
}
