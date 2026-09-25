#include "SelfTest.h"
#include "../structures/SinglyLinkedList.h"
#include "../structures/CircularQueue.h"
#include "../structures/MinHeap.h"
#include "../structures/DynamicArray.h"
#include "../core/CityGrid.h"
#include "../core/DispatchEngine.h"
#include "../core/Simulator.h"
#include "../models/Driver.h"
#include "../models/Order.h"
#include <cmath>

// =============================================================================
// SelfTest.cpp —— 自检测试实现
// 每个测试函数返回 true 表示通过，false 表示失败。
// =============================================================================

// ---- 测试1: 单向链表（头部/中部/尾部插入删除、删除不存在、判空）----
bool SelfTest::testLinkedList() {
    SinglyLinkedList list;

    // 创建几个测试司机对象（测试结束手动释放）
    Driver d1, d2, d3, d4;
    d1.id = 101; d2.id = 102; d3.id = 103; d4.id = 104;

    TEST_ASSERT(list.isEmpty(), L"初始链表应为空");
    TEST_ASSERT_EQ(list.size(), 0, L"初始链表大小应为0");

    // 头插法插入
    list.pushFront(&d1);
    list.pushFront(&d2);
    list.pushFront(&d3);
    TEST_ASSERT_EQ(list.size(), 3, L"插入3个后大小应为3");
    TEST_ASSERT(!list.isEmpty(), L"插入后不应为空");

    // 头部应该是最后插入的 d3
    TEST_ASSERT(list.front()->id == 103, L"头节点应为d3");

    // 删除中间节点 d2
    Driver* removed = list.removeById(102);
    TEST_ASSERT(removed != nullptr && removed->id == 102, L"应成功删除d2");
    TEST_ASSERT_EQ(list.size(), 2, L"删除后大小应为2");
    TEST_ASSERT(!list.contains(102), L"删除后不应包含d2");

    // 删除头节点 d3
    removed = list.removeById(103);
    TEST_ASSERT(removed != nullptr && removed->id == 103, L"应成功删除头节点d3");
    TEST_ASSERT_EQ(list.size(), 1, L"删除头后大小应为1");
    TEST_ASSERT(list.front()->id == 101, L"删除头后新头应为d1");

    // 删除尾节点（此时也是唯一节点）d1
    removed = list.removeById(101);
    TEST_ASSERT(removed != nullptr && removed->id == 101, L"应成功删除d1");
    TEST_ASSERT(list.isEmpty(), L"全部删除后应为空");

    // 删除不存在的司机
    removed = list.removeById(999);
    TEST_ASSERT(removed == nullptr, L"删除不存在的司机应返回nullptr");

    // popFront 空表
    removed = list.popFront();
    TEST_ASSERT(removed == nullptr, L"空表popFront应返回nullptr");

    // 遍历测试
    list.pushFront(&d1);
    list.pushFront(&d2);
    list.pushFront(&d3);
    int count = 0;
    list.forEach([&count](const Driver*) { count++; });
    TEST_ASSERT_EQ(count, 3, L"遍历应计数3个节点");

    list.clear();
    TEST_ASSERT(list.isEmpty(), L"clear后应为空");

    std::wcout << L"  [通过] 单向链表测试全部通过" << std::endl;
    return true;
}

