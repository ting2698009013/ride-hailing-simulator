# 智能网约车平台区域实时分布式派单与动态运力调度模拟系统

[![core-tests](https://github.com/ting2698009013/ride-hailing-simulator/actions/workflows/core-tests.yml/badge.svg)](https://github.com/ting2698009013/ride-hailing-simulator/actions/workflows/core-tests.yml)

## 项目简介

本项目是同济大学《数据结构与算法设计》课程设计题目3的实现。系统模拟网约车平台的派单与运力调度逻辑：在 1000×1000 的虚拟城市坐标系中划分 100×100 网格，手工实现单向链表、循环队列、最小堆等核心数据结构，完成空间索引、批量派单、供需失衡检测和跨网格运力调度，并通过 EasyX 图形界面实时可视化供需热力图、派单日志和全局统计指标。

## 功能列表

- **空间索引**：100×100 网格矩阵，O(1) 坐标定位，每个网格维护空闲司机链表
- **批量派单**：每模拟秒从等待队列取订单，周边网格搜索候选司机，最小堆撮合最优匹配
- **撮合公式**：基础模式 `Score = gridDistance - rating`；路况增强模式 `Score = gridDistance × trafficCoef - rating`
- **状态机**：司机 IDLE→ASSIGNED→SERVING→IDLE；订单 WAITING→MATCHED→COMPLETED / CANCELLED
- **供需失衡检测**：每秒统计 `imbalance = waiting - idle`，超阈值标记警报热点
- **动态运力调度**：从周边富余网格调度司机到热点，遵守最低保留量
- **EasyX 可视化**：热力图、司机/订单点、调度连线、实时看板、滚动日志
- **交互控制**：暂停/继续、单步、重置、调速、切换热点/撮合模式、点击查看网格详情、手动加单/加车

## 目录结构

```
RideHailingSimulator/
├─ RideHailingSimulator.sln          # VS2022 解决方案
├─ RideHailingSimulator.vcxproj      # 项目文件
├─ RideHailingSimulator.vcxproj.filters
├─ src/
│  ├─ main.cpp                       # 程序入口
│  ├─ config.h                       # 全局常量配置
│  ├─ models/
│  │  ├─ Driver.h                    # 司机模型
│  │  └─ Order.h                     # 订单模型
│  ├─ structures/                    # 手工数据结构（核心）
│  │  ├─ SinglyLinkedList.h          # 单向链表（网格空闲司机）
│  │  ├─ CircularQueue.h             # 循环队列（等待订单）
│  │  ├─ MinHeap.h                   # 最小堆（撮合候选司机）
│  │  └─ DynamicArray.h             # 动态数组（可变长度存储）
│  ├─ core/
│  │  ├─ CityGrid.h / .cpp           # 城市网格空间索引
│  │  ├─ DispatchEngine.h / .cpp     # 派单引擎（最小堆撮合）
│  │  ├─ Simulator.h / .cpp          # 模拟器（订单生成/状态机/调度）
│  │  └─ Logger.h                    # 日志器（循环缓冲区）
│  ├─ ui/
│  │  ├─ EasyXRenderer.h / .cpp      # EasyX 图形界面
│  └─ tests/
│     ├─ SelfTest.h                  # 自检测试声明
│     └─ SelfTest.cpp                # 自检测试实现
├─ docs/
│  ├─ design.md                      # 设计文档
│  ├─ test_cases.md                  # 测试用例
│  ├─ report_material.md             # 报告素材
│  └─ video_script.md                # 录屏提纲
└─ README.md                         # 本文件
```

## 环境要求

- **开发工具**：Visual Studio 2022（v143 工具集）
- **C++ 标准**：C++17
- **字符集**：Unicode（`/utf-8` 编译选项）
- **图形库**：EasyX（支持 VS2022 的版本，推荐 2022 及以上）
- **操作系统**：Windows 10/11

> 验证说明：项目源代码已在 Visual Studio 2022 工具链下完成编译检查；当前验证环境未安装 EasyX，因此最终链接会停在缺少 `easyx.lib`。安装下述 EasyX 依赖后即可完成链接。

## EasyX 安装步骤

1. 前往 EasyX 官网 https://easyx.cn 下载最新版 EasyX 安装包
2. 运行安装程序，安装程序会自动检测本机已安装的 Visual Studio
3. 勾选你的 Visual Studio 2022，点击安装
4. 安装完成后，EasyX 会自动将 `graphics.h` 头文件和 `easyx.lib` 库文件放入 VS 的对应目录
5. 验证：新建一个空项目，写 `#include <graphics.h>` 并编译，不报错即安装成功

## 打开、编译和运行步骤

1. 双击 `RideHailingSimulator.sln` 用 Visual Studio 2022 打开解决方案
2. 在顶部工具栏选择配置：推荐 `Debug | x64` 或 `Release | x64`
3. 按 `Ctrl + F5`（开始执行不调试）或 `F5`（开始调试）编译并运行
4. 编译成功后会自动弹出图形窗口

### 运行无界面核心自检

仓库提供独立的 CMake 测试目标，不需要安装 EasyX。它覆盖链表、循环队列、最小堆、动态数组、网格索引、订单匹配、超时取消、模拟器重置和热点调度：

```powershell
cmake -S . -B build
cmake --build build --config Release
.\build\Release\ride_hailing_tests.exe
```

在 Linux 或单配置生成器下，最后一条命令为 `./build/ride_hailing_tests`。GitHub Actions 会在每次推送和 Pull Request 时构建并运行同一组测试。

## 控制按钮及快捷键

### 按钮操作（地图右侧控制区）

| 按钮 | 功能 |
|------|------|
| 暂停/继续 | 暂停或恢复模拟 |
| 单步执行 | 暂停状态下执行一个模拟秒 |
| 重置模拟 | 重置到初始状态（保留随机种子） |
| 减速 / 加速 | 在 0.5× / 1× / 2× / 5× 之间切换速度 |
| 热点: 开/关 | 开启或关闭热点订单生成 |
| 模式: 基础/路况 | 切换撮合代价计算公式 |
| 清空日志 | 清除日志区域 |
| 显示/隐藏点 | 显示或隐藏司机和订单小点 |

### 鼠标操作

| 操作 | 功能 |
|------|------|
| 左键点击地图 | 选中网格，右上角显示该网格详情 |
| 右键点击地图 | 在该网格手动增加一个订单 |

### 键盘快捷键

| 按键 | 功能 |
|------|------|
| `空格` | 暂停/继续 |
| `S` | 单步执行（暂停状态下） |
| `R` | 重置模拟 |
| `1` | 在选中网格手动增加订单 |
| `2` | 在选中网格手动增加空闲司机 |
| `ESC` | 退出程序 |

## 核心数据结构说明

| 数据结构 | 文件 | 用途 | 关键操作复杂度 |
|---------|------|------|--------------|
| 单向链表 | `SinglyLinkedList.h` | 每个网格的空闲司机链表 | 头插 O(1)，按编号删除 O(n) |
| 循环队列 | `CircularQueue.h` | 等待派单的订单队列 | 入队/出队 O(1) |
| 最小堆 | `MinHeap.h` | 撮合候选司机，堆顶为最优 | 插入/弹出 O(log n) |
| 动态数组 | `DynamicArray.h` | 司机池、订单池等可变长度存储 | 追加均摊 O(1)，访问 O(1) |
| 循环缓冲区 | `Logger.h` | 最近 30 条日志 | 写入 O(1) |

所有数据结构均为**手工实现**，未使用 `std::vector`、`std::list`、`std::queue`、`std::priority_queue` 等 STL 容器。仅使用了 `std::string`/`std::wstring` 和 `<iostream>`/`<fstream>` 等基础设施。

## 派单成功率计算方式

```
派单成功率 = 派单成功数 / (派单成功数 + 取消订单数) × 100%
```

其中：
- **派单成功数**：曾经成功匹配到司机的订单总数
- **取消订单数**：因等待超时（>15秒）被乘客取消的订单数

该指标反映了系统在给定运力下的服务能力。未匹配且未超时的订单不计入分母，因为它们仍在等待中，最终结果未知。

## 已知限制

1. **无真实网络通信**："分布式"通过网格区域独立管理和批量数据流调度模拟，不涉及真正的多机部署或网络通信
2. **无平滑移动动画**：司机位置变化是瞬时的，调度仅绘制连线，不实现车辆移动动画
3. **单线程运行**：为保证课程设计稳定性，不使用多线程，主循环通过时间累计控制 Tick 频率
4. **订单队列容量固定**：循环队列容量为 8192，极端情况下可能丢弃订单（会记录日志）
5. **EasyX 依赖**：必须在安装了 EasyX 的 Windows 环境下才能编译运行
6. **热点配置固定**：3 个热点区域位置在 `config.h` 中硬编码，运行时不可修改

## 常见编译错误处理

### 1. 找不到 `graphics.h`

**原因**：未安装 EasyX 或 EasyX 未正确安装到 VS2022。

**解决**：
- 确认已从 https://easyx.cn 下载并安装 EasyX
- 安装时确保勾选了 Visual Studio 2022
- 安装后重启 Visual Studio

### 2. 链接错误 `LNK2019: 无法解析的外部符号 ... easyx`

**原因**：EasyX 库未链接。

**解决**：
- 确认项目代码中包含 `#pragma comment(lib, "easyx.lib")`（已在 `main.cpp` 和 `EasyXRenderer.cpp` 中添加）
- 如仍失败，手动在项目属性 → 链接器 → 输入 → 附加依赖项中添加 `easyx.lib`

### 3. 中文乱码

**原因**：源文件编码或编译字符集不匹配。

**解决**：
- 确认项目使用 `/utf-8` 编译选项（已在 vcxproj 中配置）
- 确认源文件以 UTF-8 编码保存
- 确认项目字符集设置为 Unicode（已在 vcxproj 中配置）

### 4. `error C2664: 无法转换参数` 等 EasyX 类型错误

**原因**：EasyX 版本差异。

**解决**：本项目基于 EasyX 2022 版本 API 编写，如使用其他版本可能需要微调部分 API 调用。

### 5. x86 (Win32) 编译报错

**原因**：部分 EasyX 版本的 x86 库可能不完整。

**解决**：推荐使用 `x64` 配置编译运行。
