#ifndef RIDEHAULING_DISPATCH_ENGINE_H
#define RIDEHAULING_DISPATCH_ENGINE_H

#include "../config.h"
#include "../structures/MinHeap.h"
#include "../structures/DynamicArray.h"
#include "CityGrid.h"

// =============================================================================
// DispatchEngine.h —— 派单引擎
// 职责：
//   1. 对单个订单，在周边网格搜索候选司机，构造最小堆，弹出堆顶完成撮合。
//   2. 计算撮合代价分数（基础模式 / 路况增强模式）。
//   3. 统计撮合耗时（单次和累计），用于看板展示。
// 撮合公式：
//   基础：Score = gridDistance - rating
//   路况：Score = gridDistance * trafficCoef - rating
// =============================================================================

struct Order;
struct Driver;

// 单次撮合结果
struct MatchResult {
    bool        success;
    Driver*     matchedDriver;
    double      distance;        // 像素距离
    double      gridDistance;    // 网格距离
    double      score;           // 撮合分数
    long long   elapsedMicros;   // 本次撮合耗时（微秒）

    MatchResult() : success(false), matchedDriver(nullptr),
                    distance(0), gridDistance(0), score(0), elapsedMicros(0) {}
};

class DispatchEngine {
public:
    DispatchEngine();
    ~DispatchEngine();

    // 为单个订单尝试匹配司机。
    // matchMode: 基础 or 路况增强
    // 返回撮合结果；success=false 表示未找到司机
    MatchResult matchOrder(Order* order, CityGrid& grid, MatchMode mode);

    // 生成指定网格的路况系数（1.0 ~ 2.0）
    // 简化模型：热点网格和警报网格路况更差
    double getTrafficCoef(int row, int col, const CityGrid& grid) const;

    // 统计
    long long getTotalMicros()  const { return totalMicros_; }
    int       getMatchCount()   const { return matchCount_; }
    long long getLastMicros()   const { return lastMicros_; }

    void resetStats();

private:
    long long totalMicros_;   // 累计撮合耗时（微秒）
    int       matchCount_;    // 成功撮合次数
    long long lastMicros_;    // 最近一次撮合耗时

    // 计算两点之间的欧氏距离（像素）
    static double pixelDistance(double x1, double y1, double x2, double y2);

    // 计算撮合分数
    double computeScore(double pixelDist, double rating,
                        MatchMode mode, double trafficCoef) const;
};

#endif // RIDEHAULING_DISPATCH_ENGINE_H