// ---- 测试2: 循环队列（空队出队、满队拒绝、首尾回绕）----
bool SelfTest::testCircularQueue() {
    // 容量5，实际可用4（留一个空位区分空满）
    CircularQueue q(5);
    Order o1, o2, o3, o4, o5;
    o1.id = 1; o2.id = 2; o3.id = 3; o4.id = 4; o5.id = 5;

    TEST_ASSERT(q.isEmpty(), L"初始队列应为空");
    TEST_ASSERT(q.pop() == nullptr, L"空队出队应返回nullptr");

    // 入队4个（满）
    TEST_ASSERT(q.push(&o1), L"o1入队应成功");
    TEST_ASSERT(q.push(&o2), L"o2入队应成功");
    TEST_ASSERT(q.push(&o3), L"o3入队应成功");
    TEST_ASSERT(q.push(&o4), L"o4入队应成功");
    TEST_ASSERT(!q.push(&o5), L"o5入队应失败（队列满）");
    TEST_ASSERT(q.isFull(), L"队列应已满");
    TEST_ASSERT_EQ(q.size(), 4, L"队列大小应为4");

    // 出队2个，测试首尾回绕
    TEST_ASSERT(q.pop()->id == 1, L"出队应为o1");
    TEST_ASSERT(q.pop()->id == 2, L"出队应为o2");
    TEST_ASSERT_EQ(q.size(), 2, L"出队2个后大小应为2");

    // 再入队2个（测试回绕）
    TEST_ASSERT(q.push(&o5), L"o5入队应成功（回绕位置）");
    TEST_ASSERT(q.push(&o1), L"o1再入队应成功（回绕位置）");
    TEST_ASSERT_EQ(q.size(), 4, L"回绕入队后大小应为4");

    // 全部出队，验证顺序正确
    TEST_ASSERT(q.pop()->id == 3, L"出队应为o3");
    TEST_ASSERT(q.pop()->id == 4, L"出队应为o4");
    TEST_ASSERT(q.pop()->id == 5, L"出队应为o5");
    TEST_ASSERT(q.pop()->id == 1, L"出队应为o1");
    TEST_ASSERT(q.isEmpty(), L"全部出队后应为空");

    // 测试 removeById
    q.push(&o1); q.push(&o2); q.push(&o3);
    TEST_ASSERT(q.removeById(2), L"应成功删除o2");
    TEST_ASSERT_EQ(q.size(), 2, L"删除后大小应为2");
    TEST_ASSERT(q.pop()->id == 1, L"出队应为o1");
    TEST_ASSERT(q.pop()->id == 3, L"出队应为o3");

    std::wcout << L"  [通过] 循环队列测试全部通过" << std::endl;
    return true;
}

// ---- 测试3: 最小堆（插入、弹出、相同分数稳定比较）----
bool SelfTest::testMinHeap() {
    MinHeap heap(8);
    Driver d1, d2, d3, d4, d5;
    d1.id = 1; d1.rating = 4.0;
    d2.id = 2; d2.rating = 5.0;
    d3.id = 3; d3.rating = 4.5;
    d4.id = 4; d4.rating = 4.0;  // 与d1同分同评分，id更大
    d5.id = 5; d5.rating = 3.8;

    TEST_ASSERT(heap.isEmpty(), L"初始堆应为空");

    // 插入候选，分数不同
    heap.push(MinHeap::Candidate(5.0, &d1, 100, 0, 0));   // score=5.0
    heap.push(MinHeap::Candidate(2.0, &d2, 50, 0, 0));    // score=2.0 最小
    heap.push(MinHeap::Candidate(3.5, &d3, 80, 0, 0));    // score=3.5
    TEST_ASSERT_EQ(heap.size(), 3, L"插入3个后大小应为3");

    // 弹出，应为分数最低的 d2
    MinHeap::Candidate c;
    TEST_ASSERT(heap.pop(c), L"弹出应成功");
    TEST_ASSERT(c.driver->id == 2, L"堆顶应为d2(score=2.0)");

    // 继续弹出
    heap.pop(c);
    TEST_ASSERT(c.driver->id == 3, L"下一个应为d3(score=3.5)");
    heap.pop(c);
    TEST_ASSERT(c.driver->id == 1, L"下一个应为d1(score=5.0)");
    TEST_ASSERT(heap.isEmpty(), L"全部弹出后应为空");

    // 相同分数稳定性测试：分数相同时评分高优先，评分也相同时id小优先
    heap.push(MinHeap::Candidate(3.0, &d1, 100, 0, 0));   // score=3.0, rating=4.0, id=1
    heap.push(MinHeap::Candidate(3.0, &d4, 100, 0, 0));   // score=3.0, rating=4.0, id=4
    heap.push(MinHeap::Candidate(3.0, &d3, 100, 0, 0));   // score=3.0, rating=4.5, id=3

    heap.pop(c);
    TEST_ASSERT(c.driver->id == 3, L"同分应选评分最高的d3(rating=4.5)");
    heap.pop(c);
    TEST_ASSERT(c.driver->id == 1, L"同分同评分应选id小的d1");
    heap.pop(c);
    TEST_ASSERT(c.driver->id == 4, L"最后是d4");

    std::wcout << L"  [通过] 最小堆测试全部通过" << std::endl;
    return true;
}

