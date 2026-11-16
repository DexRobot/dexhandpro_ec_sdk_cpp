# DexHand021Pro EtherCAT SDK 用户接口文档

版本 1.0.2    2026-11-13

| **项目** | **内容**                  |
| -------- | ------------------------- |
| 文档版本 | 1.0.2                     |
| 对应 SDK | `sdkVersion() == "1.0.2"` |
| 适用产品 | DexHand021Pro             |
| 通信方式 | EtherCAT（SOEM）          |
| 适用固件 | 手板 EC 应用 V1.1         |
| 更新日期 | 2026-11-13                |

## 1. 文档说明

本文档面向使用 DexHand021Pro SDK 进行 C++ 二次开发的用户，说明 `DexHand` 和 `DexHand_Pro` 两个公开类的接口、参数、返回值和使用约束。

所有接口位于命名空间：

```cpp
namespace DexRobot::Dex021;
```

公开头文件：

```cpp
#include "DexHand.h"
```

SDK 版本通过以下接口获取：

```cpp
const char* sdkVersion();
```

文档中的接口名称、类型名称和枚举值均与头文件保持一致。

## 2. 产品与总线拓扑

一只 DexHand021Pro 通过一条 EtherCAT 总线与上位机连接。手对整个总线是独占的：一个 SDK 实例对应一个网口、一只手。

一只手由以下从站组成：

| **从站** | **角色**             |
| -------- | -------------------- |
| 推杆 0   | SEENPIN 推杆         |
| 推杆 1   | SEENPIN 推杆         |
| 手板 0   | 拇指板（Thumb）      |
| 手板 1   | 四指板（FourFinger） |

SDK 对外提供 16 个语义关节：

- 拇指 4 个关节
- 食指、中指、无名指、小指各 3 个关节

## 3. 运行环境与集成

### 3.1 环境要求

| **项目** | **要求**                                                                    |
| -------- | --------------------------------------------------------------------------- |
| 操作系统 | Windows 10/11 x64；Ubuntu 22.04 / 24.04 x86_64                              |
| 编译器   | Windows：MSVC v143；Linux：GCC 11+（或使用兼容 libstdc++ 的 Clang）         |
| C++ 标准 | C++20                                                                       |
| 网络依赖 | Windows：Npcap（需单独安装）；Linux：无额外依赖（raw socket）               |
| 运行库   | Windows：Microsoft Visual C++ 2015–2022 x64 Redistributable；Linux：libstdc++ |

**Windows 运行前请确认：**

- 已从 Npcap 官方渠道安装 Npcap，并勾选 **WinPcap API 兼容模式**。Npcap 不随本 SDK 分发。
- 已安装 **Microsoft Visual C++ 2015–2022 x64 Redistributable**。
- Npcap 默认仅允许管理员访问网卡，因此运行本 SDK 的程序通常需要以管理员权限启动。

**Ubuntu 运行前请确认：**

- 进程需要 **root** 或 `CAP_NET_RAW` 才能打开 EtherCAT 网口；
- EtherCAT 网口应独占使用，不要配置 IP，并避免 NetworkManager 接管：

- 可以用 `setcap` 给程序单独授权，避免每次使用 sudo：
```bash
sudo setcap cap_net_raw,cap_sys_nice+ep ./build/bin/Release/hand_status_demo
```

- 非 root 且未授权时，`scanDexInterfaces()` / `scan_dex_interfaces()` 会返回空列表，而不是抛出异常。

### 3.2 仅提供 Release 版本

SDK 只提供 Release 版二进制，不提供 Debug 版：

- Windows：`dexhandpro.dll` / `dexhandpro.lib`，不提供 PDB；
- Linux：`libdexhandpro.so`。

**Windows / MSVC：**

SDK 的公开接口包含 C++ 标准库类型。MSVC 的 Debug 配置使用调试版 CRT 与 STL（`/MDd`，`_ITERATOR_DEBUG_LEVEL=2`），与 Release DLL 的 ABI 不兼容。因此：

- 客户工程必须使用 **Release** 或 **RelWithDebInfo** 构建，二者都使用发布版 CRT（`/MD`）；
- 通过本 SDK 的 CMake 包接入时，使用 **Debug** 配置编译会直接在 `DexHand.h` 报错并提示改用 Release/RelWithDebInfo；手动配置 include/lib 的工程请自行确保使用 `/MD`；

**Linux / GCC：**

- 客户工程建议使用 Release 或 RelWithDebInfo；
- SDK 使用 GCC 11.4 / libstdc++（`GLIBCXX_3.4.29`）构建，编译器版本不应低于 GCC 11；
- 不要在客户工程中定义 `_GLIBCXX_DEBUG`，libstdc++ 调试模式会改变 STL 布局，与 SDK 不兼容。

### 3.3 初始化顺序

典型调用顺序：

```cpp
scanInterfaces / scanDexInterfaces
  → createInstance 或构造 DexHand_Pro
  → connect
  → setControlMode
  → setHandCommand / moveJoint (手控制)， setPusherCommand / movePusher (推杆控制)
  → getHandStatus / setStatusRxCallback (获取手状态数据) ， getPusherStatus （获取推杆状态数据）
  → emergencyStop (运动中的急停)
  → disconnect
```

### 3.4 SDK 包结构与 CMake 接入

SDK 根目录结构：

```text
dexhandpro_ec_sdk_cpp/
├── CMakeLists.txt
├── include/
│   ├── DexHand.h
│   ├── commondef.h
│   └── DateTimeUtils.h
├── libs/
│   ├── windows/
│   │   ├── dexhandpro.dll
│   │   └── dexhandpro.lib
│   ├── linux/
│   │   └── libdexhandpro.so
│   └── cmake/
│       └── DexHandPro/
│           └── DexHandProConfig.cmake
└── examples/
    ├── CMakeLists.txt
    ├── hand_status_demo.cpp
    ├── hand_move_demo.cpp
    └── hand_sdo_demo.cpp
```

CMake 文件职责：

