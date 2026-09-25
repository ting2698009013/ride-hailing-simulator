#ifndef RIDEHAULING_EASYX_RENDERER_H
#define RIDEHAULING_EASYX_RENDERER_H

#include "../config.h"
#include "../core/Simulator.h"

// =============================================================================
// EasyXRenderer.h —— EasyX 图形界面渲染器
// 职责：
//   1. 绘制 100x100 网格供需热力图（左上）。
//   2. 绘制右侧实时看板（统计指标）。
//   3. 绘制下方/右侧实时日志。
//   4. 绘制交互按钮并处理点击。
//   5. 处理鼠标点击地图查看网格详情。
// UI 层只负责绘制和输入事件，不直接实现核心派单算法。
// =============================================================================

// 按钮定义
struct Button {
    int x, y, w, h;
    const wchar_t* label;
    int id;         // 按钮标识
    bool hover;     // 鼠标悬停
    bool active;    // 当前激活状态（用于切换类按钮）

    Button() : x(0), y(0), w(0), h(0), label(L""), id(0), hover(false), active(false) {}
    Button(int bx, int by, int bw, int bh, const wchar_t* bl, int bid)
        : x(bx), y(by), w(bw), h(bh), label(bl), id(bid), hover(false), active(false) {}

    bool contains(int mx, int my) const {
        return mx >= x && mx < x + w && my >= y && my < y + h;
    }
};

// 按钮 ID 枚举
enum ButtonId {
    BTN_PAUSE = 1,
    BTN_STEP,
    BTN_RESET,
    BTN_SPEED_DOWN,
    BTN_SPEED_UP,
    BTN_HOTSPOT,
    BTN_MODE,
    BTN_CLEAR_LOG,
    BTN_TOGGLE_POINTS,
};

class EasyXRenderer {
public:
    EasyXRenderer();
    ~EasyXRenderer();

    // 初始化窗口
    void initWindow();

    // 渲染一帧
    void render(Simulator& sim);

    // 处理鼠标/键盘事件，返回是否需要退出
    bool handleEvents(Simulator& sim);

    // 检查是否到了下一个 tick 时间（基于累计时间）
    bool shouldTick(double speed);

private:
    Button buttons_[16];
    int    buttonCount_;

    int    selectedRow_;   // 鼠标选中的网格行
    int    selectedCol_;   // 鼠标选中的网格列
    bool   hasSelection_;

    // 时间累计（用于控制 tick 频率）
    long long lastTickTime_;  // 上次 tick 的时刻（毫秒）

    // ---- 绘制方法 ----
    void drawHeatmap(Simulator& sim);
    void drawDashboard(Simulator& sim);
    void drawLog(Simulator& sim);
    void drawButtons(Simulator& sim);
    void drawGridDetail(Simulator& sim);
    void drawRepositionLines(Simulator& sim);
    void drawPoints(Simulator& sim);

    // 辅助：根据供需差值计算颜色
    void getCellColor(int imbalance, int& r, int& g, int& b);

    // 辅助：绘制按钮
    void drawButton(const Button& btn);

    // 初始化按钮布局
    void initButtons();

    // 获取当前时间（毫秒）
    long long nowMillis() const;
};

#endif // RIDEHAULING_EASYX_RENDERER_H
