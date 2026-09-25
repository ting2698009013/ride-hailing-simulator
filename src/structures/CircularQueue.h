#ifndef RIDEHAULING_CIRCULAR_QUEUE_H
#define RIDEHAULING_CIRCULAR_QUEUE_H

#include <cassert>
#include "../models/Order.h"

// =============================================================================
// CircularQueue.h —— 手工实现的循环队列（环形缓冲区）
// 用途：保存等待派单的乘客订单，遵循先来先服务（FIFO）。
// 设计要点：
//   1. 使用固定容量数组 + 头/尾指针实现环形结构，充分利用空间。
//   2. "队列满"条件保留一个空位（(rear+1)%cap == front），以区分空和满，
//      这是经典循环队列实现方式，便于课程讲解。
//   3. 提供按订单编号删除（用于超时取消）和重入队（派单失败重新排队）。
//   4. 队列内部保存的是 Order 指针，Order 对象由模拟器统一管理。
// =============================================================================

class CircularQueue {
public:
    // capacity 为队列容量；实际可用容量为 capacity-1（留一个空位区分空满）
    explicit CircularQueue(int capacity)
        : capacity_(capacity), front_(0), rear_(0), count_(0) {
        assert(capacity > 1);
        data_ = new Order * [capacity];
        for (int i = 0; i < capacity; ++i) data_[i] = nullptr;
    }

    ~CircularQueue() {
        delete[] data_;
    }

    // 禁用拷贝
    CircularQueue(const CircularQueue&) = delete;
    CircularQueue& operator=(const CircularQueue&) = delete;

    // 入队。队列满时返回 false（拒绝入队），调用方需处理
    bool push(Order* o) {
        assert(o != nullptr);
        if (isFull()) return false;
        data_[rear_] = o;
        rear_ = (rear_ + 1) % capacity_;
        ++count_;
        return true;
    }

    // 出队，返回队首订单指针；空队返回 nullptr
    Order* pop() {
        if (isEmpty()) return nullptr;
        Order* o = data_[front_];
        data_[front_] = nullptr;
        front_ = (front_ + 1) % capacity_;
        --count_;
        return o;
    }

    // 查看队首（不出队）
    Order* peek() const {
        if (isEmpty()) return nullptr;
        return data_[front_];
    }

    // 按订单编号从队列中移除（用于超时取消）。
    // 实现方式：遍历队列，找到目标后将其后元素前移一位以保持顺序。
    bool removeById(int orderId) {
        if (isEmpty()) return false;
        int idx = front_;
        for (int i = 0; i < count_; ++i) {
            if (data_[idx] != nullptr && data_[idx]->id == orderId) {
                // 从 idx 开始，把后续元素依次前移
                int cur = idx;
                for (int j = 0; j < count_ - 1 - i; ++j) {
                    int nxt = (cur + 1) % capacity_;
                    data_[cur] = data_[nxt];
                    cur = nxt;
                }
                data_[cur] = nullptr;
                rear_ = (rear_ - 1 + capacity_) % capacity_;
                --count_;
                return true;
            }
            idx = (idx + 1) % capacity_;
        }
        return false;
    }

    // 遍历队列中的订单（用于批量派单前取出，或更新等待时间）
    template <typename Func>
    void forEach(Func func) const {
        if (isEmpty()) return;
        int idx = front_;
        for (int i = 0; i < count_; ++i) {
            func(data_[idx]);
            idx = (idx + 1) % capacity_;
        }
    }

    bool isEmpty() const { return count_ == 0; }
    bool isFull()  const { return count_ == capacity_ - 1; }
    int  size()    const { return count_; }
    int  capacity() const { return capacity_ - 1; }  // 可用容量

    // 清空队列（不释放 Order 对象，对象由 Simulator 统一管理）
    void clear() {
        front_ = 0;
        rear_ = 0;
        count_ = 0;
        for (int i = 0; i < capacity_; ++i) data_[i] = nullptr;
    }

private:
    Order** data_;
    int     capacity_;  // 数组总长度
    int     front_;     // 队首下标
    int     rear_;      // 下一个入队位置
    int     count_;     // 当前元素个数（便于快速判空满）
};

#endif // RIDEHAULING_CIRCULAR_QUEUE_H