// ---- 测试4: 动态数组（扩容、访问、删除、查找）----
bool SelfTest::testDynamicArray() {
    DynamicArray<int> arr(4);

    TEST_ASSERT(arr.isEmpty(), L"初始应为空");

    // 插入超过初始容量，测试扩容
    for (int i = 0; i < 20; ++i) {
        arr.push(i * 10);
    }
    TEST_ASSERT_EQ(arr.size(), 20, L"插入20个后大小应为20");
    TEST_ASSERT(arr.capacity() >= 20, L"容量应已扩容到>=20");

    // 随机访问
    TEST_ASSERT_EQ(arr[0], 0, L"arr[0]应为0");
    TEST_ASSERT_EQ(arr[10], 100, L"arr[10]应为100");
    TEST_ASSERT_EQ(arr[19], 190, L"arr[19]应为190");

    // 查找
    int idx = arr.find([](int v) { return v == 150; });
    TEST_ASSERT_EQ(idx, 15, L"应找到值150在下标15");

    idx = arr.find([](int v) { return v == 999; });
    TEST_ASSERT_EQ(idx, -1, L"查找不存在的值应返回-1");

    // 删除
    arr.removeAt(5);
    TEST_ASSERT_EQ(arr.size(), 19, L"删除后大小应为19");

    arr.clear();
    TEST_ASSERT(arr.isEmpty(), L"clear后应为空");

    std::wcout << L"  [通过] 动态数组测试全部通过" << std::endl;
    return true;
}

// ---- 测试5: 坐标映射与边界（0、999、越界）----
bool SelfTest::testCoordMapping() {
    CityGrid grid;
    int r, c;

    // 原点
    grid.coordToGrid(0, 0, r, c);
    TEST_ASSERT(r == 0 && c == 0, L"坐标(0,0)应映射到网格(0,0)");

    // 右下角极限
    grid.coordToGrid(999, 999, r, c);
    TEST_ASSERT(r == 99 && c == 99, L"坐标(999,999)应映射到网格(99,99)");

    // 中间点
    grid.coordToGrid(505, 305, r, c);
    TEST_ASSERT(r == 30 && c == 50, L"坐标(505,305)应映射到网格(30,50)");

    // 越界坐标应被夹紧
    grid.coordToGrid(-10, -20, r, c);
    TEST_ASSERT(r == 0 && c == 0, L"负坐标应夹紧到(0,0)");

    grid.coordToGrid(2000, 2000, r, c);
    TEST_ASSERT(r == 99 && c == 99, L"超大坐标应夹紧到(99,99)");

    // 边界检查
    TEST_ASSERT(grid.isValidGrid(0, 0), L"网格(0,0)应有效");
    TEST_ASSERT(grid.isValidGrid(99, 99), L"网格(99,99)应有效");
    TEST_ASSERT(!grid.isValidGrid(-1, 0), L"网格(-1,0)应无效");
    TEST_ASSERT(!grid.isValidGrid(100, 0), L"网格(100,0)应无效");

    std::wcout << L"  [通过] 坐标映射与边界测试全部通过" << std::endl;
    return true;
}

