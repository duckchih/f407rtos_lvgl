# STM32F407 + FreeRTOS + LVGL 综合Demo项目

**[chat.deepseek.com/share/nr3bb39yqveuwg0vb9](https://chat.deepseek.com/share/nr3bb39yqveuwg0vb9)让deepseek带着你学**

本项目是一个基于 **STM32F407** 微控制器的完整嵌入式应用开发实践，从底层外设驱动到上层图形界面（GUI）完全手写并采用分层架构设计。项目放弃了传统的 Keil/IAR 等 IDE，完全基于 **CMake + Ninja + VSCode** 进行构建与开发，高度还原现代企业级底层软件的工程规范。

## 🛠️ 技术栈

- **硬件平台**：STM32F407 (Cortex-M4F, 168MHz)
- **显示外设**：1.47寸 SPI TFT 屏幕 (ST7789V3, 172x320)
- **实时操作系统**：FreeRTOS V10.5.1 (LTS)
- **GUI 框架**：LVGL V8.3
- **构建系统**：CMake + Ninja + ARM GNU Toolchain (xPack)
- **开发环境**：VSCode + Cortex-Debug + OpenOCD

## ✨ 核心功能与技术亮点

### 1. 软件架构解耦设计

项目采用了 **BSP / App / ThirdParty / Core** 四层架构。业务逻辑（App）、板级硬件驱动（BSP）与底层库（RTOS/LVGL）完全隔离。更换 MCU 或屏幕时，上层业务代码无需修改。

### 2. 底层驱动开发 (BSP)

- **SPI LCD 驱动**：手写 ST7789V3 寄存器初始化序列，处理了 172x320 物理显存偏移量问题。
- **DMA 双工通信**：串口收发全面采用 **DMA + 中断** 模式，实现了零 CPU 干预的高速数据传输。
- **IDLE 空闲中断**：接收端结合 DMA Circular 模式与 IDLE 中断，解决串口不定长数据的断帧解析问题。

### 3. RTOS 多任务协同

- **生产者-消费者模型**：基于 FreeRTOS Queue 实现任务间通信。LED 任务与串口接收任务作为生产者，GUI 任务作为消费者，安全地解耦并发操作。
- **并发控制（优先级反转实验）**：使用 Mutex 验证并解决了多任务调度中的优先级反转（Priority Inversion）问题，激活了内核的优先级继承机制。
- **系统状态监控**：利用 10kHz 硬件定时器（TIM2）与 FreeRTOS Trace 机制，实时计算并显示各任务的 **CPU 占用率** 与 **Stack 剩余高水位线**。

### 4. 系统可靠性与持久化

- **独立看门狗（IWDG）**：设计低优先级监控任务。关键业务任务周期性打卡，若系统发生死锁，看门狗将在 2 秒内强制复位系统。
- **Flash 掉电存储**：利用内部 Flash 实现关键业务数据（如计数）掉电保存。采用异步队列写盘与阈值判断机制，规避 Flash 擦除导致的总线阻塞（Bus Stall），有效延长 Flash 寿命。

## 📁 工程目录结构

```text
f407rtos_lvgl/
├── App/            # 应用层（业务逻辑、UI构建、任务创建）
├── BSP/            # 板级支持包（手写的 LCD、UART 驱动等）
├── ThirdParty/     # 第三方库（FreeRTOS, LVGL）
├── Core/           # CubeMX生成的底层硬件初始化代码
├── cmake/          # 交叉编译工具链配置
└── CMakeLists.txt  # 顶层构建脚本
```

```Shell
# 编译命令
cmake --preset Debug
cmake --build build/Debug
# 烧录命令
openocd -f openocd.cfg -c "program build/Debug/f407rtos_lvgl.elf verify reset exit"
```
