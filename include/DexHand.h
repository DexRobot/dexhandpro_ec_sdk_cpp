#pragma once
/**
 * @file DexHand.h
 * @brief Public API for DexHand 021Pro EtherCAT.
 *
 * Public: Hand / Joint / Pusher <--map--> Internal: slaves / boards / devices.
 *
 * @warning Built with MSVC v143, x64, C++20. Release packages ship Release
 *          binaries only; build consumers with Release or RelWithDebInfo
 *          (/MD). Debug (/MDd) consumers are not supported.
 * @warning One SDK instance owns one EtherCAT interface and one hand. A hand
 *          exclusively owns the bus.
 *
 * Threading:
 * - Status and error callbacks run on SDK threads.
 * - Callbacks must not block or call connect/disconnect/SDO.
 * - The status pointer is valid only during the callback.
 */

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "commondef.h"

// The release package ships Release-only binaries. Debug builds use the debug
// CRT and STL (_ITERATOR_DEBUG_LEVEL != 0), which are ABI-incompatible.
#if defined(DEXHAND_RELEASE_ONLY) && defined(_ITERATOR_DEBUG_LEVEL) && (_ITERATOR_DEBUG_LEVEL != 0)
#error "DexHandPro ships Release-only binaries. Build with Release or RelWithDebInfo (/MD), not Debug (/MDd)."
#endif

namespace DexRobot
{
namespace Dex021
{

/**
 * @brief Side of the hand.
 */
enum class HandSide : uint8_t
{
    Unknown = 0,   ///< Not identified.
    Left,          ///< Left hand.
    Right,         ///< Right hand.
};

/**
 * @brief Network interface available for EtherCAT.
 */
struct InterfaceInfo
{
    std::string name;         ///< Interface name (Npcap device path).
    std::string description;  ///< Physical adapter description.
};

/**
 * @brief Hand board identified on the bus.
 *
 * A complete hand has one thumb board and one four-finger board.
 */
enum class HandModule : uint8_t
{
    Thumb = 0,   ///< Thumb board.
    FourFinger,  ///< Four-finger board.
    Unknown,     ///< Blank board or module not identified.
};

/**
 * @brief Tactile region exposed by the SDK.
 *
 * The protocol sensor ID is hidden behind this enum. Finger regions report up
 * to 30 channels, palm regions up to 24 channels, and force groups contain
 * 1, 2, or 4 values.
 */
enum class TactilePart : uint8_t
{
    ThumbTip = 0,       ///< Thumb fingertip.
    ThumbPad,           ///< Thumb finger pad.
    IndexTip,           ///< Index fingertip.
    IndexPad,           ///< Index finger pad.
    MiddleTip,          ///< Middle fingertip.
    MiddlePad,          ///< Middle finger pad.
    RingTip,            ///< Ring fingertip.
    RingPad,            ///< Ring finger pad.
    LittleTip,          ///< Little fingertip.
    LittlePad,          ///< Little finger pad.
    Palm1,              ///< Palm region 1.
    Palm2,              ///< Palm region 2.
    Palm3,              ///< Palm region 3.
    Palm4,              ///< Palm region 4.

    ThumbForce,         ///< Thumb force group, 2 values.
    PalmForce,          ///< Palm force group, 1 value.
    IndexMiddleForce,   ///< Index and middle force group, 4 values.
    RingLittleForce,    ///< Ring and little force group, 4 values.

    Count               ///< Number of regions. Not a valid region.
};

/**
 * @brief Latest tactile values for one region.
 */
struct TactileData
{
    std::array<uint16_t, 30> values{};  ///< Channel values; valid entries are [0, count).
    uint8_t count = 0;                  ///< 30=finger, 24=palm, 2/1/4=force.
};

/**
 * @brief Finger part mapped to an internal CTRL device.
 *
 * A device may drive more than one mechanical joint.
 */
enum class FingerPart : uint8_t
{
    ThumbDist = 0,         ///< Thumb device 0: TDIP + TPIP.
    ThumbProx,             ///< Thumb device 1: TM_R + TM_F.

    IndexProx,             ///< Four-finger device 0.
    MiddleProx,            ///< Four-finger device 1.
    RingProx,              ///< Four-finger device 2.
    LittleProx,            ///< Four-finger device 3.
    IndexMiddleDistal,     ///< Four-finger device 4: index + middle distal.
    RingLittleDistal,      ///< Four-finger device 5: ring + little distal.

