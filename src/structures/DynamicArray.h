#ifndef RIDEHAULING_DYNAMIC_ARRAY_H
#define RIDEHAULING_DYNAMIC_ARRAY_H

#include <cassert>

// =============================================================================
// DynamicArray.h —— 手工实现的动态数组
// 用途：替代 std::vector，用于保存司机/订单等需要可变长度的对象集合。
// 设计要点：
//   1. 底层是连续数组，容量不够时按 1.5 倍扩容（手动 new/delete + 拷贝）。
//   2. 支持按下标随机访问 O(1)，尾部追加 O(均摊1)。
//   3. 模板类，可存储任意类型（Driver*、Order* 等）。
//   4. 禁用拷贝构造，避免浅拷贝；如需多份请显式实现深拷贝或使用指针。
//   5. 析构时释放内部数组，但不释放元素本身（元素由调用方管理）。
// =============================================================================

template <typename T>
class DynamicArray {
public:
    explicit DynamicArray(int initCapacity = 16)
        : capacity_(initCapacity > 0 ? initCapacity : 16), count_(0) {
        data_ = new T[capacity_];
    }

    ~DynamicArray() {
        delete[] data_;
    }

    // 禁用拷贝构造与赋值
    DynamicArray(const DynamicArray&) = delete;
    DynamicArray& operator=(const DynamicArray&) = delete;

    // 尾部追加元素，容量不足自动扩容
    void push(const T& value) {
        if (count_ >= capacity_) {
            grow();
        }
        data_[count_] = value;
        ++count_;
    }

    // 按下标访问（带边界检查）
    T& operator[](int index) {
        assert(index >= 0 && index < count_);
        return data_[index];
    }

    const T& operator[](int index) const {
        assert(index >= 0 && index < count_);
        return data_[index];
    }

    // 按值删除指定下标元素（用末尾元素填补，不保证顺序）
    // 适用于不关心元素顺序的场景（如司机池）
    void removeAt(int index) {
        assert(index >= 0 && index < count_);
        --count_;
        if (index != count_) {
            data_[index] = data_[count_];
        }
    }

    // 查找元素下标（线性扫描），找不到返回 -1
    // cmp 返回 true 表示匹配
    template <typename Predicate>
    int find(Predicate cmp) const {
        for (int i = 0; i < count_; ++i) {
            if (cmp(data_[i])) return i;
        }
        return -1;
    }

    void clear() { count_ = 0; }

    int  size()     const { return count_; }
    int  capacity() const { return capacity_; }
    bool isEmpty()  const { return count_ == 0; }

    T*       raw()       { return data_; }
    const T* raw() const { return data_; }

private:
    T*  data_;
    int capacity_;
    int count_;

    void grow() {
        int newCap = capacity_ + capacity_ / 2;  // 1.5 倍扩容
        if (newCap <= capacity_) newCap = capacity_ + 1;
        T* newData = new T[newCap];
        for (int i = 0; i < count_; ++i) {
            newData[i] = data_[i];
        }
        delete[] data_;
        data_ = newData;
        capacity_ = newCap;
    }
};

#endif // RIDEHAULING_DYNAMIC_ARRAY_H
