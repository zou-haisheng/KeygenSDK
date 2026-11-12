[English](README.md) | [简体中文](README.zh-CN.md)

# KeygenSDK

一个面向 [Keygen CE](https://keygen.sh) 的轻量级现代 C++ 20 客户端 SDK（Windows）。
它让 C++ 应用能够验证许可证、激活与停用机器、在本地持久化许可证状态，以及离线验证许可证——并通过 CMake 与 vcpkg 实现干净的集成。

## 功能特性

- **在线许可证验证** —— 通过 Keygen API 验证许可证密钥。
- **机器激活 / 停用** —— 激活机器，随后可停用。
- **本地许可证持久化** —— 跨进程生命周期保存与恢复激活状态，使用 Windows DPAPI 保护。
- **离线许可证验证** —— 无需任何网络访问即可验证已检出的机器文件，使用 Ed25519 签名与 AES-256-GCM。

## 环境要求

- **C++20** 编译器（MSVC；项目基于 Visual Studio 2026 开发）。
- **CMake 3.25+**（Visual Studio 2026 内置兼容的 CMake 4.x）。
- **vcpkg**（manifest 模式，`x64-windows` 或 `x86-windows` triplet）。
- **Windows（x86 与 x64）**。

## 依赖

通过 vcpkg manifest（[`vcpkg.json`](vcpkg.json)）管理：

- `curl` —— HTTP 客户端。
- `nlohmann-json` —— JSON 解析与序列化。
- `openssl` —— 离线机器文件的 Ed25519 签名验证与 AES-256-GCM 解密。

Windows 构建额外使用系统 API `Crypt32`（DPAPI）、`bcrypt` 和 `advapi32`，用于机器身份与本地数据保护。

## 构建

使用项目提供的 CMake presets 配置与构建：

```powershell
# 将 vcpkg 指向你的 vcpkg 安装目录
$env:VCPKG_ROOT = "C:\path\to\vcpkg"

# 配置（Visual Studio 生成器，x64）
cmake --preset vs2026-x64

# 构建 Debug 与 Release
cmake --build build-vs --config Debug
cmake --build build-vs --config Release

# 运行测试
ctest --test-dir .\build-vs -C Debug --output-on-failure

# 32 位（x86）构建
cmake --preset vs2026-x86
cmake --build build-vs-x86 --config Debug
ctest --test-dir .\build-vs-x86 -C Debug --output-on-failure
```

测试通过时会输出：

```text
100% tests passed, 0 tests failed
```

## 配置

SDK 的所有选项都通过 `KeygenSDK::Config` 结构体传入：

| 字段 | 类型 | 说明 |
|---|---|---|
| `host` | `std::string` | Keygen 服务器 URL。必须使用 `https://`。 |
| `accountId` | `std::string` | 你的 Keygen 账户 ID。 |
| `accountPublicKey` | `std::string` | 十六进制编码的 Ed25519 账户公钥。仅 `verifyOffline()` 需要。 |
| `timeoutSeconds` | `long` | HTTP 超时时间（秒）。默认 `15`。 |
| `localLicensePath` | `std::filesystem::path` | 本地许可证状态的持久化路径。为空则使用默认路径。 |

默认的本地许可证路径为：

```text
%LOCALAPPDATA%\KeygenSDK\license.dat
```

```cpp
KeygenSDK::Config config{
    .host = "https://example.com",
    .accountId = "your-account-id",
    .accountPublicKey = "your-account-ed25519-public-key-hex",
    .timeoutSeconds = 15,
    .localLicensePath = "C:/MyApplication/license.dat"
};
```

`accountPublicKey` 仅在需要离线验证时才需要；仅在线场景可以省略。许可证密钥与账户相关值在运行时提供，绝不硬编码进 SDK。

## 快速开始

```cpp
#include <KeygenSDK/KeygenSDK.h>

#include <iostream>

int main() {
    KeygenSDK::Config config{
        .host = "https://example.com",
        .accountId = "your-account-id",
        .accountPublicKey = "your-account-ed25519-public-key-hex",
    };

    KeygenSDK::Client client(std::move(config));

    const KeygenSDK::Result result =
        client.validateOnline("YOUR_LICENSE_KEY");

    if (result.ok) {
        std::cout << "License is valid\n";
    } else {
        std::cout << "Validation failed: "
                  << KeygenSDK::toString(result.error) << '\n';
    }

    return 0;
}
```

## API

### Client

主要入口是 `KeygenSDK::Client`：

```cpp
class Client {
public:
    explicit Client(Config config);
    ~Client();

    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    Client(Client&&) noexcept;
    Client& operator=(Client&&) noexcept;

    [[nodiscard]] Result validateOnline(const std::string& licenseKey);
    [[nodiscard]] Result activate(const std::string& licenseKey);
    [[nodiscard]] Result verifyOffline();
    [[nodiscard]] Result deactivate();
    [[nodiscard]] Result loadLocalLicense();
    [[nodiscard]] bool hasLocalLicense() const noexcept;
};
```

### Result

每个操作都返回一个 `KeygenSDK::Result`：

```cpp
struct Result {
    bool ok{false};
    ErrorCode error{ErrorCode::Unknown};
    std::string message;

    [[nodiscard]] bool success() const noexcept { return ok; }
    explicit operator bool() const noexcept { return ok; }
};
```

通过检查 `result.ok`（或在布尔上下文中直接使用 `result`）来判断是否成功。失败时，`result.error` 与 `result.message` 描述原因。

### 错误码

```cpp
enum class ErrorCode {
    None,
    InvalidConfiguration,
    NetworkError,
    InvalidLicense,
    LocalStateNotFound,
    LocalStorageError,
    LicenseExpired,
    LicenseSuspended,
    LicenseRevoked,
    LicenseOverdue,
    MachineLimitReached,
    ActivationFailed,
    OfflineDataMissing,
    OfflineVerificationFailed,
    InvalidResponse,
    ServerError,
    Unknown,
};
```

使用 `KeygenSDK::toString(code)` 获取错误码的稳定、可读名称。

### 进阶类型

以下底层类型也会作为公共头文件安装，供进阶使用：
`HttpClient` / `IHttpClient`、`MachineIdentity`、`MachineFileVerifier`、
`LocalLicenseState`、`ILocalLicenseStore`、`FileLocalLicenseStore`、
`LocalLicenseSerializer` 与 `LocalDataProtection`。

## 使用说明

### 在线许可证验证

对 Keygen 的 `validate-key` 端点验证许可证密钥：

```text
POST /v1/accounts/<accountId>/licenses/actions/validate-key
```

```cpp
const KeygenSDK::Result result =
    client.validateOnline("YOUR_LICENSE_KEY");
```

许可证相关的错误状态（已过期、已暂停、逾期等）会映射到对应的 `ErrorCode` 值。

### 机器激活

`activate()` 会验证许可证、生成机器指纹、在 Keygen 服务器上创建机器、检出机器文件，并持久化生成的本地许可证状态：

```cpp
const KeygenSDK::Result result =
    client.activate("YOUR_LICENSE_KEY");

if (result.ok && client.hasLocalLicense()) {
    // 机器已激活，状态已持久化到本地。
}
```

机器指纹由 Windows `MachineGuid` 派生（用 SHA-256 哈希），并非原始硬件数据。

### 机器停用

`deactivate()` 移除远端机器并清除本地状态：

```cpp
const KeygenSDK::Result result = client.deactivate();
```

如果远端停用成功但本地清理失败，内存中的激活状态仍会被清除。

### 离线许可证验证

`verifyOffline()` 在不发起任何 HTTP 请求的情况下验证本地存储的机器文件：

```cpp
const KeygenSDK::Result result = client.verifyOffline();

if (result.ok) {
    // 离线机器文件有效且未过期。
}
```

它会校验 Ed25519 签名、解密并验证机器载荷、核验其元数据与机器身份，并对已过期的文件返回 `LicenseExpired`。

### 恢复本地许可证

在后续的进程生命周期中恢复之前持久化的激活状态：

```cpp
const KeygenSDK::Result result = client.loadLocalLicense();

if (result.ok && client.hasLocalLicense()) {
    // 本地许可证状态已恢复。
}
```

## 本地许可证持久化

持久化状态包含许可证 ID、许可证密钥、机器 ID、机器指纹，以及（激活后）检出的机器文件。

### 结构（Schema）

状态使用带版本号的 JSON 表示。当前 schema 版本为 `2`：

```json
{
    "version": 2,
    "license": {
        "id": "...",
        "key": "..."
    },
    "machine": {
        "id": "...",
        "fingerprint": "...",
        "file": "..."
    }
}
```

版本 `1`（不含 `machine.file`）仍被接受，以向后兼容在引入离线机器文件之前持久化的状态。

### DPAPI 保护

序列化后的状态在写入磁盘前，会用 Windows 数据保护 API（DPAPI）在当前用户作用域下加密：

```text
LocalLicenseState → JSON → DPAPI → license.dat
```

SDK 不会将许可证密钥或其他本地许可证状态以明文形式存储到磁盘。

### 原子写入

保存会先写入一个临时文件，刷新后再用 `MoveFileExW(MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)` 原子地移动到目标位置。失败时会清理临时文件，从而降低留下不完整许可证文件的风险。

## 离线验证细节

SDK 使用配置的账户公钥、本地许可证密钥与机器指纹来验证并解密 Keygen 机器文件。

验证器会：

- 要求 `MACHINE FILE` 包裹格式与 `aes-256-gcm+ed25519` 算法。
- 对 `machine/` 后接加密载荷的内容验证 Ed25519 签名。
- 以 `SHA256(licenseKey + machineFingerprint)` 派生 AES-256-GCM 密钥。
- 使用 AES-256-GCM（12 字节 IV、16 字节认证标签）解密载荷。
- 校验解密后的 JSON:API 机器资源，包括其非空 ID 与 `machines` 类型，以及 `meta.issued`、`meta.expiry`、`meta.ttl` 元数据。
- 对已过期文件返回 `LicenseExpired`。

## 安装与使用

项目导出一个 CMake 包。安装后可在其他 CMake 项目中使用：

```cmake
find_package(KeygenSDK CONFIG REQUIRED)

target_link_libraries(
    MyApplication
    PRIVATE
        KeygenSDK::KeygenSDK
)
```

然后包含公共头文件：

```cpp
#include <KeygenSDK/KeygenSDK.h>
```

已安装的包会声明对 `curl`、`nlohmann-json` 与 `openssl` 的 `find_dependency`，因此消费项目需要通过 vcpkg（使用相同的 `x64-windows` triplet）提供这些包，或以其他方式让 CMake 能找到它们。

## 测试

项目使用轻量级自研测试框架（不使用 GoogleTest）和一个假的 HTTP 客户端，因此测试套件无需真实的 Keygen 服务器即可运行。

运行测试套件：

```powershell
ctest --test-dir .\build-vs -C Debug --output-on-failure
```

测试套件覆盖：在线验证（有效、过期、暂停、逾期、无效许可证；HTTP 与服务器失败；无效响应）、机器激活与停用（成功与失败路径）、本地持久化（序列化、schema 版本、DPAPI、原子替换、临时文件清理、被篡改的数据），以及离线机器文件验证（Ed25519 与 AES-256-GCM、时间戳、TTL、指纹与机器 ID 匹配）。

## 安全说明

SDK 不会硬编码许可证密钥、账户凭据、认证令牌或私钥。本地许可证状态使用 Windows DPAPI 保护，机器身份使用哈希后的指纹而非原始硬件数据。客户端侧验证不能替代服务端许可证强制执行。

## 许可证

本项目采用 MIT 许可证。详见 [LICENSE](LICENSE)。
