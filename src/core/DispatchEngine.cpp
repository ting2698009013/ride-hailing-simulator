#include "DispatchEngine.h"
#include "../models/Order.h"
#include "../models/Driver.h"
#include <chrono>
#include <cmath>
#include <random>

// =============================================================================
// DispatchEngine.cpp —— 派单引擎实现
// 核心流程：搜索候选司机 -> 构造最小堆 -> 弹堆顶 -> 完成撮合
// =============================================================================

DispatchEngine::DispatchEngine()
    : totalMicros_(0), matchCount_(0), lastMicros_(0) {}

DispatchEngine::~DispatchEngine() {}

double DispatchEngine::pixelDistance(double x1, double y1, double x2, double y2) {
    double dx = x2 - x1;
    double dy = y2 - y1;
    return std::sqrt(dx * dx + dy * dy);
}

double DispatchEngine::computeScore(double pixelDist, double rating,
                                    MatchMode mode, double trafficCoef) const {
    // 将像素距离转换为网格距离，让量纲更合理
    double gridDist = pixelDist / CELL_SIZE;
    if (mode == MatchMode::Traffic) {
        // 路况模式：距离乘以拥堵系数，再减去评分
        return gridDist * trafficCoef - rating;
    }
    // 基础模式：网格距离 - 评分
    return gridDist - rating;
}

double DispatchEngine::getTrafficCoef(int row, int col, const CityGrid& grid) const {
    // 简化路况模型：
    //   普通网格 1.0
    //   热点网格（订单高发）1.5
    //   警报网格（供需失衡）2.0
    double coef = MIN_TRAFFIC_COEF;
    const GridCell& cell = grid.getCell(row, col);
    if (cell.isAlert) {
        coef = MAX_TRAFFIC_COEF;        // 2.0
    } else if (cell.isHotspot) {
        coef = (MIN_TRAFFIC_COEF + MAX_TRAFFIC_COEF) / 2.0;  // 1.5
    }
    return coef;
}

MatchResult DispatchEngine::matchOrder(Order* order, CityGrid& grid, MatchMode mode) {
    MatchResult result;
    auto start = std::chrono::high_resolution_clock::now();

    MinHeap heap(128);  // 候选司机堆

    // 从订单所在网格开始，一圈一圈扩大搜索
    int centerRow = order->gridRow;
    int centerCol = order->gridCol;

    DynamicArray<Driver*> candidates(64);
    grid.searchCandidates(centerRow, centerCol, MAX_SEARCH_RADIUS, candidates);

    // 为每名候选司机计算撮合分数，压入最小堆
    for (int i = 0; i < candidates.size(); ++i) {
        Driver* d = candidates[i];
        if (d == nullptr || d->state != DriverState::IDLE) continue;

        double dist = pixelDistance(order->x, order->y, d->x, d->y);
        double trafficCoef = getTrafficCoef(d->gridRow, d->gridCol, grid);
        double score = computeScore(dist, d->rating, mode, trafficCoef);

        MinHeap::Candidate cand(score, d, dist, d->gridRow, d->gridCol);
        heap.push(cand);
    }

    // 弹出堆顶，获得最优匹配
    MinHeap::Candidate best;
    if (heap.pop(best)) {
        result.success       = true;
        result.matchedDriver = best.driver;
        result.distance      = best.distance;
        result.gridDistance  = best.distance / CELL_SIZE;
        result.score         = best.score;
        ++matchCount_;
    }

    auto end = std::chrono::high_resolution_clock::now();
    result.elapsedMicros = std::chrono::duration_cast<std::chrono::microseconds>(
        end - start).count();
    lastMicros_ = result.elapsedMicros;
    if (result.success) {
        totalMicros_ += result.elapsedMicros;
    }

    return result;
}

void DispatchEngine::resetStats() {
    totalMicros_ = 0;
    matchCount_  = 0;
    lastMicros_  = 0;
}
