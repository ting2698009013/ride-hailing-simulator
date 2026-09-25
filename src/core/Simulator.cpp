#include "Simulator.h"
#include <random>
#include <cmath>
#include <sstream>

// =============================================================================
// Simulator.cpp —— 模拟器核心实现
// 本文件是整个系统的"指挥中心"，协调订单生成、状态机、派单和调度。
// =============================================================================

// 使用 mt19937 作为随机数引擎，保证可复现
static std::mt19937 g_rng;

Simulator::Simulator()
    : waitingQueue_(8192),  // 订单等待队列容量
      simTime_(0), seed_(DEFAULT_SEED), paused_(false), speed_(1.0),
      hotspotEnabled_(true), matchMode_(MatchMode::Basic),
      showPoints_(true), lastAlertCount_(0),
      nextDriverId_(1), nextOrderId_(1) {
}

Simulator::~Simulator() {
    // 释放所有司机对象
    for (int i = 0; i < drivers_.size(); ++i) {
        delete drivers_[i];
    }
    // 释放所有订单对象
    for (int i = 0; i < allOrders_.size(); ++i) {
        delete allOrders_[i];
    }
}

void Simulator::init(unsigned int seed) {
    seed_ = seed;
    g_rng.seed(seed);

    // 清理已有数据
    for (int i = 0; i < drivers_.size(); ++i) delete drivers_[i];
    drivers_.clear();
    for (int i = 0; i < allOrders_.size(); ++i) delete allOrders_[i];
    allOrders_.clear();

    grid_.reset();
    engine_.resetStats();
    logger_.clear();
    repositionEvents_.clear();
    stats_.reset();
    waitingQueue_.clear();  // 清空等待订单队列

    simTime_ = 0;
    nextDriverId_ = 1;
    nextOrderId_ = 1;
    lastAlertCount_ = 0;

    generateInitialDrivers();

    addLog(L"[系统初始化] 种子=" + std::to_wstring(seed_) +
           L"，初始司机=" + std::to_wstring(INITIAL_DRIVER_COUNT));
}

void Simulator::reset() {
    init(seed_);
    addLog(L"[系统重置] 模拟已重置，保留随机种子");
}

void Simulator::generateInitialDrivers() {
    for (int i = 0; i < INITIAL_DRIVER_COUNT; ++i) {
        Driver* d = new Driver();
        d->id = nextDriverId_++;
        // 随机位置，但在热点区域附近多生成一些（模拟现实司机聚集）
        if (hotspotEnabled_ && (i % 3 == 0)) {
            // 三分之一司机生成在热点附近
            int hsIdx = g_rng() % HOTSPOT_COUNT;
            double cx, cy;
            grid_.gridToCenterCoord(HOTSPOTS[hsIdx].row, HOTSPOTS[hsIdx].col, cx, cy);
            // 在热点中心附近 ±50 像素随机
            std::uniform_real_distribution<double> off(-50, 50);
            d->x = cx + off(g_rng);
            d->y = cy + off(g_rng);
            if (d->x < 0) d->x = 0;
            if (d->y < 0) d->y = 0;
            if (d->x >= CITY_SIZE) d->x = CITY_SIZE - 1;
            if (d->y >= CITY_SIZE) d->y = CITY_SIZE - 1;
        } else {
            std::uniform_real_distribution<double> pos(0, CITY_SIZE - 1);
            d->x = pos(g_rng);
            d->y = pos(g_rng);
        }
        d->rating = randomRating();
        d->state = DriverState::IDLE;
        d->currentOrderId = -1;
        d->remainTime = 0;
        grid_.addIdleDriver(d);
        drivers_.push(d);
        ++stats_.totalGeneratedDrivers;
    }
}

double Simulator::randomRating() {
    std::uniform_real_distribution<double> r(MIN_RATING, MAX_RATING);
    // 保留一位小数，更真实
    double v = r(g_rng);
    return std::round(v * 10.0) / 10.0;
}