| **文件**                                       | **作用**                                                                                                                                          |
| ---------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------- |
| `CMakeLists.txt`                               | SDK 包 CMake 入口：检查编译环境（Windows 要求 MSVC v143）、按平台创建 `DexHandPro::dexhandpro` 导入目标、加入 examples、提供 `dexhandpro_deploy()`；不编译 SDK 本体 |
| `examples/CMakeLists.txt`                      | 编译并链接 SDK 示例程序。从 SDK 根目录构建时复用已有导入目标；单独配置 examples 时通过 `find_package` 加载 SDK，并在构建后复制对应平台的 Release 运行库 |
| `libs/cmake/DexHandPro/DexHandProConfig.cmake` | 外部工程 `find_package(DexHandPro CONFIG REQUIRED)` 的加载入口：定位头文件与当前平台 Release 二进制，定义导入目标及 `dexhandpro_deploy()`        |

包内示例构建：

```bash
# Linux
cmake -S . -B build -G Ninja
cmake --build build -j
sudo ./build/bin/Release/hand_status_demo
```

```powershell
# Windows
cmake -S . -B build
cmake --build build --config Release
.\build\bin\Release\hand_status_demo.exe
```

用户工程通过 CMake package 接入：

```cmake
find_package(DexHandPro CONFIG REQUIRED)
target_link_libraries(app PRIVATE DexHandPro::dexhandpro)
dexhandpro_deploy(app)
```

## 4. 实例创建与连接

### 4.1 DexHand::scanInterfaces

```cpp
static std::vector<InterfaceInfo> scanInterfaces(
    AdapterType adapterType = AdapterType::EC_SOEM);
```

功能：列出当前系统中可用于 EtherCAT 的网络接口。

参数：

| **参数**      | **类型**      | **说明**                                    |
| ------------- | ------------- | ------------------------------------------- |
| `adapterType` | `AdapterType` | 适配器类型，当前使用 `AdapterType::EC_SOEM` |

返回：`InterfaceInfo` 列表。

`InterfaceInfo` 字段：

| **字段**      | **类型**      | **说明**               |
| ------------- | ------------- | ---------------------- |
| `name`        | `std::string` | 接口名称，用于创建实例 |
| `description` | `std::string` | 物理网卡描述           |

### 4.2 DexHand::scanDexInterfaces

```cpp
static std::vector<InterfaceInfo> scanDexInterfaces(
    AdapterType adapterType = AdapterType::EC_SOEM);
```

功能：只返回当前已检测到021Pro 灵巧手从站的网络接口。

返回值与 `scanInterfaces` 相同。选择网卡时优先使用该接口。

### 4.3 DexHand::createInstance

```cpp
static PTR createInstance(ProductType productType,
    AdapterType adapterType, const std::string ifName);
```

功能：创建一只手的实例。

参数：

| **参数**      | **类型**            | **说明**                                       |
| ------------- | ------------------- | ---------------------------------------------- |
| `productType` | `ProductType`       | 产品类型，021Pro 使用 `ProductType::DX021_PRO` |
| `adapterType` | `AdapterType`       | 适配器类型，当前使用 `AdapterType::EC_SOEM`    |
| `ifName`      | `const std::string` | 网口名称，来自扫描结果                         |

返回：`DexHand::PTR`。产品类型不支持时返回空指针。

实例创建后尚未连接，需要继续调用 `connect()`。

### 4.4 DexHand_Pro 直接构造

```cpp
DexHand_Pro(AdapterType adapterType, const std::string ifName);
```

功能：直接构造 021Pro 实例。

参数与 `createInstance` 的 `adapterType`、`ifName` 相同。

返回值：构造成功即得到 `DexHand_Pro` 实例，仍需调用 `connect()`。

### 4.5 connect

```cpp
virtual bool connect(bool listen) = 0;
```

功能：建立完整控制连接。

内部流程包括：

- 打开 EtherCAT 网口
- 扫描从站
- 配置从站
- 映射 PDO
- 启用 DC 与 PDO 线程
- 进入 OP

参数：

| **参数** | **类型** | **说明**                                                                          |
| -------- | -------- | --------------------------------------------------------------------------------- |
| `listen` | `bool`   | true 时允许触发状态回调；false 时不触发。PDO、DC 与状态快照在两种情况下都正常运行 |

返回：true 成功；false 失败。

失败时通过 `errorCode()` 和 `errorMessage()` 获取错误信息。

注意：

- `connect()` 是阻塞操作，总线配置期间可能持续数秒
- `listen` 只控制用户状态回调，不影响 PDO 线程和 `getHandStatus()` 快照
- 回调还受 `setRealtimeResponse(rate, enable)` 的 `enable` 控制（默认开启）
- 不要在状态回调或错误回调中调用 `connect()`
- 控制连接只接受 V1.1 手板 PDO 布局

### 4.6 disconnect

```cpp
virtual bool disconnect() = 0;
```

功能：关闭当前连接。

行为：

- 清空命令槽
- 停止 PDO 线程
- 将输出恢复到安全状态
- 将总线恢复到 INIT
- 关闭网口

返回：true 成功。

### 4.7 isConnected

```cpp
virtual bool isConnected() const = 0;
```

返回：true 表示当前处于控制连接状态。

### 4.8 isAvailable

```cpp
bool isAvailable() const;
```

返回：true 表示适配器已打开。

### 4.9 productType

```cpp
virtual ProductType productType() const = 0;
```

返回：当前实例的产品类型。021Pro 返回 `ProductType::DX021_PRO`。

### 4.10 sdkVersion

```cpp
const char* sdkVersion();
```

返回：SDK 版本字符串，例如 `"1.0.2"`。

### 4.11 getHandInfo

```cpp
virtual bool getHandInfo(HandInfo& out) const = 0;
```

功能：读取当前连接手的拓扑信息。

参数：

| **参数** | **类型**    | **说明**     |
| -------- | ----------- | ------------ |
| `out`    | `HandInfo&` | 输出手部拓扑 |

返回：true 成功。

`HandInfo` 字段：

