#ifndef RIDEHAULING_LOGGER_H
#define RIDEHAULING_LOGGER_H

#include <string>
#include "../config.h"

// =============================================================================
// Logger.h —— 手工循环缓冲区日志器
// 用途：保存最近 MAX_LOG_LINES 条日志，供 UI 渲染。
// 实现：固定大小数组 + 头尾指针，满后覆盖最旧日志（FIFO 环形覆盖）。
// =============================================================================

struct LogEntry {
    int         tick;       // 模拟时间（秒）
    std::wstring text;      // 日志文本

    LogEntry() : tick(0) {}
};

class Logger {
public:
    Logger() : head_(0), count_(0) {}

    // 添加一条日志
    void log(int tick, const std::wstring& text) {
        int idx = (head_ + count_) % MAX_LOG_LINES;
        entries_[idx].tick = tick;
        entries_[idx].text = text;
        if (count_ < MAX_LOG_LINES) {
            ++count_;
        } else {
            // 满了，覆盖最旧，头指针前移
            head_ = (head_ + 1) % MAX_LOG_LINES;
        }
    }

    void clear() {
        head_ = 0;
        count_ = 0;
    }

    int size() const { return count_; }

    // 按时间顺序获取第 i 条日志（i 从 0 开始，0 是最旧的）
    const LogEntry& get(int i) const {
        int idx = (head_ + i) % MAX_LOG_LINES;
        return entries_[idx];
    }

private:
    LogEntry entries_[MAX_LOG_LINES];
    int      head_;     // 最旧日志下标
    int      count_;    // 当前日志条数
};

#endif // RIDEHAULING_LOGGER_H
