#ifndef RIDEHAULING_ORDER_H
#define RIDEHAULING_ORDER_H

// =============================================================================
// Order.h —— 订单数据模型
// 订单状态机：
//   WAITING -> MATCHED -> COMPLETED（匹配成功并服务完成）
//   WAITING -> CANCELLED（等待超时被乘客取消）
// =============================================================================

// 订单状态枚举
enum class OrderState {
    WAITING,    // 等待派单，在循环队列中
    MATCHED,    // 已匹配到司机
    CANCELLED,  // 等待超时取消
    COMPLETED   // 订单完成
};

inline const wchar_t* orderStateToString(OrderState s) {
    switch (s) {
        case OrderState::WAITING:   return L"等待中";
        case OrderState::MATCHED:   return L"已匹配";
        case OrderState::CANCELLED: return L"已取消";
        case OrderState::COMPLETED: return L"已完成";
    }
    return L"未知";
}

// 订单结构体
struct Order {
    int        id;          // 唯一编号
    double     x;           // 乘客横坐标
    double     y;           // 乘客纵坐标
    int        createTime;  // 创建时间（模拟秒）
    int        waitTime;    // 已等待时间（模拟秒）
    OrderState state;       // 当前状态
    int        matchedDriverId;  // 匹配司机编号，无匹配时为 -1
    int        gridRow;     // 所属网格行
    int        gridCol;     // 所属网格列

    Order()
        : id(-1), x(0), y(0), createTime(0), waitTime(0),
          state(OrderState::WAITING), matchedDriverId(-1),
          gridRow(0), gridCol(0) {}
};

#endif // RIDEHAULING_ORDER_H