| **字段**      | **类型**        | **说明**                              |
| ------------- | --------------- | ------------------------------------- |
| `side`        | `HandSide`      | 左手 / 右手 / 未知                    |
| `moduleCount` | `uint8_t`       | 检测到的手板数量                      |
| `pusherCount` | `uint8_t`       | 检测到的推杆数量                      |
| `modules`     | `ModuleInfo[2]` | 手板信息，有效范围 `[0, moduleCount)` |
| `pushers`     | `PusherInfo[2]` | 推杆信息，有效范围 `[0, pusherCount)` |

`ModuleInfo` 字段：

| **字段**  | **类型**     | **说明**      |
| --------- | ------------ | ------------- |
| `module`  | `HandModule` | 手板角色      |
| `version` | `PdoVersion` | PDO 版本      |
| `op`      | `bool`       | 从站是否在 OP |

`PusherInfo` 字段：

| **字段**         | **类型**   | **说明**           |
| ---------------- | ---------- | ------------------ |
| `id`             | `PusherId` | 推杆索引           |
| `op`             | `bool`     | 从站是否在 OP      |
| `hasForceSensor` | `bool`     | 是否映射了力传感器 |

## 5. 状态回调与状态快照

### 5.1 状态回调类型

```cpp
using StatusRxDataCallBack =
    std::function<void(const char* ifName, const DX21StatusRxData* status)>;
```

参数：

| **参数** | **说明**                           |
| -------- | ---------------------------------- |
| `ifName` | 网口名称，在手的生命周期内有效     |
| `status` | 状态快照指针，仅在回调执行期间有效 |

### 5.2 setStatusRxCallback

```cpp
virtual void setStatusRxCallback(
    const StatusRxDataCallBack& cb) const = 0;
```

功能：注册或清除状态回调。

参数：

| **参数** | **类型**               | **说明**                           |
| -------- | ---------------------- | ---------------------------------- |
| `cb`     | `StatusRxDataCallBack` | 回调函数；传入空函数对象可清除回调 |

注意：

- 回调运行在 PDO 线程
- 回调中禁止阻塞、禁止调用 SDO、禁止调用 `connect()` / `disconnect()`
- 如果需要保存快照，请在回调内复制数据，请勿在回调内做耗时操作。

### 5.3 setErrorRxCallback

```cpp
void setErrorRxCallback(const ErrorRxCallBack& cb) const;
```

功能：注册或清除错误回调。

错误回调类型：

```cpp
using ErrorRxCallBack =
    std::function<void(const ErrorInfo& error)>;
```

参数：

| **参数** | **类型**          | **说明**                       |
| -------- | ----------------- | ------------------------------ |
| `cb`     | `ErrorRxCallBack` | 错误回调；传入空函数对象可清除 |

`ErrorInfo` 字段：

| **字段**     | **类型**       | **说明**     |
| ------------ | -------------- | ------------ |
| `code`       | `SysErrorCode` | 错误码       |
| `domain`     | `FaultDomain`  | 故障归因     |
| `stage`      | `ConnectStage` | 发生阶段     |
| `message`    | `std::string`  | 简短错误描述 |
| `detail`     | `std::string`  | 详细诊断信息 |
| `remedy`     | `std::string`  | 建议处理方式 |
| `nativeCode` | `int`          | 原生错误码   |

### 5.4 getHandStatus

```cpp
virtual bool getHandStatus(DX21StatusRxData::PTR& out) const = 0;
```

功能：读取最近一次状态快照。

参数：

| **参数** | **类型**                 | **说明** |
| -------- | ------------------------ | -------- |
| `out`    | `DX21StatusRxData::PTR&` | 输出快照 |

返回：true 表示当前有可用快照。

### 5.5 setRealtimeResponse

```cpp
virtual bool setRealtimeResponse(uint16_t rateHz, bool enable);
```

功能：设置状态回调频率和开关。

参数：

| **参数** | **类型**   | **说明**                                      |
| -------- | ---------- | --------------------------------------------- |
| `rateHz` | `uint16_t` | 回调频率，单位 Hz；SDK 内部限制在 100–1000 Hz |
| `enable` | `bool`     | true 开启回调；false 关闭回调                 |

返回：真机实现返回 true。

### 5.6 listen / isListening

```cpp
void listen(bool enable) const;
bool isListening() const;
```

功能：

- `listen(true)`：开启状态回调
- `listen(false)`：关闭状态回调
- `isListening()`：查询当前回调开关

关闭回调不会停止 PDO 线程，也不会停止状态快照更新。

### 5.7 DX21StatusRxData 状态快照

```cpp
class DX21StatusRxData
{
public:
    using PTR = std::shared_ptr<DX21StatusRxData>;
 
    virtual HandSide side() const = 0;
    virtual int64_t receiveTimestamp() const = 0;
 
    virtual bool       hasJoint(JointId joint) const = 0;
    virtual JointState joint(JointId joint) const = 0;
};
```

接口说明：

| **接口**             | **返回**     | **说明**                              |
| -------------------- | ------------ | ------------------------------------- |
| `side()`             | `HandSide`   | 左手 / 右手 / 未知                    |
| `receiveTimestamp()` | `int64_t`    | 主站收到该帧的时间，steady_clock 微秒 |
| `hasJoint(joint)`    | `bool`       | 关节索引是否有效                      |
| `joint(joint)`       | `JointState` | 指定关节的状态；无效索引返回默认值    |

`JointState` 字段：

| **字段**             | **类型**  | **说明**                    |
| -------------------- | --------- | --------------------------- |
| `online`             | `bool`    | 关节在线                    |
| `overCurrent`        | `bool`    | 过流                        |
| `overTemp`           | `bool`    | 过温                        |
| `hallFault`          | `bool`    | Hall 故障                   |
| `stall`              | `bool`    | 堵转                        |
| `angleError`         | `bool`    | 角度错误                    |
| `angle`              | `int16_t` | 语义关节角度，0.01°         |
| `velocity`           | `int16_t` | 电机转速，rpm               |
| `current`            | `int16_t` | 电机电流，mA                |
| `hall`               | `int16_t` | Hall 值                     |
| `anglePrimary`       | `int16_t` | Ang0，0.01°                 |
| `angleFollower`      | `int16_t` | Ang1，0.01°；仅远端关节有效 |
| `hasFollower`        | `bool`    | 是否具有第二角度传感器      |
| `angleErrorPrimary`  | `bool`    | Ang0 角度错误               |
| `angleErrorFollower` | `bool`    | Ang1 角度错误               |