    Count                  ///< Number of parts. Not a valid part.
};

/**
 * @brief Semantic joint exposed by the SDK.
 *
 * Distal joints share one motor pair with their proximal partner and are
 * reported through anglePrimary/angleFollower.
 */
enum class JointId : uint8_t
{
    ThumbDist = 0,         ///< Thumb distal.
    ThumbMidd,             ///< Thumb middle.
    ThumbProxSwng,         ///< Thumb proximal swing.
    ThumbProxFlex,         ///< Thumb proximal flex.

    IndexProxSwng,         ///< Index proximal swing.
    IndexProxFlex,         ///< Index proximal flex.
    IndexDistCoupled,      ///< Index coupled distal.

    MiddleProxSwng,        ///< Middle proximal swing.
    MiddleProxFlex,        ///< Middle proximal flex.
    MiddleDistCoupled,     ///< Middle coupled distal.

    RingProxSwng,          ///< Ring proximal swing.
    RingProxFlex,          ///< Ring proximal flex.
    RingDistCoupled,       ///< Ring coupled distal.

    LittleProxSwng,        ///< Little proximal swing.
    LittleProxFlex,        ///< Little proximal flex.
    LittleDistCoupled,     ///< Little coupled distal.

    Count                  ///< Number of joints. Not a valid joint.
};

/**
 * @brief Snapshot of one semantic joint.
 */
struct JointState
{
    bool    online = false;          ///< Joint is online.
    bool    overCurrent = false;     ///< Over-current flag.
    bool    overTemp = false;        ///< Over-temperature flag.
    bool    hallFault = false;       ///< Hall sensor fault.
    bool    stall = false;           ///< Motor stall flag.
    bool    angleError = false;      ///< Angle error flag.

    int16_t angle = 0;               ///< Semantic angle, 0.01 degree; equals primary + follower for distal joints.
    int16_t velocity = 0;            ///< Motor speed, rpm.
    int16_t current = 0;             ///< Motor current, mA.
    int16_t hall = 0;                ///< Hall value.

    int16_t anglePrimary = 0;        ///< Ang0, 0.01 degree.
    int16_t angleFollower = 0;       ///< Ang1, 0.01 degree; valid when hasFollower.
    bool    hasFollower = false;     ///< True for four-finger distal joints.
    bool    angleErrorPrimary = false;  ///< Primary angle error.
    bool    angleErrorFollower = false; ///< Follower angle error.
};

/**
 * @brief Immutable status snapshot delivered to the status callback.
 *
 * The object is valid while the callback runs. Copy any data that must outlive
 * the callback. A snapshot is published only when callbacks are enabled.
 */
class DX21StatusRxData
{
public:
    using PTR = std::shared_ptr<DX21StatusRxData>;

    virtual ~DX21StatusRxData() = default;

    /// @brief Hand side reported by this snapshot.
    virtual HandSide side() const = 0;

    /// @brief Master-side receive time, steady_clock microseconds.
    ///
    /// This is not the slave-side sample time. Firmware sequence/timestamp
    /// support is reserved for a future PDO version.
    virtual int64_t receiveTimestamp() const = 0;

    /// @brief True when the joint index is in range.
    virtual bool hasJoint(JointId joint) const = 0;

