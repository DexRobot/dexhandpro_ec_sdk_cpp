# DexHand 021Pro EtherCAT SDK

## 概览

DexHand 021Pro EtherCAT SDK 为 021Pro 灵巧手提供 C++ 二次开发接口，通过主机网卡以 EtherCAT 与灵巧手通信，支持状态读取与关节运动控制。当前版本为 v1.0.1，只包含部分功能。

当前已提供：

- 网卡扫描与 EtherCAT 从站识别
- 单手实例创建、连接、断开
- 1 kHz PDO 周期通信，状态回调（默认 100 Hz，可配置 100–1000 Hz）
- 整手 16 关节状态快照与目标控制
- 单关节控制（自动处理耦合关节）
- EtherCAT 板信息、设备信息、保护温度等 SDO 只读接口

暂未提供：触觉数据、固件升级 / SII 烧录、推杆控制、Linux 版本。

## 目录结构

```
dexhandproLib/
├── CMakeLists.txt
├── README.md
├── include/
│   ├── commondef.h        # 导出宏、错误码、ProductType / AdapterType
│   ├── DexHand.h          # 主 API：DexHand / DexHand_Pro
│   └── DateTimeUtils.h    # 时间戳工具
├── lib/
│   └── windows/
│       ├── dexhandpro.dll
│       └── dexhandpro.lib
└── examples/
    ├── CMakeLists.txt
    ├── CMakeSettings.json
    ├── hand_status_demo.cpp   # 只读：连接、回调、状态快照
    ├── hand_move_demo.cpp     # 运动：单关节 +5°、回读、失能
    └── hand_sdo_demo.cpp      # 只读：SDO 读取板/设备信息
```

本目录是独立的交付工程，不依赖 SDK 源码工程，示例直接链接 `lib/windows/dexhandpro.lib`。

## 环境要求

| 项       | 要求                                              |
| -------- | ------------------------------------------------- |
| 操作系统 | Windows 10 / 11，x64                              |
| 网络驱动 | Npcap ≥ 1.70，安装时勾选 **WinPcap API 兼容模式** |
| 运行时   | VC++ 2015–2022 x64 Redistributable                |
| 构建示例 | Visual Studio 2022（v143）或 CMake ≥ 3.16，C++20  |
| 推荐网卡 | Intel I350，关闭节能、EEE 与中断节流              |

`dexhandpro.dll` 通过 Npcap 的 `wpcap.dll` 访问网卡；Npcap 需由用户自行安装，SDK 不随包分发。

## 构建示例

在 `dexhandproLib` 根目录执行：

```bash
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release

./build/bin/Release/hand_status_demo.exe
```

也可以用 VS2022“打开文件夹”选择 `dexhandproLib` 或 `examples` 目录，`CMakeSettings.json` 仅提供 `x64-Release` 配置。

构建完成后，`dexhandpro.dll` 会被自动复制到可执行文件目录；无需手动配置 PATH。

> **只能使用 Release 构建。** SDK 只提供 Release 版 DLL（`/MD` 运行时），Debug 版程序（`/MDd`）与 Release DLL 的 STL 容器布局不同，跨 DLL 传递 `std::string` / `std::vector` 会导致运行时崩溃、`(null)` 输出或卡死。工程已强制限制为 Release 配置。

## 使用流程

### 1. 扫描网卡并创建实例

```cpp
#include "DexHand.h"

using namespace DexRobot;
using namespace DexRobot::Dex021;

const auto interfaces = DexHand::scanDexInterfaces(AdapterType::EC_SOEM);
if (interfaces.empty())
    return -1;

auto hand = DexHand::createInstance(ProductType::DX021_PRO,
                                    AdapterType::EC_SOEM,
                                    interfaces.front());
if (!hand)
    return -1;
```

一个实例对应一个网口、一只物理手。控制另一只手时创建新实例并绑定另一个网口。

### 2. 注册状态回调

```cpp
hand->setStatusRxCallback([](const char* sn, const DX21StatusRxData* status)
{
    if (status == nullptr)
        return;

    const JointState joint = status->joint(JointId::IndexFlex);
    // 只做拷贝或短计算，禁止阻塞、SDO、connect/disconnect
});
```

回调运行在 PDO 线程上，单次执行应远小于 100 µs。

### 3. 连接

```cpp
if (!hand->connect(true))
{
    printf("connect failed [%s]: %s\n",
           toString(hand->errorCode()),
           hand->errorMessage().c_str());
    return -1;
}

hand->setRealtimeResponse(100, true);   // 回调频率 默认100 Hz
```

连接成功后进入 OP 状态，所有电机默认未使能。

### 4. 读取状态

```cpp
DX21StatusRxData::PTR status;
if (hand->getHandStatus(status) && status != nullptr)
{
    const float deg = status->joint(JointId::IndexFlex).angle / 100.0f;
}
```

也可通过 `handInfo()` 获取左右手、两块手板的 PDO 版本与 OP 状态。

### 5. 控制关节

