#ifndef RIDEHAULING_SINGLY_LINKED_LIST_H
#define RIDEHAULING_SINGLY_LINKED_LIST_H

#include <cassert>
#include "../models/Driver.h"

// =============================================================================
// SinglyLinkedList.h —— 手工实现的单向链表
// 用途：每个网格内部维护一个"空闲司机链表"，支持 O(1) 头插、按司机编号删除。
// 设计要点：
//   1. 链表节点只保存 Driver 指针（司机对象本身由模拟器统一数组管理），
//      这样删除/移动司机时无需拷贝整个 Driver 对象，也避免双重释放。
//   2. 禁用拷贝构造和赋值，避免浅拷贝导致重复释放；如需拷贝需显式实现深拷贝。
//   3. 所有边界条件（空表、单节点、尾部删除）均做了处理。
// =============================================================================

class SinglyLinkedList {
public:
    // 链表节点
    struct Node {
        Driver* data;   // 指向司机对象（非拥有，不负责释放）
        Node*   next;
        Node(Driver* d) : data(d), next(nullptr) {}
    };

    SinglyLinkedList() : head_(nullptr), count_(0) {}

    // 禁用拷贝构造与赋值，防止浅拷贝导致悬空指针/重复释放
    SinglyLinkedList(const SinglyLinkedList&) = delete;
    SinglyLinkedList& operator=(const SinglyLinkedList&) = delete;

    ~SinglyLinkedList() {
        clear();
    }

    // 释放所有节点（注意：只释放链表节点，不释放 Driver 对象本身）
    void clear() {
        Node* cur = head_;
        while (cur != nullptr) {
            Node* tmp = cur;
            cur = cur->next;
            delete tmp;
        }
        head_ = nullptr;
        count_ = 0;
    }

    // 头插法：新空闲司机加入网格时使用，O(1)
    void pushFront(Driver* d) {
        assert(d != nullptr);
        Node* node = new Node(d);
        node->next = head_;
        head_ = node;
        ++count_;
    }

    // 按司机编号删除节点，返回被删除的 Driver 指针；找不到返回 nullptr
    // 说明：派单成功后需把司机从网格链表中摘除
    Driver* removeById(int driverId) {
        Node* prev = nullptr;
        Node* cur  = head_;
        while (cur != nullptr) {
            if (cur->data->id == driverId) {
                Node* next = cur->next;
                Driver* d = cur->data;
                delete cur;
                if (prev == nullptr) {
                    head_ = next;       // 删除的是头节点
                } else {
                    prev->next = next;  // 删除中间或尾部节点
                }
                --count_;
                return d;
            }
            prev = cur;
            cur = cur->next;
        }
        return nullptr;  // 未找到
    }

    // 取头节点司机（不删除），用于调度时优先取出最早加入的司机
    Driver* front() const {
        return (head_ != nullptr) ? head_->data : nullptr;
    }

    // 弹出头节点司机
    Driver* popFront() {
        if (head_ == nullptr) return nullptr;
        Node* node = head_;
        Driver* d = node->data;
        head_ = head_->next;
        delete node;
        --count_;
        return d;
    }

    // 遍历：对每个司机执行回调（用于搜索候选司机、统计等）
    // 回调签名为 void(const Driver*)
    template <typename Func>
    void forEach(Func func) const {
        Node* cur = head_;
        while (cur != nullptr) {
            func(cur->data);
            cur = cur->next;
        }
    }

    // 查找指定编号的司机是否存在
    bool contains(int driverId) const {
        Node* cur = head_;
        while (cur != nullptr) {
            if (cur->data->id == driverId) return true;
            cur = cur->next;
        }
        return false;
    }

    bool isEmpty() const { return head_ == nullptr; }
    int  size()    const { return count_; }

    Node* head() const { return head_; }

private:
    Node* head_;
    int   count_;
};

#endif // RIDEHAULING_SINGLY_LINKED_LIST_H