void Simulator::pickOrderLocation(double& x, double& y) {
    // 按热点加权：热点区域的订单生成概率明显更高
    // 实现方式：以一定概率从热点中心附近生成，否则均匀随机
    if (hotspotEnabled_) {
        std::uniform_real_distribution<double> prob(0.0, 1.0);
        if (prob(g_rng) < 0.6) {
            // 60% 概率从热点生成
            int hsIdx = g_rng() % HOTSPOT_COUNT;
            double cx, cy;
            grid_.gridToCenterCoord(HOTSPOTS[hsIdx].row, HOTSPOTS[hsIdx].col, cx, cy);
            std::uniform_real_distribution<double> off(-40, 40);
            x = cx + off(g_rng);
            y = cy + off(g_rng);
            if (x < 0) x = 0;
            if (y < 0) y = 0;
            if (x >= CITY_SIZE) x = CITY_SIZE - 1;
            if (y >= CITY_SIZE) y = CITY_SIZE - 1;
            return;
        }
    }
    // 均匀随机
    std::uniform_real_distribution<double> pos(0, CITY_SIZE - 1);
    x = pos(g_rng);
    y = pos(g_rng);
}

void Simulator::generateOrders() {
    // 每模拟秒生成 5~10 个订单
    std::uniform_int_distribution<int> countDist(MIN_ORDERS_PER_TICK, MAX_ORDERS_PER_TICK);
    int n = countDist(g_rng);

    for (int i = 0; i < n; ++i) {
        Order* o = new Order();
        o->id = nextOrderId_++;
        pickOrderLocation(o->x, o->y);
        o->createTime = simTime_;
        o->waitTime = 0;
        o->state = OrderState::WAITING;
        o->matchedDriverId = -1;
        grid_.coordToGrid(o->x, o->y, o->gridRow, o->gridCol);

        allOrders_.push(o);
        if (!waitingQueue_.push(o)) {
            // 队列满（理论上 8192 足够），记录日志
            addLog(L"[警告] 订单队列已满，订单#" + std::to_wstring(o->id) + L" 被丢弃");
        }
        ++stats_.totalOrders;
    }
}

void Simulator::processTimeouts() {
    // 处理等待超时的订单：从队列中移除并标记为取消
    // 由于 CircularQueue 的 removeById 会改变顺序，先收集要取消的订单编号
    DynamicArray<int> toCancel(32);

    waitingQueue_.forEach([&](Order* o) {
        o->waitTime++;
        if (o->waitTime > ORDER_TIMEOUT_SECONDS) {
            toCancel.push(o->id);
        }
    });

    for (int i = 0; i < toCancel.size(); ++i) {
        int orderId = toCancel[i];
        waitingQueue_.removeById(orderId);

        // 在订单池中找到并标记取消
        int idx = findOrderIndex(orderId);
        if (idx >= 0 && allOrders_[idx]->state == OrderState::WAITING) {
            allOrders_[idx]->state = OrderState::CANCELLED;
            ++stats_.cancelledOrders;
            addLog(L"[订单取消] 订单#" + std::to_wstring(orderId) +
                   L" 等待超时（>" + std::to_wstring(ORDER_TIMEOUT_SECONDS) + L"秒）");
        }
    }
}

