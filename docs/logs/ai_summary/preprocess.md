# ESP32 六足机器人项目预处理记录

> 范围：从设计硬件测试脚本开始，到芯片识别、Arduino-ESP32 核心安装、显示屏排查和资料阅读结束。

## 1. 硬件测试脚本

参考目录：

```text
thirdparty/Baize_H1mini/3.Software/Arduino测试代码
```

重点文件 `KeyControl_Servo/KeyControl_Servo.ino` 使用两块 PCA9685，I2C 地址为 `0x40`、`0x41`，共 32 路舵机；PWM 参数约为 `SERVOMIN=102`、`SERVOMID=327`、`SERVOMAX=552`；按键为 GPIO35、34、39、36，同时包含 `TFT_eSPI` 显示逻辑。原示例第二块板的循环边界出现过 `<31`，可能漏掉最后一路，测试脚本应明确覆盖 0～31。

测试脚本的原则是先把硬件链路拆开，不依赖 ROS 和显示屏：

1. 初始化串口/I2C并扫描总线。
2. 初始化 0x40、0x41。
3. 32 路舵机先回中。
4. 用串口逐路或整组测试。
5. 读取按键并做约 25 ms 消抖。

命令设计为 `?`（帮助）、`c`（全回中）、`a`（自动测试）、`t`（当前通道）、`+`/`-`（小幅调整）、`n`/`p`（切换通道）。动作限制在中位附近（例如 PWM 上下 40），串口 115200，PCA9685 50 Hz。

踩坑：`Wire.begin()` 成功不代表设备存在，必须扫描/读寄存器；舵机中位和限位不是通用常数；两块 PCA9685 不能同地址；当时只有静态检查（含 `git diff --check`），没有 `arduino-cli`，所以没有完成本机编译验证。

## 2. 上传失败和芯片识别

选择 Arduino Nano ESP32 时出现：

```text
No DFU capable USB device available
Failed uploading: uploading error: exit status 74
```

Nano ESP32 是 ESP32-S3，当前板子并非该型号。执行：

```bat
py -m esptool --port COM4 chip_id
```

得到：

```text
Chip type: ESP32-D0WDQ6 (revision v1.0)
Features: Wi-Fi, BT, Dual Core + LP Core, 240MHz
Crystal frequency: 40MHz
MAC: c8:f0:9e:bc:47:00
```

结论：当前是经典 ESP32，应在 Arduino IDE 选择 `ESP32 Dev Module`、实际端口 COM4，并使用普通 esptool 串口上传，不使用 DFU。esptool 5.x 新命令为：

```bat
py -m esptool --port COM4 chip-id
```

如果卡在 `Connecting...`，按住 BOOT/GPIO0 点击上传，在开始连接时松开；必要时按 EN/RST。不要因 DFU 错误反复选择 Nano ESP32。

## 3. Arduino-ESP32 核心安装

由于 Board Manager 从 GitHub 下载依赖失败，改为手动安装到：

```text
C:\Users\ZhuanZ\Documents\Arduino\hardware\espressif\esp32
```

Sketchbook 为 `C:\Users\ZhuanZ\Documents\Arduino`，使用镜像：

```text
https://jihulab.com/esp-mirror/espressif/arduino-esp32.git
```

初始化子模块后，在 `tools` 运行 `get.exe`/`get.py`，安装 Arduino 库、Xtensa/RISC-V 工具链、GDB、OpenOCD、esptool、LittleFS、SPIFFS。

### 3.1 代理语法

在 CMD 误输入 PowerShell 语法：

```text
$env:HTTPS_PROXY = "http://127.0.0.1:7897"
```

导致“文件名、目录名或卷标语法不正确”。CMD 应用：

```bat
cd /d C:\Users\ZhuanZ\Documents\Arduino\hardware\espressif\esp32\tools
set "HTTPS_PROXY=http://127.0.0.1:7897"
set "HTTP_PROXY=http://127.0.0.1:7897"
get.exe
```

PowerShell 才用：

```powershell
$env:HTTPS_PROXY = 'http://127.0.0.1:7897'
$env:HTTP_PROXY = $env:HTTPS_PROXY
.\get.exe
```

代理值只能写实际 URL，不能粘贴 Markdown 链接或额外反斜杠。

### 3.2 下载失败处理

先后遇到 GitHub 超时、`requests.exceptions.ConnectTimeout` 和：

```text
SSLError: [SSL: UNEXPECTED_EOF_WHILE_READING]
MaxRetryError
```

这是代理/GitHub 下载流中断，不是芯片故障。处理方法：

1. 用 `get.exe -t` 取得精确文件名和 URL。
2. 用 `curl.exe` 配置代理、重试和断点续传，下载到 `tools\dist`。
3. 文件名必须完全匹配。
4. 再用 `get.exe -v` 解压安装。

命令形式：

```powershell
curl.exe --location --fail --proxy http://127.0.0.1:7897 --connect-timeout 30 --retry 10 --retry-delay 5 --retry-all-errors --continue-at - --output '<tools>\dist\<archive>' '<url>'
```