```cpp
HandCommand cmd;
cmd.mode = 6;   // 三环控制模式

JointTarget& target = cmd.joints[static_cast<size_t>(JointId::IndexDistalCoupled)];
target.targetDeg = 30.0f;   // 单位：度
target.speedRpm  = 25000;
target.currentMa = 50;
target.enable    = true;

hand->setHandCommand(cmd);
```

- 命令采用 latest-wins 语义，下一次 PDO 周期生效。
- 单关节控制可使用 `moveJoint(JointId, const JointTarget&)`，SDK 会自动使能耦合关节。
- 目标角度会按模型范围自动钳位。
- 下发空 `HandCommand{}` 表示所有电机失能（安全态）。

### 6. 断开

```cpp
hand->disconnect();
```

`disconnect()` 会先下发零使能帧，再停止 PDO 线程并关闭网卡。析构函数等价于调用 `disconnect()`。

## 接口说明

### 连接与信息

| 接口                               | 说明                       |
| ---------------------------------- | -------------------------- |
| `DexHand::scanInterfaces()`        | 扫描本机所有网卡           |
| `DexHand::scanDexInterfaces()`     | 扫描挂有 021Pro 的网卡     |
| `createInstance()`                 | 创建实例                   |
| `connect(listen)` / `disconnect()` | 连接 / 断开                |
| `isConnected()` / `isAvailable()`  | 连接状态 / 实例可用性      |
| `handInfo(HandInfo&)`              | 左右手、手板版本、推杆信息 |
| `productType()`                    | 产品类型                   |
| `errorCode()` / `errorMessage()`   | 最近一次错误码与详细描述   |

### 数据

| 接口                                  | 说明                             |
| ------------------------------------- | -------------------------------- |
| `setStatusRxCallback(cb)`             | 注册状态回调                     |
| `setRealtimeResponse(rateHz, enable)` | 回调频率与开关，范围 100–1000 Hz |
| `getHandStatus(out)`                  | 读取最新整手状态快照             |
| `isPdoHealthy()`                      | PDO 工作计数是否正常             |
| `listen(enable)` / `isListening()`    | 单独开关回调                     |

### 控制

| 接口                       | 说明                 |
| -------------------------- | -------------------- |
| `setHandCommand(cmd)`      | 下发整手 16 关节目标 |
| `getHandCommand(out)`      | 读取当前命令槽       |
| `moveJoint(joint, target)` | 下发单关节目标       |

### SDO读取接口

| 接口                                   | 说明                       |
| -------------------------------------- | -------------------------- |
| `readEcInfo(out)`                      | 读取两块 EtherCAT 手板信息 |
| `readDeviceInfo(part, out)`            | 读取指定部位的控制设备信息 |
| `readProtectionTemperature(part, out)` | 读取保护温度（0.01 ℃）     |

维护 / 产线类接口（进入 admin、改 ID/SN、标零、改保护温度、补光、清错、重启等）为内部接口，不随本 SDK 提供给最终用户。

## 数据单位

| 数据                         | 单位               |
| ---------------------------- | ------------------ |
| 关节角度 `JointState::angle` | 0.01°              |
| `JointTarget::targetDeg`     | °                  |
| 电机速度                     | rpm                |
| 电机电流                     | mA                 |
| 板 / 设备温度                | 0.01 ℃             |
| 关节状态时间戳               | µs（steady_clock） |

## 运行注意事项

1. windows需要自行安装 Npcap。
2. 连接前确认手上电、网线连接正常；网卡建议使用 Inteli350 网卡并关闭节能特性。
3. 运动前建议先下发 `HandCommand{clearError = true}` 清除固件错误，等待约 200 ms 再下发目标。
4. 回调中不要调用 `connect` / `disconnect` / SDO 读取等阻塞接口。
5. 程序会在当前工作目录创建 `log/` 用于 TX/RX 报文日志，请确保目录可写。
6. **不要用 Debug 配置构建示例**。SDK 当前仅提供 Release 版本，Debug/Release 的 CRT 与 STL ABI 不兼容，混用会在运行期崩溃；工程已限制为 Release。

## 当前版本限制

- `getFirmwareVersion()`、`getSafeTemperature()`、`resetJoints()` 当前为空实现，返回固定值。
- 触觉数据、固件升级、SII 烧录、推杆控制尚未接入。
- 当前仅提供 Windows x64 版本。

## FAQ

**提示 `No adapter with EtherCAT slaves found`**

检查手是否上电、网线是否连接到主机网卡、ubuntu是否以管理员权限运行、Npcap 是否以 WinPcap API 兼容模式安装。

**`connect failed [NO_SLAVE_FOUND]`**

网卡上未扫描到 021Pro 从站。确认使用的是正确的网口，且网卡驱动 / 节能设置正常。

**`connect failed [PDO_LAYOUT_MISMATCH]`**

设备 PDO 布局与 SDK 支持的 V1.0 / V1.1 不一致，通常为固件版本不匹配，请升级或更换固件。

**`isPdoHealthy()` 返回 false**

PDO 工作计数持续不达标，检查线缆、电磁干扰与网卡中断节流设置。