## 6. 控制模式

### 6.1 模式枚举

```cpp
enum class handCtrlMode : uint8_t
{
    ZERO_TORQUE_MODE = 0x00,
    JOINT_POS_MODE   = 0x04,
    THREE_LOOP_MODE  = 0x06,
    MIT_CTRL_MODE    = 0x07,
};
```

| **模式**           | **值** | **说明**                   |
| ------------------ | ------ | -------------------------- |
| `ZERO_TORQUE_MODE` | 0x00   | 零力矩模式；不允许使能关节 |
| `JOINT_POS_MODE`   | 0x04   | 关节位置模式               |
| `THREE_LOOP_MODE`  | 0x06   | 三环模式                   |
| `MIT_CTRL_MODE`    | 0x07   | MIT 控制模式               |

控制模式是整手级：两块手板使用同一个模式。

### 6.2 setControlMode

```cpp
virtual bool setControlMode(handCtrlMode mode, bool force = false) = 0;
```

功能：设置整手控制模式。

参数：

| **参数** | **类型**       | **说明**                          |
| -------- | -------------- | --------------------------------- |
| `mode`   | `handCtrlMode` | 目标模式                          |
| `force`  | `bool`         | true 时先失能所有关节，再切换模式 |

返回：true 成功；false 失败。

错误行为：

- 有关节使能且模式不同、`force=false` 时返回 `MODE_CONFLICT`
- 模式切换失败时通过 `errorCode()` / `errorMessage()` 获取错误

### 6.3 getControlMode

```cpp
virtual bool getControlMode(handCtrlMode& mode) const = 0;
```

功能：读取当前整手控制模式。

## 7. 关节控制

### 7.1 关节枚举

```cpp
enum class JointId : uint8_t
{
    ThumbDist = 0,
    ThumbMidd,
    ThumbProxSwng,
    ThumbProxFlex,
 
    IndexProxSwng,
    IndexProxFlex,
    IndexDistCoupled,
 
    MiddleProxSwng,
    MiddleProxFlex,
    MiddleDistCoupled,
 
    RingProxSwng,
    RingProxFlex,
    RingDistCoupled,
 
    LittleProxSwng,
    LittleProxFlex,
    LittleDistCoupled,
 
    Count
};
```

| **关节**            | **说明**       |
| ------------------- | -------------- |
| `ThumbDist`         | 拇指远端       |
| `ThumbMidd`         | 拇指中端       |
| `ThumbProxSwng`     | 拇指近端侧摆   |
| `ThumbProxFlex`     | 拇指近端弯曲   |
| `IndexProxSwng`     | 食指近端侧摆   |
| `IndexProxFlex`     | 食指近端弯曲   |
| `IndexDistCoupled`  | 食指耦合远端   |
| `MiddleProxSwng`    | 中指近端侧摆   |
| `MiddleProxFlex`    | 中指近端弯曲   |
| `MiddleDistCoupled` | 中指耦合远端   |
| `RingProxSwng`      | 无名指近端侧摆 |
| `RingProxFlex`      | 无名指近端弯曲 |
| `RingDistCoupled`   | 无名指耦合远端 |
| `LittleProxSwng`    | 小指近端侧摆   |
| `LittleProxFlex`    | 小指近端弯曲   |
| `LittleDistCoupled` | 小指耦合远端   |

耦合关节必须同时使能，SDK 会自动处理耦合关节。

### 7.2 JointTarget

```cpp
struct JointTarget
{
    float    targetDeg = 0.0f;
    uint16_t speedRpm = 0;
    uint8_t  currentMa = 0;
    int16_t  kp = 0;
    int16_t  kd = 0;
    bool     enable = false;
};
```

| **字段**    | **类型**   | **说明**             |
| ----------- | ---------- | -------------------- |
| `targetDeg` | `float`    | 目标角度，单位度     |
| `speedRpm`  | `uint16_t` | 目标速度，rpm        |
| `currentMa` | `uint8_t`  | 电流限制，mA         |
| `kp`        | `int16_t`  | 位置增益，按模式使用 |
| `kd`        | `int16_t`  | 阻尼增益，按模式使用 |
| `enable`    | `bool`     | 是否使能该关节       |

### 7.3 HandCommand

```cpp
struct HandCommand
{
    std::array<JointTarget, static_cast<size_t>(JointId::Count)> joints{};
};
```

| **字段** | **类型**                      | **说明**                    |
| -------- | ----------------------------- | --------------------------- |
| `joints` | `std::array<JointTarget, 16>` | 按 `JointId` 索引的关节目标 |

`setHandCommand()` 按整帧处理：没有使能的关节会被失能。

### 7.4 setHandCommand

```cpp
virtual bool setHandCommand(const HandCommand& cmd) = 0;
```

功能：替换整手命令帧。

参数：

| **参数** | **类型**             | **说明**     |
| -------- | -------------------- | ------------ |
| `cmd`    | `const HandCommand&` | 完整手部目标 |

返回：true 表示命令已接受。

前置条件：

- 已建立控制连接
- 已设置非零力矩的控制模式

### 7.5 getHandCommand

```cpp
virtual bool getHandCommand(HandCommand& out) const = 0;
```

功能：读取最近一次接受的整手命令。

### 7.6 moveJoint

```cpp
virtual bool moveJoint(JointId joint, const JointTarget& target,
                       const JointTarget* coupledTarget = nullptr) = 0;
```

功能：只更新一个关节，其它关节命令保持不变。

参数：