    /// @brief Joint state; returns a default state for an invalid index.
    virtual JointState joint(JointId joint) const = 0;
};

/**
 * @brief Status snapshot callback.
 * @param ifName Interface name; valid for the lifetime of the hand object.
 * @param status Snapshot pointer; valid only during the callback.
 * @warning Runs on the PDO thread. Do not block or call SDO/connect/disconnect.
 */
using StatusRxDataCallBack = std::function<void(const char* ifName, const DX21StatusRxData* status)>;

/**
 * @brief Error notification callback.
 * @param error Error snapshot; valid only during the callback.
 * @warning Runs on the thread that detected the error. Keep it short.
 */
using ErrorRxCallBack = std::function<void(const ErrorInfo& error)>;

/**
 * @brief Information about one hand board module.
 */
struct ModuleInfo
{
    HandModule module = HandModule::Thumb;       ///< Board role.
    PdoVersion version = PdoVersion::UNKNOWN;    ///< PDO layout version.
    bool       op = false;                       ///< Slave is in OP.
};

/**
 * @brief Pusher index.
 */
enum class PusherId : uint8_t
{
    Pusher0 = 0,   ///< First pusher.
    Pusher1 = 1,   ///< Second pusher.
    Count   = 2,   ///< Number of pushers.
};

/**
 * @brief Information about one pusher slave.
 */
struct PusherInfo
{
    PusherId id = PusherId::Pusher0;   ///< Pusher index.
    bool     op = false;               ///< Slave is in OP.
    bool     hasForceSensor = false;   ///< Force sensor is mapped.
};

/**
 * @brief Topology snapshot of the connected hand.
 */
struct HandInfo
{
    HandSide   side = HandSide::Unknown;  ///< Hand side.
    uint8_t    moduleCount = 2;           ///< Number of hand boards found.
    uint8_t    pusherCount = 0;           ///< Number of pushers found.
    ModuleInfo modules[2];                ///< Hand boards; valid [0, moduleCount).
    PusherInfo pushers[2];                ///< Pushers; valid [0, pusherCount).
};

/**
 * @brief Runtime transport and PDO diagnostics.
 */
struct HandDiagnostics
{
    bool     adapterAvailable = false;     ///< Adapter is open.
    bool     controlConnected = false;     ///< Control session is active.
    bool     maintenanceConnected = false; ///< Maintenance session is active.
    bool     pdoHealthy = false;           ///< Last PDO cycle met expected WKC.
    uint16_t expectedWkc = 0;              ///< Expected working counter.
    uint16_t lastWkc = 0;                  ///< Last observed working counter.
};

/**
 * @brief Parsed factory serial number.
 *
 * The SDK decodes the stored value; text is empty when the value is
 * unprogrammed or out of range.
 */
struct SerialNumber
{
    bool        valid = false;  ///< Stored value is a valid factory serial.
    std::string text;           ///< DX022-S-L-M-YYYYMMDDNNNN; empty when !valid.
};

/**
 * @brief EC board information read through SDO.
 */
struct EcInfo
{
    bool       valid = false;             ///< Data was read successfully.
    HandModule module = HandModule::Thumb;///< Board role.
    uint16_t   sysId = 0;                 ///< Raw SYS_ID (0x8100:02).
    uint16_t   appVer = 0;                ///< Raw app version; decode with decodeVersion().
    uint16_t   bootVer = 0;               ///< Raw boot version; decode with decodeVersion().
    uint16_t   hwVer = 0;                 ///< Raw hardware version.
    uint16_t   testTime = 0;              ///< Raw test timestamp.
    uint32_t   sn = 0;                    ///< Raw EC serial number.
    SerialNumber serial;                  ///< Decoded sn; valid=false when unprogrammed.
    uint16_t   status = 0;                ///< EC status word.
    int16_t    curTemp = 0;               ///< Current temperature, 0.01 Celsius.
    bool       adminOn = false;           ///< Admin mode is active.
};

/**
 * @brief CTRL device information read through SDO.
 */
struct DeviceInfo
{
    bool     valid = false;               ///< Data was read successfully.
    uint16_t sysId = 0;                   ///< Raw device SYS_ID.
    uint16_t appVer = 0;                  ///< Raw app version; decode with decodeVersion().
    uint16_t bootVer = 0;                 ///< Raw boot version; decode with decodeVersion().
    uint16_t hwVer = 0;                   ///< Raw hardware version.
    uint16_t testTime = 0;                ///< Raw test timestamp.
    uint32_t appLen = 0;                  ///< Application image length.
    uint32_t appCrc = 0;                  ///< Application image CRC.
    uint32_t sn = 0;                      ///< Raw device serial number.
    SerialNumber serial;                  ///< Decoded sn; valid=false when unprogrammed.
    uint16_t status = 0;                  ///< Device status word.
    int16_t  curTemp = 0;                 ///< Current temperature, 0.01 Celsius.
    uint16_t procTemp = 0;                ///< Protection temperature, 0.01 Celsius.
    uint16_t iapFlag = 0;                 ///< IAP/boot flag.
    uint16_t angle0Zero0 = 0;             ///< Angle0 zero calibration, channel 0.
    uint16_t angle0Zero1 = 0;             ///< Angle0 zero calibration, channel 1.
    uint16_t angle1Zero0 = 0;             ///< Angle1 zero calibration, channel 0.
    uint16_t angle1Zero1 = 0;             ///< Angle1 zero calibration, channel 1.
    uint16_t ratedCur = 0;                ///< Rated current.
    uint16_t ratedRpm = 0;                ///< Rated speed.
    uint16_t ratedTemp = 0;               ///< Rated temperature.
};

/**
 * @brief Parsed x.x.x firmware version.
 *
 * valid is false when the raw field is 0 or 0xFFFF.
 */
struct FirmwareVersion
{
    bool    valid = false;   ///< Version fields are valid.
    uint8_t major = 0;       ///< Major version.
    uint8_t minor = 0;       ///< Minor version.
    uint8_t patch = 0;       ///< Patch version.

