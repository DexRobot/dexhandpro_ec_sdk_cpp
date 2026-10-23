#pragma once
//
// DexHand.h - public API for DexHand 021Pro EtherCAT.
//
// Public addressing model: Hand / Joint / Pusher.
// EtherCAT slaves, boards and devices are internal details.
//

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "commondef.h"

namespace DexRobot
{
namespace Dex021
{

enum class HandSide : uint8_t
{
    Unknown = 0,
    Left,
    Right,
};

// User-facing module of one hand.
enum class HandModule : uint8_t
{
    Thumb = 0,
    FourFinger,
};

// User-facing controllable part. Maps 1:1 to (board role, internal device).
enum class FingerPart : uint8_t
{
    ThumbDistal = 0,       // thumb device 0: TDIP + TPIP
    ThumbBase,             // thumb device 1: TM_R + TM_F

    IndexMcp,              // four-finger device 0
    MiddleMcp,             // four-finger device 1
    RingMcp,               // four-finger device 2
    LittleMcp,             // four-finger device 3
    IndexMiddleDistal,     // four-finger device 4: index + middle distal
    RingLittleDistal,      // four-finger device 5: ring + little distal

    Count
};

enum class JointId : uint8_t
{
    ThumbDistal = 0,
    ThumbProximal,
    ThumbRotation,
    ThumbFlex,

    IndexAbduction,
    IndexFlex,
    IndexDistalCoupled,

    MiddleAbduction,
    MiddleFlex,
    MiddleDistalCoupled,

    RingAbduction,
    RingFlex,
    RingDistalCoupled,

    LittleAbduction,
    LittleFlex,
    LittleDistalCoupled,

    Count
};

struct JointState
{
    bool    online = false;
    bool    overCurrent = false;
    bool    overTemp = false;
    bool    hallFault = false;
    bool    stall = false;
    bool    angleError = false;

    int16_t angle = 0;       // 0.01 degree
    int16_t velocity = 0;    // rpm
    int16_t current = 0;     // mA
    int16_t hall = 0;        // V1.1 only
};

// Immutable status snapshot of one hand.
class DX21StatusRxData
{
public:
    using PTR = std::shared_ptr<DX21StatusRxData>;

    virtual ~DX21StatusRxData() = default;

    virtual HandSide side() const = 0;
    virtual int64_t  timestamp() const = 0;   // steady_clock, microseconds

    virtual bool       hasJoint(JointId joint) const = 0;
    virtual JointState joint(JointId joint) const = 0;

    virtual bool hasHall() const = 0;
    virtual bool hasPidGain() const = 0;
};

using StatusRxDataCallBack = std::function<void(const char* sn, const DX21StatusRxData* status)>;

struct ModuleInfo
{
    HandModule module = HandModule::Thumb;
    PdoVersion version = PdoVersion::UNKNOWN;
    bool       op = false;
};

struct PusherInfo
{
    uint8_t id = 0;
    bool    op = false;
    bool    hasForceSensor = false;
};

struct HandInfo
{
    HandSide   side = HandSide::Unknown;
    ModuleInfo modules[2];
    uint8_t    moduleCount = 2;
    uint8_t    pusherCount = 0;
    PusherInfo pushers[2];
};

// EtherCAT board information.
struct EcInfo
{
    bool       valid = false;
    HandModule module = HandModule::Thumb;
    uint16_t   sysId = 0;
    uint16_t   appVer = 0;
    uint16_t   bootVer = 0;
    uint16_t   hwVer = 0;
    uint16_t   testTime = 0;
    uint32_t   sn = 0;
    uint16_t   status = 0;
    int16_t    curTemp = 0;      // 0.01 Celsius
    bool       adminOn = false;
};

// Ctrl device information.
struct DeviceInfo
{
    bool     valid = false;
    uint16_t sysId = 0;
    uint16_t appVer = 0;
    uint16_t bootVer = 0;
    uint16_t hwVer = 0;
    uint16_t testTime = 0;
    uint32_t appLen = 0;
    uint32_t appCrc = 0;
    uint32_t sn = 0;
    uint16_t status = 0;
    int16_t  curTemp = 0;        // 0.01 Celsius
    uint16_t procTemp = 0;       // 0.01 Celsius
    uint16_t iapFlag = 0;
    uint16_t angle0Zero0 = 0;
    uint16_t angle0Zero1 = 0;
    uint16_t angle1Zero0 = 0;
    uint16_t angle1Zero1 = 0;
    uint16_t ratedCur = 0;
    uint16_t ratedRpm = 0;
    uint16_t ratedTemp = 0;
};

struct JointTarget
{
    float    targetDeg = 0.0f;
    uint16_t speedRpm = 0;
    uint8_t  currentMa = 0;
    int16_t  kp = 0;
    int16_t  kd = 0;
    bool     enable = false;
};

struct HandCommand
{
    uint8_t mode = 0;          // Ctrl_Mode_e, 4 = angle mode
    bool    clearError = false;
    std::array<JointTarget, static_cast<size_t>(JointId::Count)> joints{};
};

class DexHand
{
public:
    using PTR = std::shared_ptr<DexHand>;

public:
    DexHand() = delete;
    virtual ~DexHand();