| **参数**        | **类型**             | **说明**                                                |
| --------------- | -------------------- | ------------------------------------------------------- |
| `joint`         | `JointId`            | 目标关节                                                |
| `target`        | `const JointTarget&` | 该关节的新目标                                          |
| `coupledTarget` | `const JointTarget*` | 伙伴关节目标，默认 `nullptr`；仅 MIT 模式的耦合关节需要 |

返回：true 表示命令已接受。

说明：

- `JOINT_POS` / `THREE_LOOP`：`coupledTarget` 必须传 `nullptr`；耦合关节的伙伴由 SDK 自动保持当前角度，并沿用主关节的速度/电流/kp/kd。传入非空 `coupledTarget` 会返回失败。
- `MIT_CTRL_MODE`：耦合关节必须给出 `coupledTarget`，否则返回失败；非耦合关节传 `nullptr`。
- `MIT` 模式下，主关节和伙伴关节都会被强制使能；伙伴关节的 `kp/kd` 来自 `coupledTarget`，不会被主关节覆盖。

耦合关节对：

| **关节**         | **耦合关节**     |
| ---------------- | ---------------- |
| `ThumbProxSwng`  | `ThumbProxFlex`  |
| `IndexProxSwng`  | `IndexProxFlex`  |
| `MiddleProxSwng` | `MiddleProxFlex` |
| `RingProxSwng`   | `RingProxFlex`   |
| `LittleProxSwng` | `LittleProxFlex` |

### 7.7 resetJoints

```cpp
bool resetJoints();
```

功能：使用当前控制模式，把所有关节目标设为 0 并使能。

行为：

- 保留当前速度、电流、kp、kd 设置
- 零力矩模式下返回 `MODE_NOT_SET`

返回：true 成功。

### 7.8 emergencyStop

```cpp
virtual bool emergencyStop() = 0;
```

功能：切换到零力矩模式，并清空命令槽。

行为：

- 所有关节失能
- 不关闭连接
- 连接保持可用

返回：true 成功。

## 8. 推杆控制

### 8.1 推杆索引

```cpp
enum class PusherId : uint8_t
{
    Pusher0 = 0,
    Pusher1 = 1,
    Count   = 2,
};
```

无论左右手，近大拇指侧的推杆为Pusher0，远离大拇指侧的推杆为Pusher1。

### 8.2 推杆状态

```cpp
struct PusherState
{
    bool online = false;
    bool enabled = false;
    bool fault = false;
    bool targetReached = false;
 
    uint32_t mapped = 0;
 
    PusherMode mode = PusherMode::None;
    int8_t   modeRaw = 0;
    int32_t  pos = 0;
    int32_t  vel = 0;
    int16_t  torque = 0;
    int16_t  current = 0;
    int16_t  force = 0;
    int32_t  dcLinkVoltage = 0;
    int32_t  followingError = 0;
    uint16_t errorCode = 0;
    uint32_t alarmBits = 0;
    std::array<int16_t, 2> driveTemp{ 0, 0 };
    std::array<int16_t, 2> motorTemp{ 0, 0 };
 
    bool has(PusherField field) const noexcept;
};
```

| **字段**         | **类型**                 | **说明**                 |
| ---------------- | ------------------------ | ------------------------ |
| `online`         | `bool`                   | 推杆在线                 |
| `enabled`        | `bool`                   | 驱动器已使能             |
| `fault`          | `bool`                   | 驱动器故障               |
| `targetReached`  | `bool`                   | 目标到达                 |
| `mapped`         | `uint32_t`               | 已映射 PDO 字段位图      |
| `mode`           | `PusherMode`             | 公共模式映射             |
| `modeRaw`        | `int8_t`                 | 驱动器实际 0x6061 模式值 |
| `pos`            | `int32_t`                | 实际位置，µm             |
| `vel`            | `int32_t`                | 实际速度，µm/s           |
| `torque`         | `int16_t`                | 实际扭矩，0.1% 额定      |
| `current`        | `int16_t`                | 实际电流，0.1% 额定      |
| `force`          | `int16_t`                | 力传感器值，N            |
| `dcLinkVoltage`  | `int32_t`                | 母线电压，mV             |
| `followingError` | `int32_t`                | 跟随误差，µm             |
| `errorCode`      | `uint16_t`               | 驱动器错误码，0x603F     |
| `alarmBits`      | `uint32_t`               | 报警位图，0x2201         |
| `driveTemp`      | `std::array<int16_t, 2>` | 驱动器温度，0.1℃         |
| `motorTemp`      | `std::array<int16_t, 2>` | 电机温度，0.1℃           |

`has(PusherField)` 用于判断某个 PDO 字段是否已映射。

### 8.3 推杆可用模式

```cpp
enum class PusherMode : int8_t
{
    None = 0x00,
    Csp  = 0x08,
    Csv  = 0x09,
};
```

| **模式** | **说明**                           |
| -------- | ---------------------------------- |
| `None`   | 失能 / 未设置                      |
| `Csp`    | 周期同步位置模式；SDK 内部插补位置 |
| `Csv`    | 周期同步速度模式；SDK 内部平滑速度 |

### 8.4 PusherCommand

```cpp
struct PusherCommand
{
    bool       enable = false;
    bool       faultReset = false;
    PusherMode mode = PusherMode::Csp;
 
    int32_t pos = 0;
    int32_t vel = 0;
};
```

| **字段**     | **类型**     | **说明**                          |
| ------------ | ------------ | --------------------------------- |
| `enable`     | `bool`       | 使能推杆                          |
| `faultReset` | `bool`       | 一次性故障复位请求                |
| `mode`       | `PusherMode` | 公共控制模式                      |
| `pos`        | `int32_t`    | CSP 目标位置，µm                  |
| `vel`        | `int32_t`    | CSP 最大速度 / CSV 目标速度，µm/s |

模式与参数：

| **模式** | `pos`  | `vel` | **说明**                                        |
| -------- | ------ | ----- | ----------------------------------------------- |
| `Csp`    | 必须   | 可选  | 目标位置；`vel=0` 时使用默认最大速度 30000 µm/s |
| `Csv`    | 不使用 | 必须  | 目标速度；`vel=0` 表示停止                      |

