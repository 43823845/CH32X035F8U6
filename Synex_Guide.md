# Synex 上位机工程调试与数据监控适配指南

本项目（基于沁恒 **CH32X035F8U6**）已完整实现并内置了 **Synex**（原 EricTool）上位机支持的四大标准调试数据协议：**JustFloat**、**FireWater**、**CustomBinary** 与 **RawData**。

固件默认采用 **USB CDC 虚拟串口**（Type-C 接口免转接）全速传输数据，采样与刷新周期为 **100ms（10Hz 实时高速流）**。

---

## 1. 协议选型与对比分析（“哪种好用用那种”）

根据 Synex 官方技术规格与本项目 MCU（RISC-V QingKe V4C，32位小端架构）特点，各协议横向对比评测如下：

| 协议类型 | 数据载荷格式 | Synex 配置复杂度 | MCU 算力与 Flash 开销 | 传输吞吐与稳定性 | 适用场景与推荐度 |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **JustFloat** | 12 个小端 float32 + `0x7F800000` 尾 | **零配置**（直接选 JustFloat 即可） | **极低**（0字符串格式化，原生二进制直发） | 52 字节/帧，极低带宽，永不丢包 | ⭐⭐⭐⭐⭐ **【最强推荐】**：波形绘制、3D姿态监控、仪表盘首选 |
| **FireWater** | 英文逗号分隔的 ASCII CSV + `\n` | **零配置**（直接选 FireWater 即可） | 中等（需定点数转换打印） | 约 70~90 字节/帧，文本流 | ⭐⭐⭐⭐ **【通用推荐】**：兼顾 Synex 与第三方普通串口助手查看 |
| **CustomBinary** | `0x55 0xAA` 帧头 + 结构体 + Sum8 校验 + `\r\n` | 需导入协议 JSON 文件 | 低（结构体打包 + 累加和计算） | 34 字节/帧，带校验防误码 | ⭐⭐⭐⭐ **【高可靠性】**：严谨设备联调、工业通信与防丢包场景 |
| **RawData** | 结构化人类可读日志文本 | 无通道曲线解析 | 中等（`printf` 字符串输出） | 约 100 字节/帧 | ⭐⭐⭐ **【终端排查】**：仅用于控制台文本诊断，不生成曲线 |

> **结论与推荐**：
> 1. **在 Synex 中做实时示波器、3D 姿态展示、仪表观测时，`JustFloat` 最好用！**
>    - 零门槛：Synex 原生内置引擎，连上就出曲线，无需手动编辑任何协议。
>    - 高能效：MCU 内部浮点直接内存拷贝通过 USB 发送，CPU 占用率趋近于 0%，固件体积最小。
> 2. **如果希望同时在常规串口调试助手（如 SSCOM、Putty）中用肉眼看明文数字，选择 `FireWater`**。
> 3. **需要严格数据帧同步与校验时，选择 `CustomBinary`**（本项目已附带一键导入 JSON）。

---

## 2. 通道信号映射表（Channel Mapping）

下位机无论是 `JustFloat`、`FireWater` 还是 `CustomBinary`，均统一映射以下 12 个核心数据通道：

| 通道号 | 信号英文标号 | 信号中文名称 | 物理单位 | 典型范围 | 推荐 Synex 工作台绑定控件 |
| :---: | :--- | :--- | :---: | :---: | :--- |
| **CH0** | `lux` | 环境光照强度 | `Lux` | 0 ~ 65535 | 仪表控件（Gauge） / 实时绘图（Plot） |
| **CH1** | `pitch` | 俯仰姿态角 (Pitch) | `°` (deg) | -90 ~ +90 | **3D 姿态控件 (Attitude Tile)** |
| **CH2** | `roll` | 横滚姿态角 (Roll) | `°` (deg) | -180 ~ +180 | **3D 姿态控件 (Attitude Tile)** |
| **CH3** | `acc_x` | X 轴加速度 | `mg` | -2000 ~ +2000 | 实时绘图控件（曲线示波器） |
| **CH4** | `acc_y` | Y 轴加速度 | `mg` | -2000 ~ +2000 | 实时绘图控件（曲线示波器） |
| **CH5** | `acc_z` | Z 轴加速度 | `mg` | -2000 ~ +2000 | 实时绘图控件（曲线示波器） |
| **CH6** | `backlight` | 屏幕背光占空比 | `%` | 0 ~ 100 | 仪表控件 / 数值控件 |
| **CH7** | `tap_single` | 单击敲击计数 | 次 | 0 ~ 65535 | 数值控件（Numeric Tile） |
| **CH8** | `tap_double` | 双击敲击计数 | 次 | 0 ~ 65535 | 数值控件（Numeric Tile） |
| **CH9** | `free_fall` | 自由落体检测计数 | 次 | 0 ~ 255 | 数值控件（报警状态指示） |
| **CH10** | `int1_count` | LIS2DW12 INT1 计数 | 次 | 0 ~ 65535 | 数值控件 |
| **CH11** | `int2_count` | LTR-308 INT2 计数 | 次 | 0 ~ 65535 | 数值控件 |