    DEXHAND_API static std::vector<std::string> scanInterfaces(
        AdapterType adapterType = AdapterType::EC_SOEM);
    DEXHAND_API static std::vector<std::string> scanDexInterfaces(
        AdapterType adapterType = AdapterType::EC_SOEM);

    DEXHAND_API static PTR createInstance(ProductType productType,
                                          AdapterType adapterType,
                                          const std::string ifName);

    DEXHAND_API virtual bool connect(bool listen) = 0;
    DEXHAND_API virtual bool disconnect() = 0;
    DEXHAND_API virtual bool isConnected() const = 0;

    DEXHAND_API virtual bool handInfo(HandInfo& out) const = 0;

    DEXHAND_API virtual void setStatusRxCallback(const StatusRxDataCallBack& cb) const = 0;
    DEXHAND_API virtual bool getHandStatus(DX21StatusRxData::PTR& out) const = 0;
    DEXHAND_API virtual bool isPdoHealthy() const = 0;

    // Status callback rate; clamped to 100..1000 Hz. Default 100 Hz.
    virtual bool setRealtimeResponse(uint16_t rateHz, bool enable)
    {
        (void)rateHz;
        (void)enable;
        return false;
    }

    DEXHAND_API virtual bool setHandCommand(const HandCommand& cmd) = 0;
    DEXHAND_API virtual bool getHandCommand(HandCommand& out) const = 0;
    DEXHAND_API virtual bool moveJoint(JointId joint, const JointTarget& target) = 0;

    DEXHAND_API void listen(bool enable) const;
    DEXHAND_API bool isListening() const;
    DEXHAND_API bool isAvailable() const;

    DEXHAND_API virtual uint32_t getFirmwareVersion(uint8_t deviceId, uint8_t fingerId) = 0;
    DEXHAND_API virtual uint8_t getSafeTemperature(uint8_t deviceId,
                                                   uint8_t fingerId,
                                                   uint8_t jointPosition) = 0;
    DEXHAND_API virtual ProductType productType() const = 0;
    DEXHAND_API virtual void resetJoints() = 0;

    DEXHAND_API SysErrorCode errorCode() const;
    DEXHAND_API const std::string& errorMessage() const;

    DEXHAND_API void setTxLogLevel(LOG_LEVEL level) const;
    DEXHAND_API void setRxLogLevel(LOG_LEVEL level) const;

protected:
    DexHand(AdapterType adapterType, const std::string ifName);

    class DexHandImpl* impl = nullptr;
};

class DexHand_Pro : public DexHand
{
public:
    using PTR = std::shared_ptr<DexHand_Pro>;

public:
    DexHand_Pro() = delete;

    DEXHAND_API DexHand_Pro(AdapterType adapterType, const std::string ifName);
    ~DexHand_Pro() override;

    DEXHAND_API bool connect(bool listen) override;
    DEXHAND_API bool disconnect() override;
    DEXHAND_API bool isConnected() const override;

    DEXHAND_API bool handInfo(HandInfo& out) const override;

    DEXHAND_API void setStatusRxCallback(const StatusRxDataCallBack& cb) const override;
    DEXHAND_API bool getHandStatus(DX21StatusRxData::PTR& out) const override;
    DEXHAND_API bool isPdoHealthy() const override;
    DEXHAND_API bool setRealtimeResponse(uint16_t rateHz, bool enable) override;

    DEXHAND_API bool setHandCommand(const HandCommand& cmd) override;
    DEXHAND_API bool getHandCommand(HandCommand& out) const override;
    DEXHAND_API bool moveJoint(JointId joint, const JointTarget& target) override;

    DEXHAND_API uint32_t getFirmwareVersion(uint8_t deviceId, uint8_t fingerId) override;
    DEXHAND_API uint8_t getSafeTemperature(uint8_t deviceId,
                                           uint8_t fingerId,
                                           uint8_t jointPosition) override;
    DEXHAND_API ProductType productType() const override;
    DEXHAND_API void resetJoints() override;

    //hand public SDO operations
    // Device information. Available while connected.
    DEXHAND_API bool readEcInfo(std::vector<EcInfo>& out);
    DEXHAND_API bool readDeviceInfo(FingerPart part, DeviceInfo& out);
    DEXHAND_API bool readProtectionTemperature(FingerPart part, uint16_t& centiCelsius);

    // Device operations.
    DEXHAND_API bool clearDeviceError(FingerPart part);
    DEXHAND_API bool restartDevice(FingerPart part);
    DEXHAND_API bool setFillLight(FingerPart part, uint8_t brightness);
    //hand public SDO operations~


private:
    class DexHand021ProImpl* pimpl = nullptr;
};

}
}