    bool operator==(const FirmwareVersion&) const = default;
};

/**
 * @brief Decode a raw Dex_Version field.
 * @param raw Raw field: major(15-11) / minor(10-6) / patch(5-0).
 * @return Parsed version; valid=false for 0 or 0xFFFF.
 */
[[nodiscard]] inline FirmwareVersion decodeVersion(const uint16_t raw)
{
    FirmwareVersion version;
    if (raw == 0 || raw == 0xFFFF)
        return version;

    version.valid = true;
    version.major = static_cast<uint8_t>(raw >> 11);
    version.minor = static_cast<uint8_t>((raw >> 6) & 0x1F);
    version.patch = static_cast<uint8_t>(raw & 0x3F);
    return version;
}

/**
 * @brief Format a firmware version as "major.minor.patch".
 * @param version Parsed version.
 * @return "invalid" when version.valid is false.
 */
[[nodiscard]] inline std::string toString(const FirmwareVersion& version)
{
    if (!version.valid)
        return "invalid";

    return std::to_string(version.major) + "."
         + std::to_string(version.minor) + "."
         + std::to_string(version.patch);
}

/// @brief Whole-SDK version string, e.g. "1.0.2".
DEXHAND_API const char* sdkVersion();

/**
 * @brief Target for one semantic joint.
 */
struct JointTarget
{
    float    targetDeg = 0.0f;  ///< Target angle, degree.
    uint16_t speedRpm = 0;      ///< Target speed, rpm.
    uint8_t  currentMa = 0;     ///< Current limit, mA.
    int16_t  kp = 0;            ///< Position gain, when supported by the mode.
    int16_t  kd = 0;            ///< Damping gain, when supported by the mode.
    bool     enable = false;    ///< Enable this joint.
};

/**
 * @brief Whole-hand joint target frame.
 *
 * setHandCommand() treats this as a complete frame: joints with enable=false
 * are disabled, even if they were enabled by a previous command.
 */
struct HandCommand
{
    std::array<JointTarget, static_cast<size_t>(JointId::Count)> joints{}; ///< Indexed by JointId.
};

/**
 * @brief Public pusher control modes.
 *
 * Modes that require drive parameters or calibration are intentionally not
 * exposed. More values may be appended in later releases.
 */
enum class PusherMode : int8_t
{
    None = 0x00,   ///< Disable / no mode.
    Csp  = 0x08,   ///< Cyclic synchronous position; SDK interpolates the target.
    Csv  = 0x09,   ///< Cyclic synchronous velocity; SDK smooths the target speed.
};

/**
 * @brief Snapshot of one pusher.
 */
struct PusherState
{
    bool online = false;          ///< Pusher slave is online.
    bool enabled = false;         ///< Drive is operation-enabled.
    bool fault = false;           ///< Drive fault bit.
    bool targetReached = false;   ///< Raw DS402 statusword bit 10; not asserted by every drive.

    uint32_t mapped = 0;          ///< PusherField bitmask of mapped PDO fields.