---

## 3. 四大协议帧结构技术规格

### 3.1 JustFloat 高速浮点协议（推荐默认）
- **单片机端结构体**：
  ```c
  typedef struct __attribute__((packed)) {
      float lux;          // CH0: 环境光强 (lx)
      float pitch;        // CH1: 俯仰角 (deg)
      float roll;         // CH2: 横滚角 (deg)
      float acc_x;        // CH3: X 轴加速度 (mg)
      float acc_y;        // CH4: Y 轴加速度 (mg)
      float acc_z;        // CH5: Z 轴加速度 (mg)
      float backlight;    // CH6: 背光亮度 (%)
      float tap_single;   // CH7: 单击累计
      float tap_double;   // CH8: 双击累计
      float free_fall;    // CH9: 自由落体累计
      float int1_count;   // CH10: INT1 中断计数
      float int2_count;   // CH11: INT2 中断计数
      uint32_t tail;      // 0x7F800000 (Little-Endian: 00 00 80 7F)
  } Debug_JustFloatFrame_t;
  ```
- **帧总长度**：52 字节。
- **Synex 设置**：连接页选择 `JustFloat`，无需任何配置，自动解析 CH0 ~ CH11。

### 3.2 FireWater 文本 CSV 协议
- **数据帧格式**：单行 ASCII 纯文本，通道值之间用英文逗号 `,` 分隔，行尾以 `\n` 换行。
- **输出范例**：
  ```text
  128.5,12.3,-4.5,-45,82,998,100,3,1,0,120,45\n
  ```
- **Synex 设置**：连接页选择 `FireWater`，自动按逗号解析多通道。

### 3.3 CustomBinary 自定义二进制协议（严格对齐 Synex Protocol3 / CH32X035_Monitor）
- **帧总长度**：30 字节。
- **帧结构排布**：
  `[0xAA 0x55 (2B)][Payload (25B)][Sum8 Checksum (1B)][0x0D 0x0A (2B)]`
  - `Payload` 包括：
    - `float lux`（4 字节小端浮点，光强）
    - `int16_t p, r`（各 2 字节小端有符号整数，俯仰角、横滚角）
    - `int16_t x, y, z`（各 2 字节小端有符号整数，三轴加速度 mg）
    - `int8_t bl`（1 字节，背光占空比 %）
    - `int8_t tap1, tap2`（各 1 字节，单击、双击计数）
    - `int8_t ff1, ff2`（各 1 字节，自由落体计数、近期跌落状态）
    - `int16_t int1`（2 字节小端，INT1 计数）
    - `int32_t int2`（4 字节小端，INT2 计数）
  - `Checksum`：对 25 字节 Payload 计算 Sum8 累加和。
- **Synex 设置**：通过 Synex MCP 自动完成协议同步并已发布，在 Synex 连接页直接下拉选择 **`CH32X035_Monitor`** 或 **`Protocol3`** 即可！

### 3.4 RawData 调试日志文本
- **输出范例**：
  ```text
  [DEBUG] Lux:128.5 | P:12 R:-4 | X:-45 Y:82 Z:998 | BL:100% | TAP:3/1 FF:0(0) | INT1:120 INT2:45
  ```
- **Synex 设置**：连接页选择 `RawData`，在 Synex “控制台”或串口监视区观察原始日志。

---

## 4. 上位机 Synex 连接实操步骤

### 第一步：物理连接与串口识别
1. 将板载 Type-C 接口连接至电脑 USB 口。
2. 固件自带 USB CDC 虚拟串口驱动，电脑设备管理器将识别到串口设备（如 `COM3` 或 `CH32X035 USB CDC`）。
3. 波特率建议设置为 `115200`（由于走的是硬件全速 USB 虚拟串口，波特率设置任意值均可全速通信）。

### 第二步：Synex 连接参数配置
1. 打开 **Synex**，进入左侧导航栏的 **「连接」** 页面。
2. 数据引擎选择 **「串口」**。
3. 参数设置：
   - **端口**：选择对应的 COM 口（例如 `COM3`）。
   - **波特率**：`115200`（数据位 8，校验 None，停止位 1）。
   - **解析协议**：选择 **`JustFloat`**（或 `FireWater`）。
4. 点击右上角 **「连接开关」**，状态变为已连接，通道列表中将立即出现 CH0 到 CH11 实时变动的数值！

---

## 5. Synex 个人工作台快速搭建（推荐配置）

进入 Synex 的 **「个人工作台」** 页面，右键空白处即可添加以下专业监控控件：