### 8.5 pusherCount

```cpp
uint8_t pusherCount() const;
```

返回：当前识别的推杆数量。

### 8.6 getPusherStatus

```cpp
bool getPusherStatus(PusherId pusher, PusherState& out) const;
```

功能：读取推杆状态快照。

参数：

| **参数** | **类型**       | **说明** |
| -------- | -------------- | -------- |
| `pusher` | `PusherId`     | 推杆索引 |
| `out`    | `PusherState&` | 输出状态 |

返回：true 成功。

### 8.7 setPusherCommand

```cpp
bool setPusherCommand(PusherId pusher, const PusherCommand& cmd);
```

功能：下发推杆控制命令。

参数：

| **参数** | **类型**               | **说明**         |
| -------- | ---------------------- | ---------------- |
| `pusher` | `PusherId`             | 推杆索引         |
| `cmd`    | `const PusherCommand&` | 公共模式控制命令 |

返回：true 表示命令已接受。

说明：

- 切换模式会复位使能序列，需要重新使能
- `faultReset` 是一次性请求
- 行程限位有效时，目标位置会被裁剪到限位范围内

### 8.8 getPusherCommand

```cpp
bool getPusherCommand(PusherId pusher, PusherCommand& out) const;
```

功能：读取最近一次公共推杆命令。

返回：false 表示没有命令或当前模式不在公共接口范围内。

### 8.9 movePusher

```cpp
bool movePusher(PusherId pusher,
                int32_t posUm,
                uint32_t speedUmPerS = 0);
```

功能：使用 CSP 模式移动推杆。

参数：

| **参数**      | **类型**   | **说明**                           |
| ------------- | ---------- | ---------------------------------- |
| `pusher`      | `PusherId` | 推杆索引                           |
| `posUm`       | `int32_t`  | 目标位置，µm                       |
| `speedUmPerS` | `uint32_t` | 最大速度，µm/s；0 使用默认值 30000 |

返回：true 表示命令已接受。

### 8.10 resetPusherFault

```cpp
bool resetPusherFault(PusherId pusher);
```

功能：请求一次推杆故障复位。

返回：true 表示请求已接受。

说明：复位期间推杆失能，复位后需要重新使能。

## 9. 设备信息与温度

### 9.1 getCtrlFirmwareVersion

```cpp
bool getCtrlFirmwareVersion(FingerPart part, FirmwareVersion& out);
```

功能：读取指定 CTRL 设备的固件版本。

参数：

| **参数** | **类型**           | **说明**     |
| -------- | ------------------ | ------------ |
| `part`   | `FingerPart`       | 目标设备     |
| `out`    | `FirmwareVersion&` | 解析后的版本 |

返回：true 成功。

### 9.2 getEcFirmwareVersion

```cpp
bool getEcFirmwareVersion(HandModule module, FirmwareVersion& out);
```

功能：读取指定手板 EC 的固件版本。

参数：

| **参数** | **类型**           | **说明**     |
| -------- | ------------------ | ------------ |
| `module` | `HandModule`       | 目标手板     |
| `out`    | `FirmwareVersion&` | 解析后的版本 |

返回：true 成功。

### 9.3 FirmwareVersion

```cpp
struct FirmwareVersion
{
    bool    valid;
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
};
```

| **字段** | **说明**         |
| -------- | ---------------- |
| `valid`  | 版本字段是否有效 |
| `major`  | 主版本           |
| `minor`  | 次版本           |
| `patch`  | 补丁版本         |

版本显示接口：

```cpp
FirmwareVersion decodeVersion(uint16_t raw);
std::string toString(const FirmwareVersion& version);
```

### 9.4 getCurrentTemperature

```cpp
bool getCurrentTemperature(HandModule module, int16_t& centiCelsius);
```

功能：读取手板 EC 当前温度。

参数：

| **参数**       | **类型**     | **说明**         |
| -------------- | ------------ | ---------------- |
| `module`       | `HandModule` | 目标手板         |
| `centiCelsius` | `int16_t&`   | 温度，单位 0.01℃ |

返回：true 成功。

每个 `HandModule` 使用一个温度传感器。

### 9.5 getSafeTemperature

```cpp
bool getSafeTemperature(FingerPart part, uint8_t& celsius);
```

功能：读取指定设备的保护温度阈值。

参数：

| **参数**  | **类型**     | **说明**        |
| --------- | ------------ | --------------- |
| `part`    | `FingerPart` | 目标设备        |
| `celsius` | `uint8_t&`   | 温度阈值，单位℃ |

返回：true 成功；false 时 `celsius` 为 0。

### 9.6 setSafeTemperature

```cpp
bool setSafeTemperature(FingerPart part, uint8_t celsius);
```

功能：设置指定设备的保护温度阈值。

参数：

| **参数**  | **类型**     | **说明**                          |
| --------- | ------------ | --------------------------------- |
| `part`    | `FingerPart` | 目标设备                          |
| `celsius` | `uint8_t`    | 温度阈值，单位℃；小于 50 会被拒绝 |

返回：true 成功。

### 9.7 readEcInfo

```cpp
bool readEcInfo(std::vector<EcInfo>& out);
```

功能：读取所有 EC 手板信息。

`EcInfo` 主要字段：

| **字段**   | **类型**     | **说明**                                  |
| ---------- | ------------ | ----------------------------------------- |
| `valid`    | `bool`       | 信息是否有效                              |
| `module`   | `HandModule` | 手板                                      |
| `sysId`    | `uint16_t`   | 原始 SYS_ID                               |
| `appVer`   | `uint16_t`   | 原始应用版本，可用 `decodeVersion()` 解析 |
| `bootVer`  | `uint16_t`   | 原始 Boot 版本                            |
| `hwVer`    | `uint16_t`   | 硬件版本                                  |
| `testTime` | `uint16_t`   | 测试时间                                  |
| `sn`       | `uint32_t`     | EC 序列号原始值                           |
| `serial`   | `SerialNumber` | 解码后的序列号；`valid=false` 表示未写入 |
| `status`   | `uint16_t`   | EC 状态字                                 |
| `curTemp`  | `int16_t`    | 当前温度，0.01℃                           |
| `adminOn`  | `bool`       | Admin 模式                                |