    PusherMode mode = PusherMode::None;  ///< Public mode mapping of 0x6061.
    int8_t   modeRaw = 0;                ///< Raw 0x6061 value, for diagnostics.
    int32_t  pos = 0;                    ///< Actual position, um.
    int32_t  vel = 0;                    ///< Actual velocity, um/s.
    int16_t  torque = 0;                 ///< Actual torque, 0.1% rated.
    int16_t  current = 0;                ///< Actual current, 0.1% rated.
    int16_t  force = 0;                  ///< Force sensor value, N.
    int32_t  dcLinkVoltage = 0;          ///< DC link voltage, mV.
    int32_t  followingError = 0;         ///< Following error, um.
    uint16_t errorCode = 0;              ///< Drive error code, 0x603F.
    uint32_t alarmBits = 0;              ///< Alarm bitmap, 0x2201.
    std::array<int16_t, 2> driveTemp{ 0, 0 };   ///< Drive temperatures, 0.1 C.
    std::array<int16_t, 2> motorTemp{ 0, 0 };   ///< Motor temperatures, 0.1 C.

    /// @brief True when the given PDO field is mapped.
    [[nodiscard]] bool has(const PusherField field) const noexcept
    {
        return (mapped & pusherFieldBit(field)) != 0;
    }
};

/**
 * @brief Target for one pusher in a public control mode.
 *
 * Only Csp and Csv are accepted by setPusherCommand().
 */
struct PusherCommand
{
    bool       enable = false;      ///< Enable the drive.
    bool       faultReset = false;  ///< One-shot fault reset request.
    PusherMode mode = PusherMode::Csp; ///< Control mode.

    int32_t pos = 0;                ///< Csp target position, um.
    int32_t vel = 0;                ///< Csp max speed / Csv target speed, um/s.
};

/**
 * @brief Common DexHand interface.
 *
 * Lifecycle:
 * @code
 * auto hand = DexHand::createInstance(...);
 * hand->connect(true);
 * ... control / status ...
 * hand->emergencyStop();
 * hand->disconnect();
 * @endcode
 *
 * One instance owns one EtherCAT interface and one hand. The hand exclusively
 * owns the bus. Instances are normally managed through shared_ptr.
 */
class DexHand
{
public:
    using PTR = std::shared_ptr<DexHand>;

public:
    DexHand() = delete;
    virtual ~DexHand();

    /**
     * @brief List all network interfaces.
     * @param adapterType Adapter implementation to use.
     * @return Interface name and description for each adapter.
     */
    DEXHAND_API static std::vector<InterfaceInfo> scanInterfaces(
        AdapterType adapterType = AdapterType::EC_SOEM);

    /**
     * @brief List interfaces that currently have EtherCAT slaves attached.
     * @param adapterType Adapter implementation to use.
     * @return Interfaces with at least one responding slave.
     * @note Prefer this over scanInterfaces() when selecting an interface.
     */
    DEXHAND_API static std::vector<InterfaceInfo> scanDexInterfaces(
        AdapterType adapterType = AdapterType::EC_SOEM);

    /**
     * @brief Create a hand instance.
     * @param productType Product type; currently only DX021_PRO.
     * @param adapterType Adapter implementation.
     * @param ifName Interface name from scanInterfaces/scanDexInterfaces.
     * @return Instance on success; nullptr for an unsupported product type.
     * @note The instance is not connected until connect() succeeds.
     */
    DEXHAND_API static PTR createInstance(ProductType productType, AdapterType adapterType, const std::string ifName);

    /**
     * @brief Establish a full control connection.
     *
     * Opens the interface, configures slaves, maps PDOs, enables DC/PDO, and
     * enters OP. Only V1.1 hand firmware is accepted.
     *
     * @param listen Allow status callbacks when true. PDO, DC and snapshots
     *               run either way; getHandStatus() is unaffected.
     * @return true on success; false on failure.
     * @note Blocks while the bus is configured. Do not call from a callback.
     * @note On failure, read errorCode()/errorMessage().
     */
    DEXHAND_API virtual bool connect(bool listen) = 0;

    /**
     * @brief Close the active connection.
     * @return true on success.
     * @note Clears commands and returns motors to a safe output state.
     */
    DEXHAND_API virtual bool disconnect() = 0;

    /// @brief True while a control session is active.
    DEXHAND_API virtual bool isConnected() const = 0;

    /**
     * @brief Read the connected hand topology.
     * @param out Receives module and pusher information.
     * @return true on success.
     */
    DEXHAND_API virtual bool getHandInfo(HandInfo& out) const = 0;