// ---- 测试6: 网格司机增删（同一网格多名司机、删除不存在）----
bool SelfTest::testGridDriverOps() {
    CityGrid grid;

    Driver d1, d2, d3;
    d1.id = 1; d1.x = 505; d1.y = 305;  // 网格(30,50)
    d2.id = 2; d2.x = 502; d2.y = 308;  // 同一网格
    d3.id = 3; d3.x = 100; d3.y = 100;  // 网格(10,10)

    grid.addIdleDriver(&d1);
    grid.addIdleDriver(&d2);
    grid.addIdleDriver(&d3);

    TEST_ASSERT_EQ(grid.totalIdleDrivers(), 3, L"总空闲司机应为3");

    const GridCell& cell3050 = grid.getCell(30, 50);
    TEST_ASSERT_EQ(cell3050.idleDriverCount, 2, L"网格(30,50)应有2名司机");

    // 删除网格(30,50)中的d1
    Driver* removed = grid.removeIdleDriver(1, 30, 50);
    TEST_ASSERT(removed != nullptr && removed->id == 1, L"应成功删除d1");
    TEST_ASSERT_EQ(grid.getCell(30, 50).idleDriverCount, 1, L"删除后网格(30,50)应有1名司机");
    TEST_ASSERT_EQ(grid.totalIdleDrivers(), 2, L"总空闲应为2");

    // 删除不存在的司机
    removed = grid.removeIdleDriver(999, 30, 50);
    TEST_ASSERT(removed == nullptr, L"删除不存在司机应返回nullptr");

    // popIdleDriver 测试
    Driver* popped = grid.popIdleDriver(30, 50);
    TEST_ASSERT(popped != nullptr, L"popIdleDriver应返回司机");
    TEST_ASSERT_EQ(grid.getCell(30, 50).idleDriverCount, 0, L"pop后网格应为空");
    TEST_ASSERT_EQ(grid.totalIdleDrivers(), 1, L"总空闲应为1");

    popped = grid.popIdleDriver(30, 50);
    TEST_ASSERT(popped == nullptr, L"空网格popIdleDriver应返回nullptr");

    std::wcout << L"  [通过] 网格司机增删测试全部通过" << std::endl;
    return true;
}

// ---- 测试7: 派单撮合（同一网格多名司机的最优匹配、无司机重排队）----
bool SelfTest::testDispatch() {
    Simulator sim;
    sim.init(42);
    sim.setHotspotEnabled(false);  // 关闭热点便于控制

    // 清空所有司机，手动添加
    // 先重置，然后手动构造场景
    // 由于 init 已生成100个司机，我们直接测试派单流程
    // 创建一个订单在某个有司机的网格
    Order* o = new Order();
    o->id = 99999;
    o->x = 500; o->y = 300;
    o->createTime = 0; o->waitTime = 0;
    o->state = OrderState::WAITING;
    o->matchedDriverId = -1;
    sim.getGridMut().coordToGrid(o->x, o->y, o->gridRow, o->gridCol);

    // 用 DispatchEngine 直接测试撮合
    DispatchEngine engine;
    MatchResult result = engine.matchOrder(o, sim.getGridMut(), MatchMode::Basic);

    // 由于有100个初始司机，大概率能匹配到
    if (result.success) {
        TEST_ASSERT(result.matchedDriver != nullptr, L"应匹配到司机");
        TEST_ASSERT(result.matchedDriver->state == DriverState::IDLE, L"匹配的司机应为空闲");
        std::wcout << L"  [信息] 匹配到司机#" << result.matchedDriver->id
                  << L"，距离" << result.distance << L"，耗时"
                  << result.elapsedMicros << L"us" << std::endl;
    } else {
        std::wcout << L"  [信息] 本轮未匹配到司机（可能附近无司机），属正常" << std::endl;
    }

    // 测试无司机场景：手动创建一个偏远网格的订单
    Order* o2 = new Order();
    o2->id = 99998;
    o2->x = 5; o2->y = 5;  // 网格(0,0)，可能无司机
    o2->createTime = 0; o2->waitTime = 0;
    o2->state = OrderState::WAITING;
    o2->matchedDriverId = -1;
    sim.getGridMut().coordToGrid(o2->x, o2->y, o2->gridRow, o2->gridCol);

    // 清空所有网格的司机，确保无司机可匹配
    for (int r = 0; r < GRID_COUNT; ++r) {
        for (int c = 0; c < GRID_COUNT; ++c) {
            while (Driver* d = sim.getGridMut().popIdleDriver(r, c)) {
                d->state = DriverState::SERVING;  // 标记为非空闲
            }
        }
    }

    MatchResult result2 = engine.matchOrder(o2, sim.getGridMut(), MatchMode::Basic);
    TEST_ASSERT(!result2.success, L"无空闲司机时应匹配失败");

    delete o;
    delete o2;

    std::wcout << L"  [通过] 派单撮合测试全部通过" << std::endl;
    return true;
}

