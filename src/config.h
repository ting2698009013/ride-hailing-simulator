#ifndef RIDEHAULING_CONFIG_H
#define RIDEHAULING_CONFIG_H

// =============================================================================
// config.h —— 全局配置常量集中定义
// 说明：本课程设计中所有"魔法数字"均集中于此文件，方便课程答辩时讲解，
//       也方便修改后重新进行实验。注意：修改这些常量后需要重新编译。
// =============================================================================

// ------------------------------ 城市与网格 ----------------------------------
constexpr int CITY_SIZE        = 1000;   // 虚拟城市坐标系边长（像素单位）
constexpr int GRID_COUNT       = 100;    // 每条边上的网格数量（100x100）
constexpr int CELL_SIZE        = CITY_SIZE / GRID_COUNT;  // 单个网格边长 = 10

// ------------------------------ 窗口尺寸 ------------------------------------
constexpr int WINDOW_WIDTH     = 1400;   // 窗口总宽
constexpr int WINDOW_HEIGHT    = 820;    // 窗口总高
constexpr int MAP_PIXEL_SIZE   = 700;    // 地图热力图区域像素边长
constexpr int MAP_X            = 20;     // 地图左上角 X
constexpr int MAP_Y            = 20;     // 地图左上角 Y
constexpr int CELL_PIXEL       = MAP_PIXEL_SIZE / GRID_COUNT;  // 每个网格像素 = 7

// ------------------------------ 模拟参数 ------------------------------------
constexpr int INITIAL_DRIVER_COUNT   = 100;   // 初始空闲司机数量
constexpr int MIN_ORDERS_PER_TICK    = 5;     // 每模拟秒最少生成订单数
constexpr int MAX_ORDERS_PER_TICK    = 10;    // 每模拟秒最多生成订单数
constexpr int ORDER_TIMEOUT_SECONDS  = 15;    // 订单等待超时阈值（秒）
constexpr int MAX_SEARCH_RADIUS      = 8;     // 派单最大搜索半径（网格圈数）
constexpr int IMBALANCE_THRESHOLD    = 3;     // 供需失衡阈值（waiting - idle）
constexpr int MAX_REPOSITION_PER_TICK = 10;   // 每秒最多调度司机数
constexpr int MIN_RESERVE_DRIVERS    = 2;     // 来源网格最低保留司机数

// ------------------------------ 司机评分范围 --------------------------------
constexpr double MIN_RATING = 3.5;
constexpr double MAX_RATING = 5.0;

// ------------------------------ 路况系数（增强模式）-------------------------
constexpr double MIN_TRAFFIC_COEF = 1.0;
constexpr double MAX_TRAFFIC_COEF = 2.0;

// ------------------------------ 状态机时间 ----------------------------------
// 司机接单后到开始服务、服务结束等阶段的剩余时间（模拟秒）
constexpr int ASSIGNED_DURATION   = 3;   // 已接单 -> 服务中 持续时间
constexpr int SERVING_DURATION    = 8;   // 服务中 -> 重新空闲 持续时间
constexpr int REPOSITION_DURATION = 5;   // 调度中 -> 空闲 持续时间

// ------------------------------ 日志 ----------------------------------------
constexpr int MAX_LOG_LINES = 30;       // 日志循环缓冲区容量

// ------------------------------ 热点区域 ------------------------------------
// 预定义 3 个热点中心坐标（网格行列），订单生成概率明显更高
struct HotspotConfig {
    int row;
    int col;
    double probability;   // 该热点生成订单的权重倍数
};

// 故意设置的 2~3 个热点区域，用于测试供需失衡与调度
constexpr HotspotConfig HOTSPOTS[] = {
    { 25, 30, 6.0 },
    { 50, 55, 5.0 },
    { 75, 20, 4.0 },
};
constexpr int HOTSPOT_COUNT = sizeof(HOTSPOTS) / sizeof(HOTSPOTS[0]);

// ------------------------------ 撮合模式 ------------------------------------
enum class MatchMode {
    Basic,    // 基础模式：Score = gridDistance - rating
    Traffic,  // 路况模式：Score = gridDistance * trafficCoef - rating
};

// ------------------------------ 速度倍率 ------------------------------------
// 通过按钮切换：0.5x / 1x / 2x / 5x
constexpr double SPEED_OPTIONS[] = { 0.5, 1.0, 2.0, 5.0 };
constexpr int SPEED_OPTION_COUNT = 4;

// ------------------------------ 基础模拟时间间隔 ----------------------------
// 1 个模拟 tick = 1 模拟秒；真实间隔 = BASE_TICK_MS / speed
constexpr int BASE_TICK_MS = 1000;

// ------------------------------ 默认随机种子 --------------------------------
constexpr unsigned int DEFAULT_SEED = 20260726u;

// ------------------------------ 自检测试宏 ----------------------------------
// 定义该宏后，main 会先运行自检测试再进入图形界面
// #define RUN_SELF_TEST

#endif // RIDEHAULING_CONFIG_H
