#ifndef RIDEHAULING_CITY_GRID_H
#define RIDEHAULING_CITY_GRID_H

#include "../config.h"
#include "../structures/SinglyLinkedList.h"
#include "../structures/DynamicArray.h"
#include "../models/Driver.h"
#include "../models/Order.h"

// =============================================================================
// CityGrid.h —— 城市网格空间索引
// 职责：
//   1. 管理 100x100 网格矩阵，每个网格维护一个空闲司机链表。
//   2. 提供 O(1) 坐标 -> 网格映射。
//   3. 统计每个网格的供需情况，支持热点检测。
//   4. 提供按半径搜索候选司机的接口（供派单引擎使用）。
// =============================================================================

// 单个网格信息
struct GridCell {
    SinglyLinkedList idleDrivers;   // 空闲司机链表
    int idleDriverCount;            // 空闲司机数量（冗余，避免每次遍历计数）
    int waitingOrderCount;          // 等待订单数量（每秒派单前统计）
    int imbalance;                  // 供需差值 = waiting - idle
    bool isHotspot;                 // 是否为热点区域（订单高发）
    bool isAlert;                   // 是否触发红色警报（供需失衡）

    GridCell() : idleDriverCount(0), waitingOrderCount(0),
                 imbalance(0), isHotspot(false), isAlert(false) {}

    void reset() {
        idleDrivers.clear();
        idleDriverCount = 0;
        waitingOrderCount = 0;
        imbalance = 0;
        isAlert = false;
    }
};

class CityGrid {
public:
    CityGrid();
    ~CityGrid();

    CityGrid(const CityGrid&) = delete;
    CityGrid& operator=(const CityGrid&) = delete;

    // 重置整个网格矩阵（清空所有网格的链表和计数）
    void reset();

    // 坐标 -> 网格行列（O(1)）。越界坐标会被夹紧到合法范围。
    void coordToGrid(double x, double y, int& row, int& col) const;

    // 网格行列 -> 网格中心像素坐标（用于调度目标定位）
    void gridToCenterCoord(int row, int col, double& x, double& y) const;

    // 边界检查
    bool isValidGrid(int row, int col) const;

    // 加入空闲司机到对应网格链表
    void addIdleDriver(Driver* d);

    // 从对应网格链表移除司机（按编号）
    Driver* removeIdleDriver(int driverId, int row, int col);

    // 弹出指定网格链表头部的空闲司机（用于调度取车），正确更新计数
    Driver* popIdleDriver(int row, int col);

    // 获取指定网格
    GridCell& getCell(int row, int col);
    const GridCell& getCell(int row, int col) const;

    // 在指定中心网格周围按半径搜索候选司机，收集到 candidates 数组中
    // radius 为搜索圈数（1 表示中心+相邻8格）
    void searchCandidates(int centerRow, int centerCol, int radius,
                          DynamicArray<Driver*>& outCandidates) const;

    // 统计所有网格的供需差值，标记警报热点
    // 返回警报网格数量
    int updateImbalance();

    // 设置/获取热点标记（订单高发区域）
    void markHotspot(int row, int col, bool isHot);
    bool isHotspot(int row, int col) const;

    // 获取所有警报网格（用于调度）
    void getAlertGrids(DynamicArray<int>& outRows, DynamicArray<int>& outCols) const;

    // 全局统计
    int totalIdleDrivers() const { return totalIdle_; }

private:
    GridCell cells_[GRID_COUNT][GRID_COUNT];
    int      totalIdle_;   // 全局空闲司机总数
};

#endif // RIDEHAULING_CITY_GRID_H