### 9.8 readDeviceInfo

```cpp
bool readDeviceInfo(FingerPart part, DeviceInfo& out);
```

功能：读取指定 CTRL 设备信息。

`DeviceInfo` 主要字段：

| **字段**      | **类型**   | **说明**        |
| ------------- | ---------- | --------------- |
| `valid`       | `bool`     | 信息是否有效    |
| `sysId`       | `uint16_t` | 设备 SYS_ID     |
| `appVer`      | `uint16_t` | 原始应用版本    |
| `bootVer`     | `uint16_t` | 原始 Boot 版本  |
| `hwVer`       | `uint16_t` | 硬件版本        |
| `testTime`    | `uint16_t` | 测试时间        |
| `appLen`      | `uint32_t` | 应用镜像长度    |
| `appCrc`      | `uint32_t` | 应用镜像 CRC    |
| `sn`          | `uint32_t` | 设备序列号原始值 |
| `serial`      | `SerialNumber` | 解码后的序列号；`valid=false` 表示未写入 |
| `status`      | `uint16_t` | 设备状态字      |
| `curTemp`     | `int16_t`  | 当前温度，0.01℃ |
| `procTemp`    | `uint16_t` | 保护温度，0.01℃ |
| `iapFlag`     | `uint16_t` | IAP/Boot 标志   |
| `angle0Zero0` | `uint16_t` | Ang0 零位通道 0 |
| `angle0Zero1` | `uint16_t` | Ang0 零位通道 1 |
| `angle1Zero0` | `uint16_t` | Ang1 零位通道 0 |
| `angle1Zero1` | `uint16_t` | Ang1 零位通道 1 |
| `ratedCur`    | `uint16_t` | 额定电流        |
| `ratedRpm`    | `uint16_t` | 额定转速        |
| `ratedTemp`   | `uint16_t` | 额定温度        |

`SerialNumber` 字段：

| **字段** | **类型**      | **说明**                                                 |
| -------- | ------------- | -------------------------------------------------------- |
| `valid`  | `bool`        | 序列号是否有效                                           |
| `text`   | `std::string` | 文本形式 `DX022-S-L-M-YYYYMMDDNNNN`；`valid=false` 时为空 |

序列号由原始 `sn` 解码得到。出现以下情况时 `valid=false`：原始值为 0、版本/电机类型字段不符、月日越界、或日期不存在。

## 10. 触觉

### 10.1 TactilePart

```cpp
enum class TactilePart : uint8_t
{
    ThumbTip = 0,
    ThumbPad,
    IndexTip,
    IndexPad,
    MiddleTip,
    MiddlePad,
    RingTip,
    RingPad,
    LittleTip,
    LittlePad,
    Palm1,
    Palm2,
    Palm3,
    Palm4,
 
    ThumbForce,
    PalmForce,
    IndexMiddleForce,
    RingLittleForce,
 
    Count
};
```

| **部位**           | **有效通道数** |
| ------------------ | -------------- |
| 手指指尖 / 指腹    | 30             |
| 手掌 1–4           | 24             |
| `ThumbForce`       | 2              |
| `PalmForce`        | 1              |
| `IndexMiddleForce` | 4              |
| `RingLittleForce`  | 4              |

### 10.2 TactileData

```cpp
struct TactileData
{
    std::array<uint16_t, 30> values{};
    uint8_t count = 0;
};
```

| **字段** | **类型**                   | **说明**   |
| -------- | -------------------------- | ---------- |
| `values` | `std::array<uint16_t, 30>` | 通道值     |
| `count`  | `uint8_t`                  | 有效通道数 |

### 10.3 getTactile

```cpp
bool getTactile(TactilePart part, TactileData& out) const;
```

功能：读取指定部位最近一次的触觉数据。

参数：

| **参数** | **类型**       | **说明** |
| -------- | -------------- | -------- |
| `part`   | `TactilePart`  | 触觉部位 |
| `out`    | `TactileData&` | 输出数据 |

返回：true 表示该部位已收到过数据。

触觉帧按区域轮转上报，SDK 内部缓存每个区域最近一次接收到的数据。`count` 表示该区域的有效通道数。

## 11. 诊断与错误

### 11.1 getDiagnostics

```cpp
bool getDiagnostics(HandDiagnostics& out) const;
```

功能：读取运行时诊断快照。

`HandDiagnostics` 字段：

| **字段**               | **类型**   | **说明**                      |
| ---------------------- | ---------- | ----------------------------- |
| `adapterAvailable`     | `bool`     | 适配器已打开                  |
| `controlConnected`     | `bool`     | 控制连接已建立                |
| `maintenanceConnected` | `bool`     | 维护连接已建立                |
| `pdoHealthy`           | `bool`     | 最近一次 PDO 周期满足期望 WKC |
| `expectedWkc`          | `uint16_t` | 期望工作计数器                |
| `lastWkc`              | `uint16_t` | 最近一次工作计数器            |

### 11.2 isPdoHealthy

```cpp
virtual bool isPdoHealthy() const = 0;
```

返回：true 表示最近一次控制 PDO 周期的 WKC 达到期望值。

### 11.3 errorCode / errorMessage

```cpp
SysErrorCode errorCode() const;
std::string errorMessage() const;
```

功能：读取最近一次系统级错误。

说明：

- `errorCode()` 返回错误码
- `errorMessage()` 返回错误描述的快照

### 11.4 错误码

`SysErrorCode` 主要值：

