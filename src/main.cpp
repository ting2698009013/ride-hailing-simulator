// =============================================================================
// main.cpp —— 程序入口
// 职责：
//   1. 可选运行自检测试（通过编译宏 RUN_SELF_TEST 启用）。
//   2. 创建模拟器、初始化 EasyX 窗口、运行主循环。
// 主循环使用累计时间控制模拟 Tick，保持 UI 响应，不使用阻塞式 Sleep。
// =============================================================================

#include <iostream>
#include <string>
#include "config.h"
#include "core/Simulator.h"
#include "ui/EasyXRenderer.h"
#include <graphics.h>

// 自动链接 EasyX 库
#pragma comment(lib, "easyx.lib")

// ---- 自检测试 ----
// 定义在独立文件中，通过编译宏启用
#ifdef RUN_SELF_TEST
#include "tests/SelfTest.h"
#endif

int main() {
#ifdef RUN_SELF_TEST
    // 运行自检测试，输出到控制台
    std::wcout << L"========== 开始自检测试 ==========" << std::endl;
    bool allPassed = SelfTest::runAll();
    std::wcout << L"========== 自检测试结束："
              << (allPassed ? L"全部通过" : L"存在失败") << L" ==========" << std::endl;
    if (!allPassed) {
        std::wcout << L"自检测试未全部通过，请检查后重试。" << std::endl;
        return 1;
    }
    std::wcout << L"按回车键继续进入图形界面..." << std::endl;
    std::wcin.get();
#endif

    // 创建模拟器并初始化
    Simulator sim;
    sim.init(DEFAULT_SEED);
    sim.setSpeed(1.0);

    // 创建渲染器并初始化窗口
    EasyXRenderer renderer;
    renderer.initWindow();

    // 主循环
    bool running = true;
    while (running) {
        // 处理输入事件
        if (renderer.handleEvents(sim)) {
            running = false;
            break;
        }

        // 根据速度判断是否执行一个模拟 Tick
        if (!sim.isPaused() && renderer.shouldTick(sim.getSpeed())) {
            sim.tick();
        }

        // 渲染
        renderer.render(sim);

        // 短暂延时，避免 CPU 占用过高（不使用长 Sleep 阻塞）
        // 这里用 Sleep 控制帧率约 60fps，模拟逻辑由 shouldTick 独立控制
        Sleep(16);
    }

    closegraph();
    return 0;
}
