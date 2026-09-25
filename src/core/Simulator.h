#ifndef RIDEHAULING_SIMULATOR_H
#define RIDEHAULING_SIMULATOR_H

#include "../config.h"
#include "../structures/DynamicArray.h"
#include "../structures/CircularQueue.h"
#include "CityGrid.h"
#include "DispatchEngine.h"
#include "Logger.h"
#include "../models/Driver.h"
#include "../models/Order.h"

// =============================================================================
// Simulator.h —— 模拟器核心
// 职责（模拟器层不直接实现派单算法，派单交给 DispatchEngine）：
//   1. 生成初始司机和每秒订单（含热点加权）。
//   2. 管理司机/订单状态机流转。
//   3. 每秒执行批量派单（调用 DispatchEngine）。
//   4. 检测供需失衡，执行跨网格运力调度。
//   5. 维护全局统计数据。
// =============================================================================

// 调度事件（用于 UI 绘制连线）
struct RepositionEvent {
    int fromRow;
    int fromCol;
    int toRow;
    int toCol;
    int driverId;
    int remainTicks;  // 剩余显示帧数（用于淡出）

    RepositionEvent() : fromRow(0), fromCol(0), toRow(0), toCol(0),
                        driverId(-1), remainTicks(0) {}
};

// 模拟器统计指标
struct SimulatorStats {
    int   totalOrders;        // 已生成订单数
    int   matchedOrders;      // 派单成功数
    int   cancelledOrders;    // 取消订单数
    int   completedOrders;    // 完成订单数
    int   totalGeneratedDrivers; // 累计生成司机数（含重置后）

    SimulatorStats() { reset(); }
    void reset() {
        totalOrders = 0;
        matchedOrders = 0;
        cancelledOrders = 0;
        completedOrders = 0;
        totalGeneratedDrivers = 0;
    }

    // 派单成功率 = 派单成功 / (派单成功 + 取消) × 100%
    // 说明：已完成订单数也计入派单成功，因为成功匹配后才会进入服务。
    //       这里 matchedOrders 统计的是"曾经成功匹配"的订单数。
    double successRate() const {
        int denom = matchedOrders + cancelledOrders;
        if (denom == 0) return 0.0;
        return static_cast<double>(matchedOrders) / denom * 100.0;
    }
};

class Simulator {
public:
    Simulator();
    ~Simulator();

    Simulator(const Simulator&) = delete;
    Simulator& operator=(const Simulator&) = delete;

    // 初始化：生成初始司机，设置种子
    void init(unsigned int seed);

    // 重置模拟器到初始状态
    void reset();

    // 执行一个模拟 tick（1 模拟秒）的核心逻辑：
    //   1. 更新等待时间，处理超时取消
    //   2. 更新司机状态机（接单->服务->空闲）
    //   3. 生成新订单
    //   4. 统计网格供需，标记警报
    //   5. 批量派单
    //   6. 热点检测与动态调度
    void tick();

    // ---- 控制接口 ----
    void setPaused(bool p) { paused_ = p; }
    bool isPaused() const  { return paused_; }

    void setSpeed(double s) { speed_ = s; }
    double getSpeed() const { return speed_; }

    void setHotspotEnabled(bool e) { hotspotEnabled_ = e; }
    bool isHotspotEnabled() const  { return hotspotEnabled_; }

    void setMatchMode(MatchMode m) { matchMode_ = m; }
    MatchMode getMatchMode() const { return matchMode_; }

    void setShowPoints(bool s) { showPoints_ = s; }
    bool getShowPoints() const { return showPoints_; }

    // ---- 查询接口（供 UI 使用）----
    int   getSimTime() const      { return simTime_; }
    unsigned int getSeed() const  { return seed_; }
    int   getWaitingQueueSize() const { return waitingQueue_.size(); }
    int   getIdleDriverCount() const  { return grid_.totalIdleDrivers(); }

    // 统计服务中/调度中司机数
    int   getServingDriverCount() const;
    int   getRepositioningDriverCount() const;
    int   getAlertGridCount() const { return lastAlertCount_; }

    const SimulatorStats& getStats() const { return stats_; }
    const DispatchEngine& getEngine() const { return engine_; }
    const CityGrid& getGrid() const { return grid_; }
    CityGrid& getGridMut() { return grid_; }
    const Logger& getLogger() const { return logger_; }
    Logger& getLoggerMut() { return logger_; }

    // 获取调度事件列表（UI 绘制连线用）
    const DynamicArray<RepositionEvent>& getRepositionEvents() const {
        return repositionEvents_;
    }

    // 手动交互：在指定网格增加订单 / 增加空闲司机
    void addOrderAtGrid(int row, int col);
    void addDriverAtGrid(int row, int col);

    // 选中网格信息（点击地图查看）
    struct GridInfo {
        int row, col;
        int orderCount;
        int driverCount;
        int imbalance;
        bool isHotspot;
        bool isAlert;
    };
    GridInfo getGridInfo(int row, int col) const;

    // 司机池访问（UI 绘制司机点用）
    const DynamicArray<Driver*>& getDrivers() const { return drivers_; }

    // 所有订单池（UI 绘制订单点用，含等待中的）
    const DynamicArray<Order*>& getOrders() const { return allOrders_; }

private:
    CityGrid        grid_;
    DispatchEngine  engine_;
    CircularQueue   waitingQueue_;
    Logger          logger_;

    DynamicArray<Driver*>  drivers_;    // 所有司机对象（拥有，析构释放）
    DynamicArray<Order*>   allOrders_;  // 所有订单对象（拥有，析构释放）

    DynamicArray<RepositionEvent> repositionEvents_;  // 调度事件（UI 连线）

    SimulatorStats  stats_;

    int   simTime_;           // 模拟运行时间（秒）
    unsigned int seed_;       // 当前随机种子
    bool  paused_;
    double speed_;
    bool  hotspotEnabled_;
    MatchMode matchMode_;
    bool  showPoints_;
    int   lastAlertCount_;
    int   nextDriverId_;      // 下一个司机编号
    int   nextOrderId_;       // 下一个订单编号

    // 内部方法
    void generateInitialDrivers();
    void generateOrders();
    void processTimeouts();
    void updateDriverStates();
    void batchDispatch();
    void detectAndReposition();
    void addLog(const std::wstring& text);

    // 生成订单的辅助：按热点加权随机选择位置
    void pickOrderLocation(double& x, double& y);
    // 生成随机评分
    double randomRating();
    // 找到司机在 drivers_ 数组中的下标
    int findDriverIndex(int driverId);
    int findOrderIndex(int orderId);
};

#endif // RIDEHAULING_SIMULATOR_H