| **错误码**                  | **说明**       |
| --------------------------- | -------------- |
| `NONE`                      | 无错误         |
| `ADAPTER_NA`                | 适配器不可用   |
| `DEV_TYPE_MISMATCH`         | 设备类型不匹配 |
| `CONN_LOST`                 | 连接丢失       |
| `CONN_TIMEOUT`              | 连接超时       |
| `BOOT_FAILURE`              | 启动失败       |
| `PERMISSION_DENIED`         | 权限不足       |
| `DEPENDENCY_MISSING`        | 依赖缺失       |
| `INTERFACE_NOT_FOUND`       | 网口不存在     |
| `NO_SLAVE_FOUND`            | 未发现从站     |
| `PDO_LAYOUT_MISMATCH`       | PDO 布局不匹配 |
| `PDO_UNHEALTHY`             | PDO 不健康     |
| `STATE_TRANSITION_FAILED`   | 状态切换失败   |
| `SDO_ABORT`                 | SDO abort      |
| `SII_INVALID`               | SII 无效       |
| `UPGRADE_FAILED`            | 升级失败       |
| `MAINTENANCE_BUSY`          | 维护连接占用   |
| `INTERNAL`                  | SDK 内部错误   |
| `FIRMWARE_UPGRADE_REQUIRED` | 需要升级固件   |
| `MODE_CONFLICT`             | 控制模式冲突   |
| `MODE_NOT_SET`              | 未设置控制模式 |

## 12. 设备操作

### 12.1 restartDevice

```cpp
bool restartDevice(FingerPart part);
```

功能：重启指定 CTRL 设备。

返回：true 成功。

### 12.2 setFillLight

```cpp
bool setFillLight(uint8_t brightness);
```

功能：设置四指板 EC device 的补光灯亮度。

参数：

| **参数**     | **类型**  | **说明**                 |
| ------------ | --------- | ------------------------ |
| `brightness` | `uint8_t` | 0 为关闭，255 为最大亮度 |

返回：true 成功；未检测到四指板时返回 false。

### 12.3 restartAllCtrl

```cpp
bool restartAllCtrl();
```

功能：失能并重新使能全部 CTRL 设备。

返回：true 表示全部设备重新使能成功。

### 12.4 requestRestartAllEc

```cpp
void requestRestartAllEc();
```

功能：请求全部 EC 手板重启。

说明：

- 手板立即复位
- 没有 SDO 返回值
- 调用方需要重新连接

## 13. 类型与枚举参考

### 13.1 HandSide

```cpp
enum class HandSide : uint8_t
{
    Unknown = 0,
    Left,
    Right,
};
```

### 13.2 HandModule

```cpp
enum class HandModule : uint8_t
{
    Thumb = 0,
    FourFinger,
    Unknown,
};
```

### 13.3 FingerPart

```cpp
enum class FingerPart : uint8_t
{
    ThumbDist = 0,
    ThumbProx,
    IndexProx,
    MiddleProx,
    RingProx,
    LittleProx,
    IndexMiddleDistal,
    RingLittleDistal,
    Count
};
```

### 13.4 PdoVersion

```cpp
enum class PdoVersion : uint8_t
{
    UNKNOWN = 0,
    V1_1 = 2,
};
```

### 13.5 ProductType / AdapterType

```cpp
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
```

### 13.6 PusherField

`PusherField` 用于 `PusherState::has()`，表示推杆 PDO 中可用的字段。

| **字段**      | **位** | **说明**            |
| ------------- | ------ | ------------------- |
| `Statusword`  | 0      | 0x6041 状态字       |
| `ModeDisplay` | 1      | 0x6061 模式显示     |
| `Position`    | 2      | 0x6064 实际位置     |
| `Velocity`    | 3      | 0x606C 实际速度     |
| `Torque`      | 4      | 0x6077 实际扭矩     |
| `Current`     | 5      | 0x6078 实际电流     |
| `Force`       | 6      | 0x300A 力传感器值   |
| `DcLinkVolt`  | 7      | 0x6079 母线电压     |
| `FollowErr`   | 8      | 0x60F4 跟随误差     |
| `ErrorCode`   | 9      | 0x603F 错误码       |
| `AlarmBits`   | 10     | 0x2201 报警位图     |
| `DriveTemp1`  | 11     | 0x3004 驱动器温度 1 |
| `DriveTemp2`  | 12     | 0x3005 驱动器温度 2 |
| `MotorTemp1`  | 13     | 0x3006 电机温度 1   |
| `MotorTemp2`  | 14     | 0x3007 电机温度 2   |

### 13.7 ConnectStage

| **值**       | **说明**   |
| ------------ | ---------- |
| `Idle`       | 空闲       |
| `Init`       | 网口初始化 |
| `ConfigInit` | 从站配置   |
| `ConfigMap`  | PDO 映射   |
| `Identify`   | 板型识别   |
| `SafeOp`     | SAFE_OP    |
| `Op`         | OP         |
| `Verify`     | 校验       |
| `Ready`      | 就绪       |

## 14. 单位约定

| **数据**                                                | **单位**     |
| ------------------------------------------------------- | ------------ |
| 关节角度                                                | 0.01°        |
| 关节目标角度                                            | 度           |
| 电机速度                                                | rpm          |
| 电机电流                                                | mA           |
| EC / 设备温度                                           | 0.01℃        |
| 保护温度（`getSafeTemperature` / `setSafeTemperature`） | ℃            |
| 推杆位置                                                | µm           |
| 推杆速度                                                | µm/s         |
| 推杆扭矩 / 电流                                         | 0.1% 额定    |
| 推杆力（命令）                                          | 0.1% 额定    |
| 推杆力（状态）                                          | N            |
| 触觉通道值                                              | 原始传感器值 |

## 15. 文档修订记录

| **日期**   | **版本** | **说明**                                                                                   |
| ---------- | -------- | ------------------------------------------------------------------------------------------ |
| 2026-11-08 | 1.0.1    | 初版用户接口文档，覆盖 `DexHand` 与 `DexHand_Pro`                                          |
| 2026-11-13 | 1.0.2    | 修改了moveJoint、setFillLight接口参数，删除了setCtrlEnable、清错接口和关节控制中的清错位。 |
| 2026-11-15 | 1.0.2    | 增加 Ubuntu 支持：`libs/` 平台化库目录、Linux 权限与网卡配置、CMake 接入方式、序列号字段说明。 |
