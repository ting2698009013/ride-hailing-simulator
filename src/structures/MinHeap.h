#ifndef RIDEHAULING_MIN_HEAP_H
#define RIDEHAULING_MIN_HEAP_H

#include <cassert>
#include <cmath>
#include "../models/Driver.h"

// =============================================================================
// MinHeap.h —— 手工实现的最小堆
// 用途：派单时，把搜索到的候选司机按"撮合代价分数"压入堆，堆顶即最优匹配。
// 设计要点：
//   1. 堆元素是 Candidate 结构体，同时保存分数、司机指针、距离等信息，
//      这样弹出堆顶后可以直接拿到匹配所需全部信息，无需二次查找。
//   2. 使用动态数组存储，容量不够时自动扩容（realloc 思路，手动 new/delete）。
//   3. 比较规则：分数低者优先；分数相同时，评分高者优先；评分也相同时，
//      编号小者优先——保证结果稳定可复现（题目明确要求）。
//   4. 堆基于 0 下标，parent(i)=(i-1)/2，left(i)=2i+1，right(i)=2i+2。
// =============================================================================

class MinHeap {
public:
    // 候选项：派单时对每名候选司机计算后压入堆
    struct Candidate {
        double  score;       // 撮合代价分数（越低越优）
        Driver* driver;      // 候选司机指针
        double  distance;    // 距离（像素或网格单位，用于日志）
        int     driverGridRow;
        int     driverGridCol;

        Candidate() : score(0), driver(nullptr), distance(0), driverGridRow(0), driverGridCol(0) {}
        Candidate(double s, Driver* d, double dist, int r, int c)
            : score(s), driver(d), distance(dist), driverGridRow(r), driverGridCol(c) {}
    };

    explicit MinHeap(int initCapacity = 64) : capacity_(initCapacity), count_(0) {
        assert(initCapacity > 0);
        data_ = new Candidate[capacity_];
    }

    ~MinHeap() {
        delete[] data_;
    }

    // 禁用拷贝
    MinHeap(const MinHeap&) = delete;
    MinHeap& operator=(const MinHeap&) = delete;

    // 入堆：先放到末尾再上浮
    void push(const Candidate& c) {
        assert(c.driver != nullptr);
        if (count_ >= capacity_) {
            grow();
        }
        data_[count_] = c;
        siftUp(count_);
        ++count_;
    }

    // 弹出堆顶（最小元素）。空堆返回 false。
    bool pop(Candidate& out) {
        if (count_ == 0) return false;
        out = data_[0];
        --count_;
        if (count_ > 0) {
            data_[0] = data_[count_];
            siftDown(0);
        }
        return true;
    }

    // 查看堆顶（不弹出）
    const Candidate* top() const {
        return (count_ > 0) ? &data_[0] : nullptr;
    }

    void clear() { count_ = 0; }
    bool isEmpty() const { return count_ == 0; }
    int  size()    const { return count_; }

private:
    Candidate* data_;
    int        capacity_;
    int        count_;

    // 扩容：容量翻倍
    void grow() {
        int newCap = capacity_ * 2;
        Candidate* newData = new Candidate[newCap];
        for (int i = 0; i < count_; ++i) {
            newData[i] = data_[i];
        }
        delete[] data_;
        data_ = newData;
        capacity_ = newCap;
    }

    // 比较函数：返回 true 表示 a 比 b 优先（应排在前面）
    // 规则：score 小优先；score 相同 rating 大优先；rating 相同 id 小优先
    static bool isHigherPriority(const Candidate& a, const Candidate& b) {
        if (a.score != b.score) return a.score < b.score;
        if (a.driver->rating != b.driver->rating) return a.driver->rating > b.driver->rating;
        return a.driver->id < b.driver->id;
    }

    // 上浮
    void siftUp(int i) {
        while (i > 0) {
            int parent = (i - 1) / 2;
            if (isHigherPriority(data_[i], data_[parent])) {
                swap(data_[i], data_[parent]);
                i = parent;
            } else {
                break;
            }
        }
    }

    // 下沉
    void siftDown(int i) {
        while (true) {
            int left  = 2 * i + 1;
            int right = 2 * i + 2;
            int best  = i;
            if (left < count_ && isHigherPriority(data_[left], data_[best])) {
                best = left;
            }
            if (right < count_ && isHigherPriority(data_[right], data_[best])) {
                best = right;
            }
            if (best == i) break;
            swap(data_[i], data_[best]);
            i = best;
        }
    }

    static void swap(Candidate& a, Candidate& b) {
        Candidate tmp = a;
        a = b;
        b = tmp;
    }
};

#endif // RIDEHAULING_MIN_HEAP_H