void Simulator::updateDriverStates() {
    // 遍历所有司机，推进状态机
    for (int i = 0; i < drivers_.size(); ++i) {
        Driver* d = drivers_[i];
        if (d->state == DriverState::IDLE) continue;  // 空闲司机无需推进

        // 非空闲司机有剩余时间，递减
        if (d->remainTime > 0) {
            d->remainTime--;
        }

        if (d->remainTime > 0) continue;  // 状态未结束

        // 状态结束，进行流转
        switch (d->state) {
            case DriverState::ASSIGNED: {
                // 已接单 -> 服务中
                d->state = DriverState::SERVING;
                d->remainTime = SERVING_DURATION;
                if (d->currentOrderId >= 0) {
                    addLog(L"[开始服务] 司机#" + std::to_wstring(d->id) +
                           L" 开始服务订单#" + std::to_wstring(d->currentOrderId));
                }
                break;
            }
            case DriverState::SERVING: {
                // 服务中 -> 空闲，移动到新随机位置重新加入网格
                int oldOrderId = d->currentOrderId;
                d->state = DriverState::IDLE;
                d->currentOrderId = -1;
                d->remainTime = 0;

                // 标记订单完成
                if (oldOrderId >= 0) {
                    int oidx = findOrderIndex(oldOrderId);
                    if (oidx >= 0 && allOrders_[oidx]->state == OrderState::MATCHED) {
                        allOrders_[oidx]->state = OrderState::COMPLETED;
                        ++stats_.completedOrders;
                    }
                }

                // 移动到新随机位置（模拟送客到达目的地）
                std::uniform_real_distribution<double> pos(0, CITY_SIZE - 1);
                d->x = pos(g_rng);
                d->y = pos(g_rng);
                grid_.addIdleDriver(d);

                addLog(L"[订单完成] 司机#" + std::to_wstring(d->id) +
                       L" 完成订单#" + std::to_wstring(oldOrderId) + L"，重新空闲");
                break;
            }
            case DriverState::REPOSITIONING: {
                // 调度中 -> 空闲，到达目标热点网格
                double nx, ny;
                grid_.gridToCenterCoord(d->targetRow, d->targetCol, nx, ny);
                d->x = nx;
                d->y = ny;
                d->state = DriverState::IDLE;
                d->remainTime = 0;
                grid_.addIdleDriver(d);

                addLog(L"[调度到达] 司机#" + std::to_wstring(d->id) +
                       L" 到达热点网格(" + std::to_wstring(d->targetRow) + L"," +
                       std::to_wstring(d->targetCol) + L")");
                break;
            }
            default:
                break;
        }
    }
}

int Simulator::findDriverIndex(int driverId) {
    for (int i = 0; i < drivers_.size(); ++i) {
        if (drivers_[i]->id == driverId) return i;
    }
    return -1;
}

int Simulator::findOrderIndex(int orderId) {
    for (int i = 0; i < allOrders_.size(); ++i) {
        if (allOrders_[i]->id == orderId) return i;
    }
    return -1;
}

void Simulator::batchDispatch() {
    // 批量派单：从等待队列取出所有订单逐个撮合
    // 注意：派单失败的要重新入队，但不能在遍历中反复处理同一订单
    // 策略：先全部取出，逐个撮合，成功的标记 MATCHED 并移除司机，
    //       失败的重新入队

    DynamicArray<Order*> toProcess(256);
    // 取出所有等待订单
    while (Order* o = waitingQueue_.pop()) {
        toProcess.push(o);
    }

    for (int i = 0; i < toProcess.size(); ++i) {
        Order* o = toProcess[i];
        if (o->state != OrderState::WAITING) continue;  // 可能已被超时取消

        MatchResult result = engine_.matchOrder(o, grid_, matchMode_);

        if (result.success) {
            // 匹配成功
            Driver* d = result.matchedDriver;
            // 从网格链表移除该司机
            Driver* removed = grid_.removeIdleDriver(d->id, d->gridRow, d->gridCol);
            if (removed == nullptr) {
                // 司机可能已被其他逻辑移除，重新入队
                waitingQueue_.push(o);
                continue;
            }

            // 更新状态
            d->state = DriverState::ASSIGNED;
            d->currentOrderId = o->id;
            d->remainTime = ASSIGNED_DURATION;

            o->state = OrderState::MATCHED;
            o->matchedDriverId = d->id;
            ++stats_.matchedOrders;

            // 日志（耗时转毫秒显示）
            double ms = result.elapsedMicros / 1000.0;
            std::wostringstream ss;
            ss << L"[派单成功] 订单#" << o->id << L" ("
               << static_cast<int>(o->x) << L"," << static_cast<int>(o->y)
               << L") -> 司机#" << d->id << L"，评分" << d->rating
               << L"，距离" << static_cast<int>(result.distance)
               << L"，耗时" << std::fixed;
            ss.precision(2);
            ss << ms << L"ms";
            addLog(ss.str());
        } else {
            // 匹配失败，重新入队等待下一轮
            waitingQueue_.push(o);
        }
    }
}