    /**
     * @brief Register or clear the status callback.
     * @param cb Callback; an empty function clears it.
     * @warning The callback runs on the PDO thread. Do not block or call SDO,
     *          connect, or disconnect from it.
     */
    DEXHAND_API virtual void setStatusRxCallback(const StatusRxDataCallBack& cb) const = 0;

    /**
     * @brief Register or clear the error callback.
     * @param cb Callback; an empty function clears it.
     */
    DEXHAND_API void setErrorRxCallback(const ErrorRxCallBack& cb) const;

    /**
     * @brief Get the latest status snapshot.
     * @param out Receives a shared snapshot.
     * @return true when a snapshot is available.
     */
    DEXHAND_API virtual bool getHandStatus(DX21StatusRxData::PTR& out) const = 0;

    /// @brief True when the last control PDO cycle met expected WKC.
    DEXHAND_API virtual bool isPdoHealthy() const = 0;

    /**
     * @brief Read transport and PDO diagnostics.
     * @param out Receives a diagnostics snapshot.
     * @return true when the adapter exists.
     */
    DEXHAND_API bool getDiagnostics(HandDiagnostics& out) const;

    /**
     * @brief Set status callback rate and enable state.
     * @param rateHz Callback rate, clamped to [100, 1000] Hz.
     * @param enable Enable or disable callbacks; the same flag as listen().
     * @return true on success; false when unsupported.
     */
    virtual bool setRealtimeResponse(uint16_t rateHz, bool enable)
    {
        (void)rateHz;
        (void)enable;
        return false;
    }

    /**
     * @brief Set the hand-wide control mode.
     * @param mode Control mode; both boards use the same mode.
     * @param force Disable all joints before changing mode when true.
     * @return true on success.
     * @note Returns MODE_CONFLICT when joints are enabled and mode differs.
     */
    DEXHAND_API virtual bool setControlMode(handCtrlMode mode, bool force = false) = 0;

    /// @brief Read the current hand-wide control mode.
    DEXHAND_API virtual bool getControlMode(handCtrlMode& mode) const = 0;

    /**
     * @brief Replace the whole-hand command frame.
     * @param cmd Complete target frame; joints with enable=false are disabled.
     * @return true when accepted.
     * @note Requires a control mode. ZERO_TORQUE_MODE rejects enabled joints.
     */
    DEXHAND_API virtual bool setHandCommand(const HandCommand& cmd) = 0;

    /// @brief Read the latest accepted command frame.
    DEXHAND_API virtual bool getHandCommand(HandCommand& out) const = 0;

    /**
     * @brief Update one joint target, leaving other joints unchanged.
     * @param joint Joint to update.
     * @param target New target for that joint.
     * @param coupledTarget Partner target for a coupled joint in MIT mode;
     *                      the partner's own kp/kd come from this struct.
     *                      Required for coupled joints in MIT mode; must be
     *                      null in the other modes.
     * @return true when accepted.
     * @note JOINT_POS/THREE_LOOP hold the coupled partner at its current
     *       angle automatically. MIT_CTRL_MODE requires an explicit partner
     *       target for coupled joints.
     */
    DEXHAND_API virtual bool moveJoint(JointId joint, const JointTarget& target,
                                       const JointTarget* coupledTarget = nullptr) = 0;

    /// @brief Zero torque and disable all joints.
    DEXHAND_API virtual bool emergencyStop() = 0;

    /**
     * @brief Start or stop status callbacks.
     * @param enable true to start callbacks; false to stop them.
     * @note Shares the enable flag with setRealtimeResponse(); the last call
     *       wins and the configured callback rate is kept.
     * @note PDO and snapshots continue while callbacks are stopped.
     */
    DEXHAND_API void listen(bool enable) const;

    /// @brief True when status callbacks are enabled.
    DEXHAND_API bool isListening() const;

    /// @brief True while the adapter is open (control or maintenance session).
    DEXHAND_API bool isAvailable() const;

    /// @brief Product type of this instance.
    DEXHAND_API virtual ProductType productType() const = 0;

    /// @brief Last error code. Call after a failed operation.
    DEXHAND_API SysErrorCode errorCode() const;

