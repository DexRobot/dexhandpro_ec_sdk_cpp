//
// hand_move_demo.cpp - move one joint by +5 degrees, read it back, then disable.
//
// WARNING: this example drives the motors. Keep the hand clear of obstacles
// and make sure the hand is powered and properly mounted before running it.
//

#include <chrono>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>

#include "DexHand.h"

using namespace DexRobot;
using namespace DexRobot::Dex021;

namespace
{

constexpr JointId kDemoJoint = JointId::IndexDistCoupled;
constexpr float kDeltaDeg = 5.0f;

DexHand::PTR createHand()
{
    const std::vector<InterfaceInfo> interfaces = DexHand::scanDexInterfaces(AdapterType::EC_SOEM);
    if (interfaces.empty())
    {
        std::printf("No adapter with EtherCAT slaves found.\n"
            "  - check the hand is powered and the cable is connected\n"
            "  - if windows please check Npcap is installed with WinPcap API compatible mode\n"
            "  - if ubuntu please run this program as Administrator\n");
        return nullptr;
    }

    std::printf("adapter: %s (%s)\n",
                interfaces.front().name.c_str(),
                interfaces.front().description.c_str());

    return DexHand::createInstance(ProductType::DX021_PRO,
                                   AdapterType::EC_SOEM,
                                   interfaces.front().name);
}

}   // namespace

int main()
{
    DexHand::PTR hand = createHand();
    if (!hand)
        return 1;

    if (!hand->connect(true))
    {
        std::printf("connect failed [%s]: %s\n",
                    toString(hand->errorCode()),
                    hand->errorMessage().c_str());
        return 2;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    DX21StatusRxData::PTR status;
    if (!hand->getHandStatus(status) || status == nullptr)
    {
        std::printf("no status snapshot, abort.\n");
        hand->disconnect();
        return 3;
    }

    const JointState before = status->joint(kDemoJoint);
    if (!before.online)
    {
        std::printf("joint %d is offline, abort.\n", static_cast<int>(kDemoJoint));
        hand->disconnect();
        return 4;
    }

    if (!hand->setControlMode(handCtrlMode::MIT_CTRL_MODE, true))
    {
        std::printf("setControlMode failed: %s\n", hand->errorMessage().c_str());
        hand->disconnect();
        return 5;
    }

    HandCommand cmd;
    JointTarget& target = cmd.joints[static_cast<size_t>(kDemoJoint)];
    target.targetDeg = before.angle / 100.0f + kDeltaDeg;
    target.speedRpm = 25000;
    target.currentMa = 100;
    target.kp = 464;    // MIT position gain
    target.kd = 0;      // MIT damping
    target.enable = true;

    std::printf("WARNING: moving joint %d from %.2f to %.2f deg\n",
                static_cast<int>(kDemoJoint),
                before.angle / 100.0f,
                target.targetDeg);

    if (!hand->setHandCommand(cmd))
    {
        std::printf("setHandCommand failed: %s\n", hand->errorMessage().c_str());
        hand->disconnect();
        return 5;
    }

    std::this_thread::sleep_for(std::chrono::seconds(3));

    DX21StatusRxData::PTR after;
    if (hand->getHandStatus(after) && after != nullptr)
    {
        std::printf("joint %d readback: %.2f deg\n",
                    static_cast<int>(kDemoJoint),
                    after->joint(kDemoJoint).angle / 100.0f);
    }

    // Empty command disables all motors.
    hand->setHandCommand(HandCommand{});
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    hand->disconnect();
    std::printf("done.\n");
    return 0;
}