### 控件 1：3D 姿态模型（Attitude Widget）
- **添加方式**：右键添加「姿态控件」。
- **数据绑定**：
  - **Pitch（俯仰角）**：绑定 **CH1**。
  - **Roll（横滚角）**：绑定 **CH2**。
- **效果**：拿起开发板晃动或翻转，Synex 屏幕中的 3D 飞机/立方体模型将毫秒级跟随开发板实时姿态旋转！

### 控件 2：多通道加速度与光强示波器（Plot Widget）
- **添加方式**：右键添加「绘图控件」。
- **通道配置**：
  - 勾选 **CH3 (AccX)**、**CH4 (AccY)**、**CH5 (AccZ)**，Y 轴范围设置 -2000 ~ +2000 mg。
  - 勾选 **CH0 (Lux)**，开启动态 Y 轴。
- **效果**：轻敲桌面或摇晃板子，可观察到清晰的三轴加速度阶跃与震动波形；手遮挡光敏传感器，可观察到光照突降曲线。

### 控件 3：环境光强与背光仪表盘（Gauge Widget）
- **添加方式**：右键添加「仪表控件」。
- **数据绑定**：
  - 仪表 1：绑定 **CH0 (Lux)**，量程 0 ~ 1000 Lux。
  - 仪表 2：绑定 **CH6 (Backlight)**，量程 0 ~ 100%。

### 控件 4：手势敲击与中断事件面板（Numeric / Console Widget）
- **添加方式**：右键添加「数值控件」。
- **数据绑定**：
  - 绑定 **CH7 (单击计数)** 与 **CH8 (双击计数)**。
  - 绑定 **CH9 (自由落体计数)**。
- **效果**：用手指轻敲开发板，即可看到单击/双击计数器立即自增。

---

## 6. 运行中动态切换协议（指令台交互）

本项目内置了动态协议解析切换引擎。即使系统在运行中，你也可以在 **Synex 的「指令台」**、串口终端或控制台中发送单字符指令完成协议热切换，无需重新烧录固件：

| 发送字符 | 切换目标协议 | 下位机响应回显 |
| :---: | :--- | :--- |
| `1` 或 `J` | 切换为 **JustFloat** | `[PROTO] -> JustFloat (Synex High-Speed)` |
| `2` 或 `F` | 切换为 **FireWater** | `[PROTO] -> FireWater (CSV Stream)` |
| `3` 或 `C` | 切换为 **CustomBinary** | `[PROTO] -> CustomBinary (0x55 0xAA Frame)` |
| `4` 或 `R` | 切换为 **RawData** | `[PROTO] -> RawData (Text Log)` |
| `?` | 查询当前协议与帮助 | `[PROTO] Cur: ... \| Keys: 1=JustFloat 2=FireWater 3=CustomBinary 4=Raw` |

---

## 7. 常见问题排查（FAQ）

1. **连接后没有波形刷新？**
   - 检查 Synex 连接页的解析协议是否与下位机当前协议匹配（默认下位机为 `JustFloat`）。
   - 若不确定，在 Synex 串口控制台发送字符 `1`，下位机将强制切换为 `JustFloat` 模式。
2. **为什么推荐 JustFloat 而不是直接 printf 浮点数？**
   - 单片机使用 `printf` 浮点数需要链接大型浮点格式化库（占用 5~10KB Flash），而在 62KB Flash 限制下极易导致代码段溢出。
   - `JustFloat` 直接发送 IEEE 754 内存原值，零运算开销、零精度损失、高刷新率，是工控与姿态解算的工业标准。

---

## 8. Synex MCP Server 协同集成与自动化工作流

本项目已将 Synex 官方 MCP 工具服务端集成至 Antigravity MCP 配置：
- **全局配置**：`~/.gemini/config/mcp_config.json`
- **项目配置**：`.agents/mcp_config.json`

```json
{
  "mcpServers": {
    "synex": {
      "command": "D:\\Synex_v4.0.1\\mcp\\SynexMcpServer.exe"
    }
  }
}
```

通过 MCP Server，AI 助手已与运行中的 Synex 实时互通，完成以下闭环操作：
1. **协议结构解析与校验**：调用 `validate_protocol_json` 验证协议配置语法与字段属性；
2. **工作区状态对齐**：调用 `get_protocol_editor_workspace` 读取 Synex 当前协议列表；
3. **下位机固件代码生成验证**：调用 `generate_protocol_firmware_code` 提取严格匹配的 C 语言 struct 定义；
4. **协议发布并同步连接页**：通过 `publish_protocol_editor_protocol` 自动发布 `CH32X035_Monitor` 协议；
5. **帧布局分析**：调用 `analyze_protocol_frame_layout` 确认 30 字节帧各字段偏移与校验和计算范围。

