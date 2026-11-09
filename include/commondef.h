#pragma once
//
// commondef.h - base definitions (aligned with DexLibCommon/interfaces/commondef.h)
//

#include <cstdint>
#include <string>

#if defined(_WIN32)
#define EXPORT_API_DX21 __declspec(dllexport)
#define IMPORT_API_DX21 __declspec(dllimport)

#ifdef BUILD_DLL   // building the SDK -> export symbols masked by DEXHAND_API
#define DEXHAND_API    EXPORT_API_DX21
#else              // using the SDK  -> import symbols masked by DEXHAND_API
#define DEXHAND_API    IMPORT_API_DX21
#endif

#else              // Linux: no explicit export/import attribute
#define DEXHAND_API
#endif

namespace DexRobot
{
enum class handCtrlMode: uint8_t
{
    ZERO_TORQUE_MODE = 0x00,

    JOINT_POS_MODE = 0x04,
    THREE_LOOP_MODE = 0x06,
    MIT_CTRL_MODE = 0x07,
};

// Log levels. Order must match DexLibCommon.
enum class LOG_LEVEL
{
    DX_CLOSE = 0,
    DX_FATAL,
    DX_ERROR,
    DX_WARN,
    DX_INFO,
    DX_DEBUG,
};

// Stable error codes. Append only; never reorder.
enum class SysErrorCode
{
    NONE = 0,
    ADAPTER_NA,
    DEV_TYPE_MISMATCH,
    CONN_LOST,
    CONN_TIMEOUT,
    BOOT_FAILURE,

    // EC-specific codes
    PERMISSION_DENIED,
    DEPENDENCY_MISSING,
    INTERFACE_NOT_FOUND,
    NO_SLAVE_FOUND,
    PDO_LAYOUT_MISMATCH,
    PDO_UNHEALTHY,
    STATE_TRANSITION_FAILED,
    SDO_ABORT,
    SII_INVALID,
    UPGRADE_FAILED,
    MAINTENANCE_BUSY,
    INTERNAL,
    FIRMWARE_UPGRADE_REQUIRED,
    MODE_CONFLICT,
    MODE_NOT_SET,
};

// Failure attribution.
enum class FaultDomain : uint8_t
{
    None = 0,
    Environment,
    Hardware,
    Configuration,
    Device,
    Sdk,
};

// EC connection stages.
enum class ConnectStage : uint8_t
{
    Idle = 0,
    Init,
    ConfigInit,
    ConfigMap,
    Identify,
    SafeOp,
    Op,
    Verify,
    Ready,
};

namespace Dex021
{

enum class ProductType : int
{
    UNSUPPORTED = -1,
    DX021_PRO = 0,
};

enum class AdapterType : int
{
    UNKNOWN = -1,
    EC_SOEM = 1,
};

// PDO layout version. The extent (byte sizes) is the dispatch key.
enum class PdoVersion : uint8_t
{
    UNKNOWN = 0,
    V1_1 = 2,
};

// Pusher PDO fields. Bit order defines PusherState::mapped.
enum class PusherField : uint32_t
{
    Statusword  = 0,    // 0x6041
    ModeDisplay = 1,    // 0x6061
    Position    = 2,    // 0x6064
    Velocity    = 3,    // 0x606C
    Torque      = 4,    // 0x6077
    Current     = 5,    // 0x6078
    Force       = 6,    // 0x300A
    DcLinkVolt  = 7,    // 0x6079
    FollowErr   = 8,    // 0x60F4
    ErrorCode   = 9,    // 0x603F
    AlarmBits   = 10,   // 0x2201
    DriveTemp1  = 11,   // 0x3004
    DriveTemp2  = 12,   // 0x3005
    MotorTemp1  = 13,   // 0x3006
    MotorTemp2  = 14,   // 0x3007
};

[[nodiscard]] inline constexpr uint32_t pusherFieldBit(const PusherField field) noexcept
{
    return 1u << static_cast<uint32_t>(field);
}

}   // namespace Dex021

// Structured error. Text is for humans; code/domain/stage are for program logic.
struct ErrorInfo
{
    SysErrorCode code   = SysErrorCode::NONE;
    FaultDomain  domain = FaultDomain::None;
    ConnectStage stage  = ConnectStage::Idle;

    std::string message;    // short, actionable
    std::string detail;     // slave, expected/actual, WKC, AL status, ...
    std::string remedy;     // suggested action
    int         nativeCode = 0;   // pcap/errno/AL status if available

    [[nodiscard]] bool ok() const { return code == SysErrorCode::NONE; }
    explicit operator bool() const { return ok(); }
};

// Human-readable name for logs. Not for program logic.
inline const char* toString(const SysErrorCode code)
{
    switch (code)
    {
        case SysErrorCode::NONE:                     return "NONE";
        case SysErrorCode::ADAPTER_NA:               return "ADAPTER_NA";
        case SysErrorCode::DEV_TYPE_MISMATCH:        return "DEV_TYPE_MISMATCH";
        case SysErrorCode::CONN_LOST:                return "CONN_LOST";
        case SysErrorCode::CONN_TIMEOUT:             return "CONN_TIMEOUT";
        case SysErrorCode::BOOT_FAILURE:             return "BOOT_FAILURE";
        case SysErrorCode::PERMISSION_DENIED:        return "PERMISSION_DENIED";
        case SysErrorCode::DEPENDENCY_MISSING:       return "DEPENDENCY_MISSING";
        case SysErrorCode::INTERFACE_NOT_FOUND:      return "INTERFACE_NOT_FOUND";
        case SysErrorCode::NO_SLAVE_FOUND:           return "NO_SLAVE_FOUND";
        case SysErrorCode::PDO_LAYOUT_MISMATCH:      return "PDO_LAYOUT_MISMATCH";
        case SysErrorCode::PDO_UNHEALTHY:            return "PDO_UNHEALTHY";
        case SysErrorCode::STATE_TRANSITION_FAILED:  return "STATE_TRANSITION_FAILED";
        case SysErrorCode::SDO_ABORT:                return "SDO_ABORT";
        case SysErrorCode::SII_INVALID:              return "SII_INVALID";
        case SysErrorCode::UPGRADE_FAILED:           return "UPGRADE_FAILED";
        case SysErrorCode::MAINTENANCE_BUSY:         return "MAINTENANCE_BUSY";
        case SysErrorCode::INTERNAL:                 return "INTERNAL";
        case SysErrorCode::FIRMWARE_UPGRADE_REQUIRED: return "FIRMWARE_UPGRADE_REQUIRED";
        case SysErrorCode::MODE_CONFLICT:            return "MODE_CONFLICT";
        case SysErrorCode::MODE_NOT_SET:             return "MODE_NOT_SET";
    }

    return "UNKNOWN";
}

}   // namespace DexRobot
