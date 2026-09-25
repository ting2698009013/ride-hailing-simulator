#ifndef RIDEHAULING_SELF_TEST_H
#define RIDEHAULING_SELF_TEST_H

#include <iostream>
#include <string>

// =============================================================================
// SelfTest.h —— 手工数据结构与核心算法自检测试
// 启用方式：在 config.h 中定义 RUN_SELF_TEST 宏，或通过编译命令行定义。
// 测试框架：不依赖第三方测试库，使用简单的断言宏 + 控制台输出。
// =============================================================================

// 简单断言宏
#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::wcout << L"  [失败] " << msg << L" (行:" << __LINE__ << L")" << std::endl; \
            return false; \
        } \
    } while (0)

#define TEST_ASSERT_EQ(a, b, msg) \
    do { \
        if ((a) != (b)) { \
            std::wcout << L"  [失败] " << msg << L" 期望:" << (b) << L" 实际:" << (a) << L" (行:" << __LINE__ << L")" << std::endl; \
            return false; \
        } \
    } while (0)

class SelfTest {
public:
    // 运行所有测试，返回是否全部通过
    static bool runAll() {
        bool allPassed = true;

        std::wcout << L"\n--- 测试1: 单向链表 ---" << std::endl;
        if (!testLinkedList()) allPassed = false;

        std::wcout << L"\n--- 测试2: 循环队列 ---" << std::endl;
        if (!testCircularQueue()) allPassed = false;

        std::wcout << L"\n--- 测试3: 最小堆 ---" << std::endl;
        if (!testMinHeap()) allPassed = false;

        std::wcout << L"\n--- 测试4: 动态数组 ---" << std::endl;
        if (!testDynamicArray()) allPassed = false;

        std::wcout << L"\n--- 测试5: 坐标映射与边界 ---" << std::endl;
        if (!testCoordMapping()) allPassed = false;

        std::wcout << L"\n--- 测试6: 网格司机增删 ---" << std::endl;
        if (!testGridDriverOps()) allPassed = false;

        std::wcout << L"\n--- 测试7: 派单撮合 ---" << std::endl;
        if (!testDispatch()) allPassed = false;

        std::wcout << L"\n--- 测试8: 订单超时取消 ---" << std::endl;
        if (!testOrderTimeout()) allPassed = false;

        std::wcout << L"\n--- 测试9: 模拟器重置 ---" << std::endl;
        if (!testSimulatorReset()) allPassed = false;

        std::wcout << L"\n--- 测试10: 热点检测与调度 ---" << std::endl;
        if (!testHotspotReposition()) allPassed = false;

        return allPassed;
    }

private:
    static bool testLinkedList();
    static bool testCircularQueue();
    static bool testMinHeap();
    static bool testDynamicArray();
    static bool testCoordMapping();
    static bool testGridDriverOps();
    static bool testDispatch();
    static bool testOrderTimeout();
    static bool testSimulatorReset();
    static bool testHotspotReposition();
};

#endif // RIDEHAULING_SELF_TEST_H