void Simulator::detectAndReposition() {
    // 1. 统计网格供需，标记警报
    lastAlertCount_ = grid_.updateImbalance();

    if (lastAlertCount_ == 0) {
        // 没有警报网格，清除旧的调度事件
        return;
    }

    // 2. 对每个警报网格，从周边网格调度空闲司机
    DynamicArray<int> alertRows(16), alertCols(16);
    grid_.getAlertGrids(alertRows, alertCols);

    int repositionedThisTick = 0;

    for (int a = 0; a < alertRows.size() && repositionedThisTick < MAX_REPOSITION_PER_TICK; ++a) {
        int targetRow = alertRows[a];
        int targetCol = alertCols[a];

        // 搜索周边网格（半径 1~3）的空闲司机
        // 优先从距离近、司机富余量大的网格调度
        struct SourceCandidate {
            int row, col;
            int surplus;  // 富余量 = idle - minReserve
            int distance;  // 到目标网格的距离（网格单位）
        };

        DynamicArray<SourceCandidate> sources(32);

        for (int radius = 1; radius <= 3; ++radius) {
            for (int dr = -radius; dr <= radius; ++dr) {
                for (int dc = -radius; dc <= radius; ++dc) {
                    if (dr == 0 && dc == 0) continue;
                    // 只取这一圈的边界
                    if (std::abs(dr) != radius && std::abs(dc) != radius) continue;

                    int sr = targetRow + dr;
                    int sc = targetCol + dc;
                    if (!grid_.isValidGrid(sr, sc)) continue;

                    const GridCell& cell = grid_.getCell(sr, sc);
                    if (cell.isAlert) continue;  // 警报网格不作为来源

                    int surplus = cell.idleDriverCount - MIN_RESERVE_DRIVERS;
                    if (surplus > 0) {
                        SourceCandidate sc2;
                        sc2.row = sr;
                        sc2.col = sc;
                        sc2.surplus = surplus;
                        sc2.distance = std::abs(dr) + std::abs(dc);
                        sources.push(sc2);
                    }
                }
            }
        }

        // 按距离近、富余量大排序（简单选择排序，数据量小）
        for (int i = 0; i < sources.size() - 1; ++i) {
            int bestIdx = i;
            for (int j = i + 1; j < sources.size(); ++j) {
                // 优先距离近，其次富余大
                if (sources[j].distance < sources[bestIdx].distance ||
                    (sources[j].distance == sources[bestIdx].distance &&
                     sources[j].surplus > sources[bestIdx].surplus)) {
                    bestIdx = j;
                }
            }
            if (bestIdx != i) {
                SourceCandidate tmp = sources[i];
                sources[i] = sources[bestIdx];
                sources[bestIdx] = tmp;
            }
        }

        // 从来源网格调度司机到目标网格
        int need = grid_.getCell(targetRow, targetCol).imbalance;
        int dispatched = 0;
        for (int s = 0; s < sources.size() && dispatched < need && repositionedThisTick < MAX_REPOSITION_PER_TICK; ++s) {
            SourceCandidate& src = sources[s];

            // 从来源网格取一名空闲司机（通过 CityGrid 统一接口，正确更新计数）
            Driver* d = grid_.popIdleDriver(src.row, src.col);
            if (d == nullptr) continue;

            // 进入调度状态
            d->state = DriverState::REPOSITIONING;
            d->targetRow = targetRow;
            d->targetCol = targetCol;
            d->remainTime = REPOSITION_DURATION;
            d->currentOrderId = -1;

            // 记录调度事件（UI 连线）
            RepositionEvent ev;
            ev.fromRow = src.row;
            ev.fromCol = src.col;
            ev.toRow = targetRow;
            ev.toCol = targetCol;
            ev.driverId = d->id;
            ev.remainTicks = 30;  // 显示 30 帧
            repositionEvents_.push(ev);

            ++dispatched;
            ++repositionedThisTick;

            addLog(L"[运力调度] 司机#" + std::to_wstring(d->id) +
                   L" 从网格(" + std::to_wstring(src.row) + L"," +
                   std::to_wstring(src.col) + L") 调度到热点(" +
                   std::to_wstring(targetRow) + L"," +
                   std::to_wstring(targetCol) + L")，原因：供需失衡");
        }

        if (dispatched > 0) {
            addLog(L"[热点警报] 网格(" + std::to_wstring(targetRow) + L"," +
                   std::to_wstring(targetCol) + L") 供需差" +
                   std::to_wstring(grid_.getCell(targetRow, targetCol).imbalance) +
                   L"，已调度" + std::to_wstring(dispatched) + L"名司机");
        }
    }

    // 清理过期的调度事件（UI 连线淡出）
    for (int i = repositionEvents_.size() - 1; i >= 0; --i) {
        repositionEvents_[i].remainTicks--;
        if (repositionEvents_[i].remainTicks <= 0) {
            repositionEvents_.removeAt(i);
        }
    }
}