    /// @brief Last error message snapshot. Call after a failed operation.
    DEXHAND_API std::string errorMessage() const;

protected:
    /**
     * @brief Construct a hand implementation.
     * @param adapterType Adapter implementation.
     * @param ifName Interface name.
     * @note For derived implementations; use createInstance() or DexHand_Pro.
     */
    DexHand(AdapterType adapterType, const std::string ifName);

    class DexHandImpl* impl = nullptr;   ///< Internal implementation.
};

/**
 * @brief DexHand 021Pro implementation.
 *
 * Provides device-level and pusher features. Create it directly or cast the
 * result of DexHand::createInstance().
 */
class DexHand_Pro : public DexHand
{
public:
    using PTR = std::shared_ptr<DexHand_Pro>;

public:
    DexHand_Pro() = delete;

    /**
     * @brief Construct a 021Pro hand implementation.
     * @param adapterType Adapter implementation.
     * @param ifName Interface name from scanInterfaces/scanDexInterfaces.
     * @note The instance is not connected until connect() succeeds.
     */
    DEXHAND_API DexHand_Pro(AdapterType adapterType, const std::string ifName);
    ~DexHand_Pro() override;

    /// @copydoc DexHand::connect
    DEXHAND_API bool connect(bool listen) override;

    /// @copydoc DexHand::disconnect
    DEXHAND_API bool disconnect() override;

    /// @copydoc DexHand::isConnected
    DEXHAND_API bool isConnected() const override;

    /// @copydoc DexHand::getHandInfo
    DEXHAND_API bool getHandInfo(HandInfo& out) const override;

    /// @copydoc DexHand::setStatusRxCallback
    DEXHAND_API void setStatusRxCallback(const StatusRxDataCallBack& cb) const override;

    /// @copydoc DexHand::getHandStatus
    DEXHAND_API bool getHandStatus(DX21StatusRxData::PTR& out) const override;

    /// @copydoc DexHand::isPdoHealthy
    DEXHAND_API bool isPdoHealthy() const override;

    /// @copydoc DexHand::setRealtimeResponse
    DEXHAND_API bool setRealtimeResponse(uint16_t rateHz, bool enable) override;

    /// @copydoc DexHand::setControlMode
    DEXHAND_API bool setControlMode(handCtrlMode mode, bool force = false) override;

    /// @copydoc DexHand::getControlMode
    DEXHAND_API bool getControlMode(handCtrlMode& mode) const override;

    /// @copydoc DexHand::setHandCommand
    DEXHAND_API bool setHandCommand(const HandCommand& cmd) override;

    /// @copydoc DexHand::getHandCommand
    DEXHAND_API bool getHandCommand(HandCommand& out) const override;

    /// @copydoc DexHand::moveJoint
    DEXHAND_API bool moveJoint(JointId joint, const JointTarget& target,
                               const JointTarget* coupledTarget = nullptr) override;

    /**
     * @brief Zero torque and disable all joints.
     *
     * Sets the hand control mode to ZERO_TORQUE_MODE and clears the command
     * slot. The connection remains open.
     */
    DEXHAND_API bool emergencyStop() override;

    /// @copydoc DexHand::productType
    DEXHAND_API ProductType productType() const override;

    /**
     * @brief Read a CTRL device firmware version.
     * @param part Device to query.
     * @param out Receives the parsed version.
     * @return true on success.
     */
    DEXHAND_API bool getCtrlFirmwareVersion(FingerPart part, FirmwareVersion& out);

    /**
     * @brief Read an EC board firmware version.
     * @param module Board to query.
     * @param out Receives the parsed version.
     * @return true on success.
     */
    DEXHAND_API bool getEcFirmwareVersion(HandModule module, FirmwareVersion& out);

    /**
     * @brief Read current EC board temperature.
     * @param module Board to query.
     * @param centiCelsius Receives temperature in 0.01 Celsius.
     * @return true on success.
     * @note One sensor per module.
     */
    DEXHAND_API bool getCurrentTemperature(HandModule module, int16_t& centiCelsius);

    /**
     * @brief Read the protection temperature threshold.
     * @param part Device to query.
     * @param celsius Receives the threshold in Celsius.
     * @return true on success.
     */
    DEXHAND_API bool getSafeTemperature(FingerPart part, uint8_t& celsius);

