#include "CityGrid.h"
#include <cmath>

// =============================================================================
// CityGrid.cpp —— 城市网格空间索引实现
// =============================================================================

CityGrid::CityGrid() : totalIdle_(0) {
    // 标记预定义热点区域
    for (int i = 0; i < HOTSPOT_COUNT; ++i) {
        int r = HOTSPOTS[i].row;
        int c = HOTSPOTS[i].col;
        if (r >= 0 && r < GRID_COUNT && c >= 0 && c < GRID_COUNT) {
            cells_[r][c].isHotspot = true;
        }
    }
}

CityGrid::~CityGrid() {
    // GridCell 的 SinglyLinkedList 析构会自动释放节点
}

void CityGrid::reset() {
    for (int r = 0; r < GRID_COUNT; ++r) {
        for (int c = 0; c < GRID_COUNT; ++c) {
            cells_[r][c].reset();
            // 保留热点标记（热点配置不随重置消失）
            for (int i = 0; i < HOTSPOT_COUNT; ++i) {
                if (HOTSPOTS[i].row == r && HOTSPOTS[i].col == c) {
                    cells_[r][c].isHotspot = true;
                }
            }
        }
    }
    totalIdle_ = 0;
}

void CityGrid::coordToGrid(double x, double y, int& row, int& col) const {
    // 用整数除法在 O(1) 时间内定位网格
    // 越界坐标夹紧，避免数组越界
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= CITY_SIZE) x = CITY_SIZE - 1;
    if (y >= CITY_SIZE) y = CITY_SIZE - 1;

    col = static_cast<int>(x) / CELL_SIZE;
    row = static_cast<int>(y) / CELL_SIZE;

    // 二次保护
    if (row < 0) row = 0;
    if (row >= GRID_COUNT) row = GRID_COUNT - 1;
    if (col < 0) col = 0;
    if (col >= GRID_COUNT) col = GRID_COUNT - 1;
}

void CityGrid::gridToCenterCoord(int row, int col, double& x, double& y) const {
    x = col * CELL_SIZE + CELL_SIZE / 2.0;
    y = row * CELL_SIZE + CELL_SIZE / 2.0;
}

bool CityGrid::isValidGrid(int row, int col) const {
    return row >= 0 && row < GRID_COUNT && col >= 0 && col < GRID_COUNT;
}

void CityGrid::addIdleDriver(Driver* d) {
    assert(d != nullptr);
    int r, c;
    coordToGrid(d->x, d->y, r, c);
    d->gridRow = r;
    d->gridCol = c;
    cells_[r][c].idleDrivers.pushFront(d);
    cells_[r][c].idleDriverCount++;
    totalIdle_++;
}

Driver* CityGrid::removeIdleDriver(int driverId, int row, int col) {
    if (!isValidGrid(row, col)) return nullptr;
    Driver* d = cells_[row][col].idleDrivers.removeById(driverId);
    if (d != nullptr) {
        cells_[row][col].idleDriverCount--;
        totalIdle_--;
    }
    return d;
}

Driver* CityGrid::popIdleDriver(int row, int col) {
    if (!isValidGrid(row, col)) return nullptr;
    Driver* d = cells_[row][col].idleDrivers.popFront();
    if (d != nullptr) {
        cells_[row][col].idleDriverCount--;
        totalIdle_--;
    }
    return d;
}

GridCell& CityGrid::getCell(int row, int col) {
    assert(isValidGrid(row, col));
    return cells_[row][col];
}

const GridCell& CityGrid::getCell(int row, int col) const {
    assert(isValidGrid(row, col));
    return cells_[row][col];
}

void CityGrid::searchCandidates(int centerRow, int centerCol, int radius,
                                DynamicArray<Driver*>& outCandidates) const {
    // 按一圈一圈扩大搜索：radius=0 只搜中心格，radius=1 搜中心+周围8格...
    // 这种方式保证距离近的司机先被收集到（虽然堆会重新排序，但减少远距离候选）
    for (int ring = 0; ring <= radius; ++ring) {
        int r0 = centerRow - ring;
        int r1 = centerRow + ring;
        int c0 = centerCol - ring;
        int c1 = centerCol + ring;

        if (ring == 0) {
            // 只搜中心格
            if (isValidGrid(centerRow, centerCol)) {
                cells_[centerRow][centerCol].idleDrivers.forEach(
                    [&outCandidates](Driver* d) { outCandidates.push(d); });
            }
        } else {
            // 搜这一圈的边界网格（上下两条边 + 左右两条边，避免重复角点）
            for (int c = c0; c <= c1; ++c) {
                if (isValidGrid(r0, c)) {
                    cells_[r0][c].idleDrivers.forEach(
                        [&outCandidates](Driver* d) { outCandidates.push(d); });
                }
                if (r1 != r0 && isValidGrid(r1, c)) {
                    cells_[r1][c].idleDrivers.forEach(
                        [&outCandidates](Driver* d) { outCandidates.push(d); });
                }
            }
            for (int r = r0 + 1; r < r1; ++r) {
                if (isValidGrid(r, c0)) {
                    cells_[r][c0].idleDrivers.forEach(
                        [&outCandidates](Driver* d) { outCandidates.push(d); });
                }
                if (c1 != c0 && isValidGrid(r, c1)) {
                    cells_[r][c1].idleDrivers.forEach(
                        [&outCandidates](Driver* d) { outCandidates.push(d); });
                }
            }
        }
    }
}

int CityGrid::updateImbalance() {
    int alertCount = 0;
    for (int r = 0; r < GRID_COUNT; ++r) {
        for (int c = 0; c < GRID_COUNT; ++c) {
            GridCell& cell = cells_[r][c];
            cell.imbalance = cell.waitingOrderCount - cell.idleDriverCount;
            // 供需差值超过阈值 -> 红色警报热点
            if (cell.imbalance > IMBALANCE_THRESHOLD) {
                if (!cell.isAlert) {
                    cell.isAlert = true;
                }
                ++alertCount;
            } else {
                cell.isAlert = false;
            }
        }
    }
    return alertCount;
}

void CityGrid::markHotspot(int row, int col, bool isHot) {
    if (isValidGrid(row, col)) {
        cells_[row][col].isHotspot = isHot;
    }
}

bool CityGrid::isHotspot(int row, int col) const {
    if (!isValidGrid(row, col)) return false;
    return cells_[row][col].isHotspot;
}

void CityGrid::getAlertGrids(DynamicArray<int>& outRows,
                             DynamicArray<int>& outCols) const {
    for (int r = 0; r < GRID_COUNT; ++r) {
        for (int c = 0; c < GRID_COUNT; ++c) {
            if (cells_[r][c].isAlert) {
                outRows.push(r);
                outCols.push(c);
            }
        }
    }
}
