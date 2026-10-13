# CH32X035F8U6 传感器与 WCHGUI 屏幕监控系统

本项目基于沁恒 RISC-V 微控制器 **CH32X035F8U6**（亮度采集板 V1.0），使用 **MounRiver Studio Ⅱ** 框架构建。系统实现了环境光照强度采集、三轴加速度与姿态角解算、0.96 寸彩色 TFT 屏高帧率显示、屏幕背光 PWM 调光、用户按键交互及系统心跳指示。

图形界面完全由 **WCHGUIDesigner** 官方可视化设计器生成，工程源码直接引用并驱动 `WCHGUIProject/generated/wui_user/` 目录中的 UI 驱动代码，实现了底层驱动、GUI 框架、用户业务逻辑的三层彻底解耦。

---

## 1. 硬件引脚资源映射表

| 芯片引脚 | 网络标号 | 对应功能模块 | 硬件电路特性与驱动方式 |
| :--- | :--- | :--- | :--- |
| **Pin 1** | `VDD` | 3.3V 主电源 | ME6211C33M5G-N LDO 供电 |
| **Pin 2** | `PA0` (INT1) | 姿态传感器中断 1 | LIS2DW12TR `INT1`，板载 10kΩ 上拉电阻 (R10) |
| **Pin 3** | `PA1` (INT2) | 亮度传感器中断 | LTR-308ALS-01 `INT`，板载 10kΩ 上拉电阻 (R11) |
| **Pin 4** | `PA2` (K1) | 用户按键 SW2 | 板载 10kΩ 上拉电阻 (R13) + 100nF 滤波电容 (C10)，按下为低电平 |
| **Pin 5** | `PA3` (BL) | TFT 屏幕背光调节 | 驱动 DTC143ZE (U5) 偏置三极管控制 LEDK；支持 **TIM2_CH4 PWM 调光** / GPIO |
| **Pin 6** | `PA4` (TRES) | TFT 屏幕复位 (Reset) | 通用推挽输出 (GPIO_Mode_Out_PP)，低电平复位 |
| **Pin 7** | `PA5` (TSCL) | TFT 屏幕 SPI 时钟 (SCL) | **硬件 SPI1_SCK** 复用推挽输出 (24MHz 高速传输，主频 48MHz/2) |
| **Pin 8** | `PA6` (TRS) | TFT 屏幕 数据/命令选择 (DC) | 通用推挽输出 (GPIO_Mode_Out_PP)，高电平数据，低电平命令 |
| **Pin 9** | `PA7` (TSDA) | TFT 屏幕 SPI 数据 (MOSI) | **硬件 SPI1_MOSI** 复用推挽输出 (24MHz 高速传输，主频 48MHz/2) |
| **Pin 10** | `PB0` (SDA) | I2C 总线数据线 (SDA) | 连接 LTR-308 与 LIS2DW12，板载 4.7kΩ 上拉 (R7)，模拟开漏时序 |
| **Pin 11** | `PB1` (SCL) | I2C 总线时钟线 (SCL) | 连接 LTR-308 与 LIS2DW12，板载 4.7kΩ 上拉 (R6)，推挽输出时钟 |
| **Pin 12** | `PB3` | 未使用 | 悬空引脚 |
| **Pin 13** | `PB11` | 未使用 | 悬空引脚 |
| **Pin 14** | `PC18` (DIO) | 调试接口 SWDIO | 连接 Type-C SBU1，用于 WCH-Link 烧录与仿真 |
| **Pin 15** | `PB12` (RUN) | 运行状态指示灯 LED1 | 阳极经 1kΩ (R3) 接 3.3V，阴极接 PB12；**低电平点亮**，心跳指示 |
| **Pin 16** | `PC19` (DCK) | 调试接口 SWCLK | 连接 Type-C SBU2，用于 WCH-Link 烧录与仿真 |
| **Pin 17** | `PC16` (UDM) | USB 2.0 数据 D- | 连接 Type-C USB DM（带 USBLC6-2SC6 ESD 保护） |
| **Pin 18** | `PC17` (UDP) | USB 2.0 数据 D+ / Boot 按键 | 经 1kΩ (R12) 接 3.3V 并连接 SW1 按键；**按下接高电平 (拉高)**，进入芯片内置 Bootloader 烧录模式 |
| **Pin 19** | `PC14` (CC1) | Type-C 配置通道 1 | 5.1kΩ 下拉至地 |
| **Pin 20** | `PC15` (CC2) | Type-C 配置通道 2 | 5.1kΩ 下拉至地 |
| **Pin 21** | `GND` | 芯片底部接地焊盘 (EP) | 参考地 |
| **—** | `CS` | TFT 屏幕片选 | **硬件固定接地 (GND)**，始终选通使能，无需占用额外引脚 |