    /**
     * @brief Write the protection temperature threshold.
     * @param part Device to configure.
     * @param celsius Threshold in Celsius; values below 50 are rejected.
     * @return true on success.
     */
    DEXHAND_API bool setSafeTemperature(FingerPart part, uint8_t celsius);

    /**
     * @brief Read the latest cached tactile data for one region.
     * @param part Region to query.
     * @param out Receives the cached values and valid count.
     * @return true when the region has been received since connect.
     * @note Tactile frames rotate through the PDO; the SDK caches the latest
     *       value per region. The cache is cleared on reconnect.
     */
    DEXHAND_API bool getTactile(TactilePart part, TactileData& out) const;

    /**
     * @brief Set all joint targets to zero using the current control mode.
     * @return true on success.
     * @note Uses the current speed/current/kp/kd settings.
     */
    DEXHAND_API bool resetJoints();

    /**
     * @brief Read all EC board information.
     * @param out Receives one entry per EC board.
     * @return true on success.
     */
    DEXHAND_API bool readEcInfo(std::vector<EcInfo>& out);

    /**
     * @brief Read one CTRL device information block.
     * @param part Device to query.
     * @param out Receives device information.
     * @return true on success.
     */
    DEXHAND_API bool readDeviceInfo(FingerPart part, DeviceInfo& out);

    /**
     * @brief Restart one CTRL device.
     * @param part Device to restart.
     * @return true on success.
     */
    DEXHAND_API bool restartDevice(FingerPart part);

    /**
     * @brief Set fill-light brightness on the four-finger board EC device.
     * @param brightness 0=off, 255=maximum.
     * @return true on success; false when the four-finger board is absent.
     */
    DEXHAND_API bool setFillLight(uint8_t brightness);

    /**
     * @brief Disable and re-enable all CTRL devices.
     * @return true when all devices were re-enabled.
     */
    DEXHAND_API bool restartAllCtrl();

    /**
     * @brief Request all EC boards to restart.
     *
     * The boards reset immediately, so no SDO result is available. The caller
     * must reconnect after the reset.
     */
    DEXHAND_API void requestRestartAllEc();

    /// @brief Number of pushers found on the bus.
    DEXHAND_API uint8_t pusherCount() const;

    /**
     * @brief Read a pusher status snapshot.
     * @param pusher Pusher index.
     * @param out Receives the snapshot.
     * @return true on success.
     * @note The snapshot is updated by the PDO thread. Poll until
     *       PusherState::online is true before using pos or issuing commands.
     */
    DEXHAND_API bool getPusherStatus(PusherId pusher, PusherState& out) const;

    /**
     * @brief Set a pusher command in a public control mode.
     * @param pusher Pusher index.
     * @param cmd Command; only None/Csp/Csv are accepted.
     * @return true when accepted.
     * @note Changing mode resets the enable sequence; the next command must
     *       enable the drive again.
     * @note faultReset is consumed as a one-shot request.
     */
    DEXHAND_API bool setPusherCommand(PusherId pusher, const PusherCommand& cmd);

    /**
     * @brief Read the last accepted public pusher command.
     * @param pusher Pusher index.
     * @param out Receives the command.
     * @return false when no command exists or the drive is in a mode that is
     *         not part of the public API. Use getPusherStatus() for modeRaw.
     */
    DEXHAND_API bool getPusherCommand(PusherId pusher, PusherCommand& out) const;

    /**
     * @brief Move a pusher with the CSP convenience API.
     * @param pusher Pusher index.
     * @param posUm Target position, um.
     * @param speedUmPerS Maximum speed, um/s; 0 uses the SDK default (30000).
     * @return true when accepted.
     * @note Uses CSP mode with the SDK's fixed internal acceleration.
     */
    DEXHAND_API bool movePusher(PusherId pusher, int32_t posUm, uint32_t speedUmPerS = 0);

    /**
     * @brief Request a one-shot pusher fault reset.
     * @param pusher Pusher index.
     * @return true when the request was accepted.
     * @note The drive is disabled during reset; re-enable it afterwards.
     */
    DEXHAND_API bool resetPusherFault(PusherId pusher);

private:
    class DexHand021ProImpl* pimpl = nullptr;   ///< 021Pro-specific implementation.
};

}
}
