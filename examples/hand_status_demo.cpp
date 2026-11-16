//
// hand_status_demo.cpp - scan adapters, connect, receive status callbacks,
// print one snapshot of all 16 joints.
//
// This example is read-only and does not drive the motors.
//

#include <atomic>
#include <chrono>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>

#include "DexHand.h"
#include "DateTimeUtils.h"

using namespace DexRobot;
using namespace DexRobot::Dex021;

namespace
{

const char* versionName(const PdoVersion version)
{
    switch (version)
    {
        case PdoVersion::V1_1: return "V1.1";
        default:               return "unknown";
    }
}

const char* moduleName(const HandModule module)
{
    switch (module)
    {
        case HandModule::Thumb:      return "thumb";
        case HandModule::FourFinger: return "four-finger";
        default:                     return "unknown";
    }
}

DexHand::PTR createHand()
{
    const std::vector<InterfaceInfo> interfaces = DexHand::scanDexInterfaces(AdapterType::EC_SOEM);
    if (interfaces.empty())
    {
        std::printf("No adapter with EtherCAT slaves found.\n"
                    "  - check the hand is powered and the cable is connected\n"
                    "  - Windows: install Npcap (WinPcap API compatible mode) and run as Administrator\n"
                    "  - Linux: run as root or grant CAP_NET_RAW to this program\n");
        return nullptr;
    }

    std::printf("adapter: %s (%s)\n",
                interfaces.front().name.c_str(),
                interfaces.front().description.c_str());

    DexHand::PTR hand = DexHand::createInstance(ProductType::DX021_PRO,
                                                AdapterType::EC_SOEM,
                                                interfaces.front().name);
    if (!hand)
        std::printf("createInstance failed.\n");

    return hand;
}

void printHandInfo(DexHand& hand)
{
    HandInfo info{};
    if (!hand.getHandInfo(info))
    {
        std::printf("getHandInfo failed: %s\n", hand.errorMessage().c_str());
        return;
    }

    const char* side = info.side == HandSide::Left ? "left"
                     : info.side == HandSide::Right ? "right" : "unknown";

    std::printf("hand side: %s, pushers: %u\n", side, static_cast<unsigned>(info.pusherCount));

    for (uint8_t i = 0; i < info.moduleCount && i < 2; ++i)
    {
        const ModuleInfo& module = info.modules[i];
        std::printf("  %-12s %-5s op=%d\n",
                    moduleName(module.module),
                    versionName(module.version),
                    module.op ? 1 : 0);
    }
}

void printJoints(const DX21StatusRxData& status)
{
    auto printGroup = [&status](const char* label, const int begin, const int end)
    {
        std::printf("  %-8s", label);
        for (int i = begin; i <= end; ++i)
        {
            const JointState joint = status.joint(static_cast<JointId>(i));
            std::printf(" %7.2f", joint.angle / 100.0);
        }
        std::printf("\n");
    };

    printGroup("thumb",  0,  3);
    printGroup("index",  4,  6);
    printGroup("middle", 7,  9);
    printGroup("ring",  10, 12);
    printGroup("little",13, 15);
}

void printDistalSensors(const DX21StatusRxData& status)
{
    static const JointId joints[] = {
        JointId::IndexDistCoupled,
        JointId::MiddleDistCoupled,
        JointId::RingDistCoupled,
        JointId::LittleDistCoupled,
    };

    std::printf("four-finger distal sensors (deg):\n");
    for (const JointId id : joints)
    {
        const JointState s = status.joint(id);
        std::printf("  joint %2d: total=%7.2f primary=%7.2f follower=%7.2f hasFollower=%d\n",
                    static_cast<int>(id),
                    s.angle / 100.0,
                    s.anglePrimary / 100.0,
                    s.angleFollower / 100.0,
                    s.hasFollower ? 1 : 0);
    }
}

}   // namespace

int main()
{
    const std::string startTime = timestamp_to_loc_string(current_timestamp(), MICROSECOND);
    std::printf("started at %s\n", startTime.c_str());

    DexHand::PTR hand = createHand();
    if (!hand)
        return 1;

    std::atomic<uint64_t> frames{ 0 };

    hand->setStatusRxCallback([&frames](const char*, const DX21StatusRxData* status)
    {
        if (status != nullptr)
            frames.fetch_add(1, std::memory_order_relaxed);
    });

    if (!hand->connect(true))
    {
        std::printf("connect failed [%s]: %s\n",
                    toString(hand->errorCode()),
                    hand->errorMessage().c_str());
        return 2;
    }

    hand->setRealtimeResponse(100, true);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    printHandInfo(*hand);

    DX21StatusRxData::PTR status;
    if (hand->getHandStatus(status) && status != nullptr)
    {
        std::printf("joint angles (deg):\n");
        printJoints(*status);
        printDistalSensors(*status);
    }
    else
    {
        std::printf("no status snapshot available.\n");
    }

    std::printf("running 10 s at 100 Hz callback...\n");
    for (int i = 0; i < 10; ++i)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::printf("  t=%2ds callbacks=%llu\n",
                    i + 1,
                    static_cast<unsigned long long>(frames.load(std::memory_order_relaxed)));
    }

    hand->disconnect();
    std::printf("done.\n");
    return 0;
}