Arduino 缓存 `C:\Users\ZhuanZ\AppData\Local\Arduino15\staging\packages` 中已有部分精确版本包，可复用到 `tools\dist`；不能混用不同版本。

校验命令：

```powershell
Get-FileHash '<file>' -Algorithm SHA256
```

已校验的关键包包括：`esp32-arduino-libs-idf-release_v5.5-73550728-v6.zip`（`6d923bbf17f16d094adc6b503697bf90cce306856350ee815e087d5582f7d37a`）、Xtensa/RISC-V GDB、OpenOCD、esptool 和 LittleFS。最终 `get.exe -v` 输出 `Platform Tools Installed`；`esptool.exe` 输出 `esptool v5.3.0`。工具链安装成功，但不等于项目已经编译或上传成功。

## 4. 显示屏排查

代码使用 `TFT_eSPI`：

```cpp
#include <TFT_eSPI.h>
TFT_eSPI tft;
tft.init();
tft.setRotation(3);
tft.fillScreen(TFT_BLACK);
```

TFT_eSPI 不会自动识别屏幕型号和引脚，配置由 `User_Setup.h` 决定。检查：

```text
C:\Users\ZhuanZ\Documents\Arduino\libraries\TFT_eSPI\User_Setup.h
```

当前启用：

```cpp
#define ILI9341_DRIVER
#define TFT_MISO PIN_D6
#define TFT_MOSI PIN_D7
#define TFT_SCLK PIN_D5
#define TFT_CS   PIN_D8
#define TFT_DC   PIN_D3
#define TFT_RST  PIN_D4
```

`PIN_D5`～`PIN_D8` 是 ESP8266 NodeMCU 风格别名，与经典 ESP32 存在不匹配风险。配置中虽有常见 ESP32 示例 19/23/18/15/2/4，但不能直接认定为本板接线。必须确认驱动 IC（ILI9341、ST7789 等）、CS/DC/RST/MOSI/MISO/SCLK、BL、电源及电平转换电路。

接线确认后先烧录最小显示测试：

```cpp
#include <TFT_eSPI.h>
TFT_eSPI tft;
void setup() {
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLUE);
  tft.setTextColor(TFT_WHITE, TFT_BLUE);
  tft.drawString("Baize ESP32", 20, 20, 2);
}
void loop() {}
```

仍不显示时，优先检查电源、接线、驱动宏和 TFT_eSPI setup，不要继续修改机器人业务代码。

## 5. 硬件资料结论

`thirdparty/Baize_H1mini/2.Hardware files(EasyEDA)` 主要是旧版舵机控制板和 `Baize_ServoDriver_esp8266_V1.0`，核心为 `ESP-12F(ESP8266MOD)`。虽然有 SCLK/MOSI/MISO/CS0/SDA/SCL 网络名，但没有明确 ESP32 版 TFT 接口，也没有 `TFT_DC`、`TFT_RST`、`TFT_BL` 映射，不能据此确定当前第三方板的屏幕引脚。

硬件使用说明主要涉及 USB、PS2、MP3、串口、电源和舵机。ROS 版制作教程 PDF 共 154 页、版本 20231126，明确区分 `Baize_ServoDriver_esp8266` 和 `Baize_ServoDriver_esp32`；Arduino 示例偏向 ESP8266，使用 ESP32 必须用对应 ESP32 工程。教程没有给出当前第三方板完整 TFT_eSPI 配置。

## 6. 当前结论、待办和 Git 注意事项

已确认：

- 芯片是 ESP32-D0WDQ6，COM4，不能按 Nano ESP32/DFU 处理。
- Arduino-ESP32 工具链已通过手动下载、缓存复用和校验安装。
- 舵机测试应独立验证 I2C、PCA9685 和舵机。
- 当前 TFT_eSPI 使用 ESP8266 风格 PIN_D* 配置，存在明显风险。
- 资料能确认 ESP8266/ESP32 是两个版本，但不能确认当前 TFT 引脚。

尚未确认：第三方板准确型号和 GPIO 分配、TFT 驱动/分辨率/SPI 接线、最小显示结果、舵机真实动作结果，以及 Arduino IDE 是否完成正确板型下的完整编译上传。

下一步：

1. 记录板子正反面丝印、排线标号和接口名称。
2. 从实物或 ESP32 原理图确认 SCLK/MOSI/MISO/CS/DC/RST/BL。
3. 修改 TFT_eSPI setup，先测最小显示程序。
4. 单独测 I2C/PCA9685 和 32 路舵机。
5. 最后合并显示、舵机、Wi-Fi/ROS。

Git 方面，`thirdparty` 已忽略其下级内容并保留 `thirdparty/urls.md`。本次只新增本文档，不自动提交或推送；工作树已有用户自己的改动，提交前执行：

```powershell
git status --short
git diff -- .gitignore preprocess.md
```

不要使用 `git reset --hard` 或清理未跟踪文件来“整理”状态。

推荐顺序：`esptool chip-id` 确认芯片 → 选择正确 board/COM → 最小串口程序 → I2C/PCA9685 → TFT 最小程序 → 最后合并机器人功能。