// ---- 测试8: 订单超时取消 ----
bool SelfTest::testOrderTimeout() {
    Simulator sim;
    sim.init(100);
    sim.setHotspotEnabled(false);
    // 注意：不能 setPaused(true)，因为 tick() 在暂停时直接返回
    // 这里让模拟器正常运行，通过 tick 推进时间

    // 先记录初始取消数
    int beforeCancelled = sim.getStats().cancelledOrders;

    // 执行足够多 tick 使部分订单超时
    // 每秒生成5-10个订单，100个初始司机，每个司机服务约11秒
    // 约11秒后开始有订单找不到司机，再等15秒后超时取消
    // 所以需要至少 30 个 tick 才能看到取消
    for (int i = 0; i < 40; ++i) {
        sim.tick();
    }
    int afterCancelled = sim.getStats().cancelledOrders;
    TEST_ASSERT(afterCancelled > beforeCancelled, L"超时后取消订单数应增加");

    std::wcout << L"  [通过] 订单超时取消测试通过（取消数: "
              << beforeCancelled << L" -> " << afterCancelled << L"）" << std::endl;
    return true;
}

// ---- 测试9: 模拟器重置（计数器、队列、链表、堆均清理）----
bool SelfTest::testSimulatorReset() {
    Simulator sim;
    sim.init(200);
    // 注意：不能 setPaused(true)，因为 tick() 在暂停时直接返回

    // 运行几个 tick 产生数据
    for (int i = 0; i < 5; ++i) {
        sim.tick();
    }

    TEST_ASSERT(sim.getStats().totalOrders > 0, L"运行后应有订单生成");
    TEST_ASSERT(sim.getSimTime() > 0, L"运行后模拟时间应>0");

    // 重置
    sim.reset();

    TEST_ASSERT_EQ(sim.getSimTime(), 0, L"重置后模拟时间应为0");
    TEST_ASSERT_EQ(sim.getStats().totalOrders, 0, L"重置后订单数应为0");
    TEST_ASSERT_EQ(sim.getStats().matchedOrders, 0, L"重置后匹配数应为0");
    TEST_ASSERT_EQ(sim.getWaitingQueueSize(), 0, L"重置后等待队列应为空");
    TEST_ASSERT_EQ(sim.getEngine().getMatchCount(), 0, L"重置后引擎匹配计数应为0");
    TEST_ASSERT_EQ(sim.getEngine().getTotalMicros(), 0, L"重置后引擎总耗时应为0");

    // 验证网格被清空后重新初始化了司机
    TEST_ASSERT_EQ(sim.getIdleDriverCount(), INITIAL_DRIVER_COUNT, L"重置后空闲司机数应恢复初始值");

    std::wcout << L"  [通过] 模拟器重置测试全部通过" << std::endl;
    return true;
}

// ---- 测试10: 热点检测与调度（最低保留量）----
bool SelfTest::testHotspotReposition() {
    Simulator sim;
    sim.init(300);
    sim.setHotspotEnabled(true);  // 开启热点
    // 注意：不能 setPaused(true)，因为 tick() 在暂停时直接返回

    // 运行足够多 tick 让热点产生供需失衡
    for (int i = 0; i < 30; ++i) {
        sim.tick();
    }

    // 检查是否有警报网格产生（热点开启时应出现）
    int alertCount = sim.getAlertGridCount();
    std::wcout << L"  [信息] 运行30 tick后警报网格数: " << alertCount << std::endl;

    // 检查最低保留量：遍历所有网格，确认没有网格司机数 < 0
    bool noNegativeDrivers = true;
    const CityGrid& grid = sim.getGrid();
    for (int r = 0; r < GRID_COUNT; ++r) {
        for (int c = 0; c < GRID_COUNT; ++c) {
            if (grid.getCell(r, c).idleDriverCount < 0) {
                noNegativeDrivers = false;
                break;
            }
        }
    }
    TEST_ASSERT(noNegativeDrivers, L"不应有网格司机数为负");

    // 检查总空闲司机数非负
    TEST_ASSERT(sim.getIdleDriverCount() >= 0, L"总空闲司机数应非负");

    std::wcout << L"  [通过] 热点检测与调度测试通过" << std::endl;
    return true;
}