---

## 2. 软件工程目录架构

```text
CH32X035F8U/
├── App/                                        # 应用层与系统入口 (业务逻辑收拢)
│   ├── main.c                                  # 系统主循环、任务调度与页面数据流推送
│   ├── app_sensor.h / app_sensor.c             # 传感器数据融合采样、中断统计与俯仰/横滚姿态角解算
│   ├── ch32x035_it.h / ch32x035_it.c           # 中断服务函数 (EXTI 中断、系统嘀嗒等)
│   ├── system_ch32x035.h / system_ch32x035.c   # 芯片系统时钟初始化配置 (48MHz 主频)
│   └── ch32x035_conf.h                         # 固件库外设头文件使能配置
├── Hardware/                                   # 硬件驱动层 (传感器/屏幕/外设总线/USB-CDC 收拢)
│   ├── soft_i2c.h / soft_i2c.c                 # 模拟 I2C 驱动 (PB0-SDA, PB1-SCL)，标准开漏时序与超时检测
│   ├── spi_driver.h / spi_driver.c             # 硬件 SPI1 驱动 (PA5-SCK, PA7-MOSI)，24MHz 寄存器级高速直通传输
│   ├── usb_cdc.h / usb_cdc.c                   # USB-CDC 虚拟串口驱动 (PC16-DM, PC17-DP 启动后重映射为调试串口)
│   ├── lis2dw12.h / lis2dw12.c                 # LIS2DW12TR 三轴加速度计驱动 (I2C: 0x19, 14-bit 高性能, INT1 使能)
│   ├── ltr308.h / ltr308.c                     # LTR-308ALS-01 环境光传感器驱动 (I2C: 0x53, 3X增益, INT2 使能)
│   ├── st7735.h / st7735.c                     # 0.96寸 ST7735S 屏幕底层驱动 (80x160 偏移校准与窗口绘制)
│   ├── bsp_int.h / bsp_int.c                   # 外部中断检测 (PA0-INT1 LIS2DW12, PA1-INT2 LTR308, EXTI7_0)
│   ├── bsp_led.h / bsp_led.c                   # LED1 运行指示灯 (PB12，心跳控制)
│   ├── bsp_key.h / bsp_key.c                   # 用户按键 SW2 (PA2，非阻塞消抖与长短按识别)
│   └── bsp_backlight.h / bsp_backlight.c       # 背光驱动 (PA3，TIM2_CH4 PWM 调光与开关)
├── Core/                                       # WCH 官方 RISC-V QingKe 内核支持库
├── Peripheral/                                 # WCH 官方标准外设固件库 (GPIO, TIM, SPI, USB 等)
├── Startup/ & Ld/                              # 启动汇编 (startup_ch32x035.S) 与链接脚本 (Link.ld)
├── Debug/                                      # WCH 官方调试串口底层支持库
├── wui/                                        # WCH 官方 WCHGUI 运行时核心库 (libwui_0.2.a)
├── wui_port/                                   # WCHGUI 硬件抽象移植层 (对齐 ST7735S、DMA、字体、内存池)
├── WCHGUIProject/                              # WCHGUIDesigner 可视化设计器工程与生成代码 (wui_user/)
├── obj/                                        # MounRiver 编译输出目录
└── README.md                                   # 项目文档与技术规格书
```

---

## 3. UI 架构与页面流转机制

### 3.1 页面切换逻辑
```mermaid
flowchart TD
    A[系统上电复位] --> B[硬件外设初始化: I2C / SPI / LED / KEY / BL / ST7735]
    B --> C[初始化 WUI_Port 并载入 Page 1]
    C --> D[显示 Page 1: 开机欢迎页与片内 Logo]
    D --> E[系统硬件自检与传感器初始化]
    E -->|自检通过 / 延时1.2s| F[平滑自动切换至 Page 2: 数据监控页]
    F --> G[进入主任务循环]
    G --> H[10ms: 按键消抖与长短按处理]
    G --> I[100ms: 采集传感器并调用 wui_page_page_2_update_data 刷新显示]
    G --> J[500ms: PB12 运行指示灯心跳闪烁]
```

### 3.2 Page 2 控件与数据映射说明
在 `WCHGUIProject/generated/wui_user/wui_page_page_2.h` 与 `.c` 中，定义了数据驱动接口：
```c
void wui_page_page_2_update_data(float lux, float pitch, float roll, float ax, float ay, float az, uint8_t bl_percent);
```
控件 ID 与显示内容绑定关系如下：
- **`PAGE_2_COMPONENT_001`**：环境光照强度（`光强: xxxx`），单位 Lux。
- **`PAGE_2_COMPONENT_002`**：姿态角度（`姿态:P:xx R:xx`），实时显示俯仰角与横滚角（度）。
- **`PAGE_2_COMPONENT_003`**：X 轴加速度（`X: xxx`），单位 mg。
- **`PAGE_2_COMPONENT_004`**：Y 轴加速度（`Y: xxx`），单位 mg。
- **`PAGE_2_COMPONENT_005`**：Z 轴加速度（`Z: xxx`），单位 mg。
- **`PAGE_2_COMPONENT_006`**：屏幕背光占空比（`亮度: xxx%`）。

