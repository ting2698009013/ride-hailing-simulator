#ifndef RIDEHAULING_DRIVER_H
#define RIDEHAULING_DRIVER_H

#include <string>

// =============================================================================
// Driver.h —— 司机数据模型
// 司机是派单撮合的核心对象，状态机流转如下：
//   IDLE -> ASSIGNED -> SERVING -> IDLE（服务结束后到新位置重新空闲）
//   IDLE -> REPOSITIONING -> IDLE（被调度到热点后重新空闲）
// =============================================================================

// 司机状态枚举
enum class DriverState {
    IDLE,          // 空闲，在网格空闲链表中
    ASSIGNED,      // 已接单，正在前往乘客位置
    SERVING,       // 服务中，正在送客
    REPOSITIONING  // 调度中，被系统引导到热点网格
};

// 司机状态转中文（用于日志和界面显示）
inline const wchar_t* driverStateToString(DriverState s) {
    switch (s) {
        case DriverState::IDLE:         return L"空闲";
        case DriverState::ASSIGNED:     return L"已接单";
        case DriverState::SERVING:      return L"服务中";
        case DriverState::REPOSITIONING:return L"调度中";
    }
    return L"未知";
}

// 司机结构体
// 说明：使用固定大小的 POD 风格结构，避免每帧创建大量临时对象，
//       司机对象在模拟器中通过数组统一管理，链表中只保存指针。
struct Driver {
    int      id;           // 唯一编号
    double   x;            // 横坐标（0 ~ CITY_SIZE-1）
    double   y;            // 纵坐标（0 ~ CITY_SIZE-1）
    double   rating;       // 评分（3.5 ~ 5.0）
    DriverState state;     // 当前状态
    int      gridRow;      // 所属网格行（0 ~ GRID_COUNT-1）
    int      gridCol;      // 所属网格列（0 ~ GRID_COUNT-1）
    int      currentOrderId;   // 当前订单编号，无订单时为 -1
    int      remainTime;       // 当前状态剩余时间（模拟秒）

    // 调度相关：记录调度目标网格，到达后修改坐标
    int      targetRow;
    int      targetCol;

    Driver()
        : id(-1), x(0), y(0), rating(5.0), state(DriverState::IDLE),
          gridRow(0), gridCol(0), currentOrderId(-1), remainTime(0),
          targetRow(0), targetCol(0) {}
};

#endif // RIDEHAULING_DRIVER_H