void Simulator::addLog(const std::wstring& text) {
    logger_.log(simTime_, text);
}

void Simulator::tick() {
    if (paused_) return;

    ++simTime_;

    // 1. 处理超时取消
    processTimeouts();

    // 2. 推进司机状态机
    updateDriverStates();

    // 3. 生成新订单
    generateOrders();

    // 4. 重置网格等待订单计数（派单前统计）
    for (int r = 0; r < GRID_COUNT; ++r) {
        for (int c = 0; c < GRID_COUNT; ++c) {
            grid_.getCell(r, c).waitingOrderCount = 0;
        }
    }
    // 统计每个网格当前等待订单数
    waitingQueue_.forEach([&](Order* o) {
        if (o->state == OrderState::WAITING) {
            grid_.getCell(o->gridRow, o->gridCol).waitingOrderCount++;
        }
    });

    // 5. 批量派单
    batchDispatch();

    // 6. 热点检测与动态调度
    detectAndReposition();
}

int Simulator::getServingDriverCount() const {
    int count = 0;
    for (int i = 0; i < drivers_.size(); ++i) {
        if (drivers_[i]->state == DriverState::SERVING) ++count;
    }
    return count;
}

int Simulator::getRepositioningDriverCount() const {
    int count = 0;
    for (int i = 0; i < drivers_.size(); ++i) {
        if (drivers_[i]->state == DriverState::REPOSITIONING) ++count;
    }
    return count;
}

void Simulator::addOrderAtGrid(int row, int col) {
    if (!grid_.isValidGrid(row, col)) return;
    Order* o = new Order();
    o->id = nextOrderId_++;
    double x, y;
    grid_.gridToCenterCoord(row, col, x, y);
    o->x = x;
    o->y = y;
    o->createTime = simTime_;
    o->waitTime = 0;
    o->state = OrderState::WAITING;
    o->matchedDriverId = -1;
    o->gridRow = row;
    o->gridCol = col;
    allOrders_.push(o);
    waitingQueue_.push(o);
    ++stats_.totalOrders;
    addLog(L"[手动下单] 订单#" + std::to_wstring(o->id) +
           L" 已添加到网格(" + std::to_wstring(row) + L"," +
           std::to_wstring(col) + L")");
}

void Simulator::addDriverAtGrid(int row, int col) {
    if (!grid_.isValidGrid(row, col)) return;
    Driver* d = new Driver();
    d->id = nextDriverId_++;
    double x, y;
    grid_.gridToCenterCoord(row, col, x, y);
    d->x = x;
    d->y = y;
    d->rating = randomRating();
    d->state = DriverState::IDLE;
    d->currentOrderId = -1;
    d->remainTime = 0;
    grid_.addIdleDriver(d);
    drivers_.push(d);
    ++stats_.totalGeneratedDrivers;
    addLog(L"[手动加车] 司机#" + std::to_wstring(d->id) +
           L" 评分" + std::to_wstring(d->rating) +
           L" 已添加到网格(" + std::to_wstring(row) + L"," +
           std::to_wstring(col) + L")");
}

Simulator::GridInfo Simulator::getGridInfo(int row, int col) const {
    GridInfo info;
    info.row = row;
    info.col = col;
    if (!grid_.isValidGrid(row, col)) {
        info.orderCount = 0;
        info.driverCount = 0;
        info.imbalance = 0;
        info.isHotspot = false;
        info.isAlert = false;
        return info;
    }
    const GridCell& cell = grid_.getCell(row, col);
    info.orderCount = cell.waitingOrderCount;
    info.driverCount = cell.idleDriverCount;
    info.imbalance = cell.imbalance;
    info.isHotspot = cell.isHotspot;
    info.isAlert = cell.isAlert;
    return info;
}