---

## 4. 各核心功能模块技术细节

### 4.1 亮度传感器：LTR-308ALS-01
- **接口**：模拟 I2C，从机地址 `0x53`。
- **配置**：3X 增益，100ms 积分时间，软件复位与 `PART_ID` 自检校验。
- **换算**：自动将 20-bit 原生通道计数值换算为标准物理单位 Lux。

### 4.2 姿态传感器：LIS2DW12TR
- **接口**：模拟 I2C，从机地址 `0x19`。
- **配置**：14-bit 高性能模式，±2g 量程，灵敏度 0.244 mg/LSB，启用 BDU（块数据更新防撕裂）。
- **姿态解算**：基于重力分量三角函数实时解算 Pitch / Roll 角。

### 4.3 0.96 寸彩色 TFT 屏：ST7735S 与 WCHGUI 移植
- **接口**：硬件 SPI1（PA5: SCK, PA7: MOSI），24MHz 传输主频（48MHz/2）。
- **坐标修正**：横屏模式（160×80），Offset 校准为 $X=1, Y=26$。
- **HAL 移植**：`wui_port.c` 实现了标准 `wui_hal_t`，通过 `memcpy` 直接从片内 Flash 数组 `wui_internal_data` 读取开机图片，由 WCHGUI 静态库驱动无闪烁渲染。

### 4.4 背光调节与按键交互
- **背光控制**：PA3 由 TIM2_CH4 输出 10kHz PWM，支持 0%~100% 占空比。
- **用户按键 (PA2)**：
  - **单击**：循环切换背光档位（`100% -> 75% -> 50% -> 25% -> 10% -> 100%`），界面实时同步显示亮度百分比。
  - **长按**（>600ms）：关断背光进入节能状态 / 再次长按恢复背光。
- **心跳指示 (PB12)**：主循环以 500ms 周期翻转低电平驱动 LED1。

### 4.5 Type-C 接口双模运行机制 (USB ISP 烧录 vs USB CDC 虚拟串口调试)
板载 Type-C 接口连接至 MCU 的 PC16 (UDM) 与 PC17 (UDP)，通过硬件上电检测实现免额外转接模块的双模智能复用：
1. **USB 烧录模式 (ROM Bootloader)**：
   - 按住板载按键 **SW1** 插入 Type-C 线或复位，硬件将 PC17 经 R12(1kΩ) 接 3.3V 拉高；
   - 芯片自动进入出厂固化的 Bootloader，电脑识别为 **WCH USB ISP 设备**，使用 WCHISPTool 极速下载固件。
2. **USB CDC 虚拟串口调试模式 (User App)**：
   - 正常开机启动（松开 SW1），MCU 直接执行用户固件；
   - 固件自动将 USBFS 外设初始化为 **USB CDC-ACM 设备**（全速 12Mbps），Windows 10/11 系统免驱自动枚举为 `USB 串行设备 (COMx)`；
   - 底层 `printf` 已重定向至 USB CDC 环形发送队列，打开串口助手（115200 8N1）即可实时接收传感器结构化调试日志；
   - **非阻塞无感设计**：未插电脑或串口未打开时，发送函数直接非阻塞跳过，不影响屏幕刷新与系统流畅度。
   - **调试日志输出格式示例**：
     ```text
     [DEBUG] Lux: 850.0 | P:  0 R:  0 | X:   0 Y:   0 Z: 980 | BL:100% | INT1:52 INT2:5
     ```

### 4.6 外部硬件中断监控：INT1 与 INT2
针对板载两路硬件传感器中断引脚，实现了全流程闭环事件检测：
- **PA0 (INT1 - LIS2DW12TR)**：
  - 加速度计配置为开漏输出 (`PP_OD=1`)、低电平有效 (`H_LACTIVE=1`)，启用 `CTRL4_INT1_PAD_CTRL` 中的数据就绪中断 (`DRDY`)；
  - 芯片每完成一次采样（100Hz ODR），INT1 产生低电平脉冲，由 MCU 侧 `EXTI_Line0` 下降沿中断捕获并累加触发计数器。
- **PA1 (INT2 - LTR-308ALS-01)**：
  - 环境光传感器配置为开漏输出、低电平有效 (`INT_POL=0`)，启用 `INT_CFG` 中的 ALS 测量中断；
  - 传感器完成光强积分转换后拉低引脚，由 MCU 侧 `EXTI_Line1` 下降沿中断捕获并累加计数器。
- **中断控制器 (PFIC)**：
  - 统一映射至青稞 RISC-V 快速中断通道 `EXTI7_0_IRQn`（抢占 1，子优先级 1），保证微秒级低延迟响应；
  - 提供 `Bsp_Int_Snapshot()` 单次原子快照以及 `Bsp_Int_GetInt1Count()`、`Bsp_Int_GetInt2Count()`、`Bsp_Int_GetInt1Flag()` 等统一数据接口供业务层查询。

### 4.7 系统鲁棒性与性能深度优化设计 (Industrial-Grade Performance & Robustness)
- **SPI 寄存器级高速总线驱动**：`SPI1_WriteBuffer` 消除逐字节固件库函数调用开销，直接寄存器操作使高频刷屏效率倍增；
- **GUI 零冗余重绘 (Diff Change Detection)**：Page 2 数据控件仅在显示字符串发生实质变更时触发重绘，在静止/恒定状态下大幅节省 CPU 与 SPI 总线负载；
- **整型快速格式化**：避免标准浮点格式化（`dtoa`）的巨大运行时与堆栈开销，提高帧率响应；
- **软件 I2C 总线原子操作与防死锁**：SCL/SDA 引脚采用直接寄存器位带原子置位，`Soft_I2C_WaitAck()` 内置微秒延时与 400µs 宽容限超时保护，杜绝从机掉线或总线干扰导致的无限等待；
- **SPI 传输故障安全保护 (Failsafe)**：所有硬件 SPI 标志位（`TXE`、`BSY`）等待循环均加入 `SPI_TIMEOUT_COUNT` 边界计数器，防止因时钟异常造成单片机硬卡死；
- **USB CDC 临界区并发保护**：主线程 `SendBuffer` 与 USB 中断例程 `USBFS_IRQHandler` 操作环形缓冲区时，采用 `NVIC_DisableIRQ(USBFS_IRQn)` 临界区原子锁定，彻底杜绝数据乱序与重入冲突；
- **USB 多包控制传输状态机**：支持完整的控制读取分包机制，彻底消除 Windows 获取 67 字节 CDC 配置描述符时的截断缺陷（解决设备管理器代码 10 无法启动故障）。

---

## 5. 编译、烧录与资源占用

### 5.1 编译方式

#### 方式一：MounRiver Studio Ⅱ IDE 构建
1. 打开 MounRiver Studio Ⅱ，选择 `File -> Import -> General -> Existing Projects into Workspace`；
2. 浏览并选中本项目工程目录 `CH32X035F8U` 导入；
3. 点击工具栏 **Build** 按钮（或快捷键 `Ctrl + B`）即可自动一键编译。

#### 方式二：命令行编译
已配置标准 GCC 与 Make 构建系统，可直接在 `obj/` 目录执行：
```powershell
make clean
make all
```

### 5.2 资源占用情况 (RISC-V GCC rv32imacxw -Os)
```text
   text    data     bss     dec     hex filename
  41016     200    7376   48592    bdd0 CH32X035F8U.elf
```
- **Flash (Text + Data)**：**41.22 KB** / 62 KB (占用率约 **66.5%**)
- **SRAM (Data + BSS)**：**7.40 KB** / 20 KB (占用率约 **37.0%**，包含 WCHGUI 3KB 独立运行堆区与 USB CDC 缓冲区)
- 生成固件位于：
  - `obj/CH32X035F8U.elf`
  - `obj/CH32X035F8U.hex`

### 5.3 固件烧录指南

#### 免调试器烧录 (Type-C USB ISP 模式)
1. 准备一根 Type-C 数据线连接至电脑；
2. **按住板载 SW1 按键不放**，将 Type-C 线插入开发板（或按住 SW1 时按下 Reset）；
3. 打开官方烧录工具 **WCHISPTool**；
4. 芯片系列选择 **CH32X035**，下载方式选择 **USB**，软件将自动识别出芯片；
5. 在“用户程序文件”中载入 `obj/CH32X035F8U.hex`；
6. 点击 **“下载”**，烧录完成后松开 SW1 重新上电，屏幕即刻点亮并运行！

#### WCH-Link 仿真器烧录 (SWD 模式)
1. 将 WCH-Link 模块的 SWDIO、SWCLK、GND、3V3 分别连接至板载对应的测试点或 Type-C 引脚；
2. 在 MounRiver Studio Ⅱ 中点击工具栏 **Download** 按钮直接烧录并支持单步硬件仿真。

