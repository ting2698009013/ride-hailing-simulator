#include "EasyXRenderer.h"
#include <graphics.h>
#include <cmath>
#include <chrono>
#include <sstream>
#include <ctime>
#include <string>
#include <conio.h>

// 自动链接 EasyX 库（MSVC 专用）
// EasyX 安装后会把 easyx.lib 放到 VC 的 lib 目录，此处自动链接
#pragma comment(lib, "easyx.lib")

// =============================================================================
// EasyXRenderer.cpp —— EasyX 图形界面实现
// 注意：
//   1. 使用 BeginBatchDraw/FlushBatchDraw/EndBatchDraw 减少闪烁。
//   2. 界面文字使用中文，配合 /utf-8 编译选项和 Unicode 字符集。
//   3. 坐标计算严格遵守 config.h 中的常量，避免越界。
// =============================================================================

EasyXRenderer::EasyXRenderer()
    : buttonCount_(0), selectedRow_(-1), selectedCol_(-1),
      hasSelection_(false), lastTickTime_(0) {
    initButtons();
}

EasyXRenderer::~EasyXRenderer() {
    // EasyX 窗口在程序结束时自动关闭
}

void EasyXRenderer::initWindow() {
    // 初始化 EasyX 图形窗口
    initgraph(WINDOW_WIDTH, WINDOW_HEIGHT);
    setbkcolor(RGB(30, 30, 38));  // 深色背景
    lastTickTime_ = nowMillis();
}

void EasyXRenderer::initButtons() {
    // 按钮布局：放在地图右侧、看板下方的控制区
    int baseX = MAP_X + MAP_PIXEL_SIZE + 20;  // 地图右侧
    int baseY = 480;  // 看板下方
    int bw = 130;
    int bh = 36;
    int gap = 8;

    // 第一行
    buttons_[0] = Button(baseX,            baseY, bw, bh, L"暂停/继续",   BTN_PAUSE);
    buttons_[1] = Button(baseX + bw + gap, baseY, bw, bh, L"单步执行",   BTN_STEP);
    // 第二行
    buttons_[2] = Button(baseX,            baseY + bh + gap, bw, bh, L"重置模拟",   BTN_RESET);
    buttons_[3] = Button(baseX + bw + gap, baseY + bh + gap, bw, bh, L"减速", BTN_SPEED_DOWN);
    // 第三行
    buttons_[4] = Button(baseX,            baseY + 2*(bh+gap), bw, bh, L"加速", BTN_SPEED_UP);
    buttons_[5] = Button(baseX + bw + gap, baseY + 2*(bh+gap), bw, bh, L"热点: 开", BTN_HOTSPOT);
    // 第四行
    buttons_[6] = Button(baseX,            baseY + 3*(bh+gap), bw, bh, L"模式: 基础", BTN_MODE);
    buttons_[7] = Button(baseX + bw + gap, baseY + 3*(bh+gap), bw, bh, L"清空日志",   BTN_CLEAR_LOG);
    // 第五行
    buttons_[8] = Button(baseX,            baseY + 4*(bh+gap), bw, bh, L"显示/隐藏点", BTN_TOGGLE_POINTS);

    buttonCount_ = 9;
}

long long EasyXRenderer::nowMillis() const {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch());
    return ms.count();
}

bool EasyXRenderer::shouldTick(double speed) {
    long long now = nowMillis();
    // 间隔 = BASE_TICK_MS / speed，speed 越大间隔越短
    double interval = BASE_TICK_MS / speed;
    if (now - lastTickTime_ >= static_cast<long long>(interval)) {
        lastTickTime_ = now;
        return true;
    }
    return false;
}

void EasyXRenderer::getCellColor(int imbalance, int& r, int& g, int& b) {
    // imbalance > 0：订单多 -> 红色（越深越严重）
    // imbalance < 0：司机多 -> 绿色（越深越富余）
    // imbalance ≈ 0：供需平衡 -> 灰色
    if (imbalance == 0) {
        r = 60; g = 60; b = 70;  // 深灰
    } else if (imbalance > 0) {
        int intensity = imbalance * 30;
        if (intensity > 180) intensity = 180;
        r = 80 + intensity;       // 红
        g = 40;
        b = 40;
    } else {
        int intensity = (-imbalance) * 30;
        if (intensity > 180) intensity = 180;
        r = 40;
        g = 80 + intensity;       // 绿
        b = 40;
    }
}

void EasyXRenderer::drawHeatmap(Simulator& sim) {
    const CityGrid& grid = sim.getGrid();

    // 绘制背景框
    setfillcolor(RGB(20, 20, 26));
    fillrectangle(MAP_X - 2, MAP_Y - 2,
                  MAP_X + MAP_PIXEL_SIZE + 2, MAP_Y + MAP_PIXEL_SIZE + 2);

    // 绘制每个网格
    for (int r = 0; r < GRID_COUNT; ++r) {
        for (int c = 0; c < GRID_COUNT; ++c) {
            const GridCell& cell = grid.getCell(r, c);
            int cr, cg, cb;
            getCellColor(cell.imbalance, cr, cg, cb);

            int x = MAP_X + c * CELL_PIXEL;
            int y = MAP_Y + r * CELL_PIXEL;

            setfillcolor(RGB(cr, cg, cb));
            fillrectangle(x, y, x + CELL_PIXEL, y + CELL_PIXEL);

            // 热点区域用黄色边框标记
            if (cell.isHotspot) {
                setlinecolor(RGB(255, 200, 0));
                setlinestyle(PS_SOLID, 1);
                rectangle(x, y, x + CELL_PIXEL, y + CELL_PIXEL);
            }
            // 警报区域用红色粗边框
            if (cell.isAlert) {
                setlinecolor(RGB(255, 50, 50));
                setlinestyle(PS_SOLID, 2);
                rectangle(x, y, x + CELL_PIXEL, y + CELL_PIXEL);
            }
        }
    }

    setlinestyle(PS_SOLID, 1);

    // 绘制地图边框
    setlinecolor(RGB(120, 120, 140));
    rectangle(MAP_X, MAP_Y, MAP_X + MAP_PIXEL_SIZE, MAP_Y + MAP_PIXEL_SIZE);

    // 标题
    settextcolor(RGB(220, 220, 230));
    settextstyle(16, 0, L"微软雅黑");
    outtextxy(MAP_X, MAP_Y - 18, L"城市网格供需热力图（100x100）  红=订单多  绿=司机多  黄框=热点  红框=警报");

    // 图例
    int legendX = MAP_X;
    int legendY = MAP_Y + MAP_PIXEL_SIZE + 8;
    setfillcolor(RGB(200, 40, 40));
    fillrectangle(legendX, legendY, legendX + 16, legendY + 12);
    outtextxy(legendX + 20, legendY - 2, L"订单多");
    setfillcolor(RGB(40, 200, 40));
    fillrectangle(legendX + 90, legendY, legendX + 106, legendY + 12);
    outtextxy(legendX + 110, legendY - 2, L"司机多");
    setfillcolor(RGB(60, 60, 70));
    fillrectangle(legendX + 180, legendY, legendX + 196, legendY + 12);
    outtextxy(legendX + 200, legendY - 2, L"平衡");
}

void EasyXRenderer::drawPoints(Simulator& sim) {
    if (!sim.getShowPoints()) return;

    // 绘制司机点（小蓝点）和订单点（小红点）
    const DynamicArray<Driver*>& drivers = sim.getDrivers();
    const DynamicArray<Order*>& orders = sim.getOrders();

    // 司机：空闲=蓝，服务中=黄，调度中=紫
    for (int i = 0; i < drivers.size(); ++i) {
        Driver* d = drivers[i];
        int px = MAP_X + static_cast<int>(d->x / CELL_SIZE * CELL_PIXEL);
        int py = MAP_Y + static_cast<int>(d->y / CELL_SIZE * CELL_PIXEL);
        if (px < MAP_X || px >= MAP_X + MAP_PIXEL_SIZE) continue;
        if (py < MAP_Y || py >= MAP_Y + MAP_PIXEL_SIZE) continue;

        int color;
        switch (d->state) {
            case DriverState::IDLE:         color = RGB(80, 180, 255); break;  // 蓝
            case DriverState::ASSIGNED:     color = RGB(255, 200, 80); break;  // 黄
            case DriverState::SERVING:      color = RGB(255, 200, 80); break;  // 黄
            case DriverState::REPOSITIONING:color = RGB(200, 100, 255); break; // 紫
            default:                        color = RGB(200, 200, 200); break;
        }
        setfillcolor(color);
        fillcircle(px, py, 1);
    }

    // 等待中的订单：小红点
    for (int i = 0; i < orders.size(); ++i) {
        Order* o = orders[i];
        if (o->state != OrderState::WAITING) continue;
        int px = MAP_X + static_cast<int>(o->x / CELL_SIZE * CELL_PIXEL);
        int py = MAP_Y + static_cast<int>(o->y / CELL_SIZE * CELL_PIXEL);
        if (px < MAP_X || px >= MAP_X + MAP_PIXEL_SIZE) continue;
        if (py < MAP_Y || py >= MAP_Y + MAP_PIXEL_SIZE) continue;

        setfillcolor(RGB(255, 80, 80));
        fillcircle(px, py, 1);
    }
}

void EasyXRenderer::drawRepositionLines(Simulator& sim) {
    // 绘制调度连线（从来源网格到目标网格）
    const DynamicArray<RepositionEvent>& events = sim.getRepositionEvents();
    for (int i = 0; i < events.size(); ++i) {
        const RepositionEvent& ev = events[i];
        int x1 = MAP_X + ev.fromCol * CELL_PIXEL + CELL_PIXEL / 2;
        int y1 = MAP_Y + ev.fromRow * CELL_PIXEL + CELL_PIXEL / 2;
        int x2 = MAP_X + ev.toCol * CELL_PIXEL + CELL_PIXEL / 2;
        int y2 = MAP_Y + ev.toRow * CELL_PIXEL + CELL_PIXEL / 2;

        // 根据剩余帧数调整透明度（用颜色深浅模拟）
        int alpha = ev.remainTicks * 8;
        if (alpha > 255) alpha = 255;
        int c = RGB(200, 100, 255);
        setlinecolor(c);
        setlinestyle(PS_SOLID, 2);
        line(x1, y1, x2, y2);
        // 箭头终点
        setfillcolor(c);
        fillcircle(x2, y2, 2);
    }
    setlinestyle(PS_SOLID, 1);
}

void EasyXRenderer::drawDashboard(Simulator& sim) {
    int panelX = MAP_X + MAP_PIXEL_SIZE + 20;
    int panelY = 20;
    int panelW = WINDOW_WIDTH - panelX - 20;
    int panelH = 440;

    // 面板背景
    setfillcolor(RGB(40, 42, 52));
    fillrectangle(panelX, panelY, panelX + panelW, panelY + panelH);
    setlinecolor(RGB(80, 80, 100));
    rectangle(panelX, panelY, panelX + panelW, panelY + panelH);

    settextcolor(RGB(240, 240, 250));
    settextstyle(18, 0, L"微软雅黑");
    outtextxy(panelX + 10, panelY + 8, L"实时看板");

    // 分隔线
    setlinecolor(RGB(80, 80, 100));
    line(panelX + 10, panelY + 32, panelX + panelW - 10, panelY + 32);

    settextstyle(15, 0, L"Consolas");

    // 获取当前系统时间
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_s(&tm, &t);
    wchar_t timeBuf[64];
    wcsftime(timeBuf, 64, L"%Y-%m-%d %H:%M:%S", &tm);

    const SimulatorStats& st = sim.getStats();
    const DispatchEngine& eng = sim.getEngine();

    // 逐行绘制指标
    int lineY = panelY + 40;
    int lineHeight = 22;

    struct Metric {
        const wchar_t* label;
        std::wstring value;
    };

    Metric metrics[] = {
        { L"系统时间：", timeBuf },
        { L"模拟时间：", std::to_wstring(sim.getSimTime()) + L" 秒" },
        { L"随机种子：", std::to_wstring(sim.getSeed()) },
        { L"当前速度：", std::to_wstring(sim.getSpeed()) + L"x" },
        { L"订单队列：", std::to_wstring(sim.getWaitingQueueSize()) + L" 单" },
        { L"空闲司机：", std::to_wstring(sim.getIdleDriverCount()) + L" 人" },
        { L"服务中：",   std::to_wstring(sim.getServingDriverCount()) + L" 人" },
        { L"调度中：",   std::to_wstring(sim.getRepositioningDriverCount()) + L" 人" },
        { L"热点警报：", std::to_wstring(sim.getAlertGridCount()) + L" 个网格" },
        { L"已生成订单：", std::to_wstring(st.totalOrders) },
        { L"派单成功：", std::to_wstring(st.matchedOrders) },
        { L"取消订单：", std::to_wstring(st.cancelledOrders) },
        { L"完成订单：", std::to_wstring(st.completedOrders) },
        { L"派单成功率：", [st]() -> std::wstring {
            wchar_t buf[32];
            swprintf(buf, 32, L"%.1f%%", st.successRate());
            return buf;
        }() },
        { L"总撮合耗时：", std::to_wstring(eng.getTotalMicros() / 1000) + L" ms" },
        { L"平均耗时：", [eng]() -> std::wstring {
            double avg = eng.getMatchCount() > 0
                ? static_cast<double>(eng.getTotalMicros()) / eng.getMatchCount() / 1000.0
                : 0.0;
            wchar_t buf[32];
            swprintf(buf, 32, L"%.3f ms", avg);
            return buf;
        }() },
        { L"最近耗时：", std::to_wstring(eng.getLastMicros() / 1000) + L" ms" },
    };

    for (int i = 0; i < sizeof(metrics) / sizeof(metrics[0]); ++i) {
        settextcolor(RGB(160, 170, 190));
        outtextxy(panelX + 12, lineY, metrics[i].label);
        settextcolor(RGB(240, 240, 250));
        outtextxy(panelX + 110, lineY, metrics[i].value.c_str());
        lineY += lineHeight;
    }
}

void EasyXRenderer::drawLog(Simulator& sim) {
    // 日志区域：窗口底部
    int logX = MAP_X;
    int logY = MAP_Y + MAP_PIXEL_SIZE + 30;
    int logW = MAP_PIXEL_SIZE;
    int logH = WINDOW_HEIGHT - logY - 10;

    setfillcolor(RGB(30, 32, 40));
    fillrectangle(logX, logY, logX + logW, logY + logH);
    setlinecolor(RGB(80, 80, 100));
    rectangle(logX, logY, logX + logW, logY + logH);

    settextcolor(RGB(200, 200, 210));
    settextstyle(14, 0, L"Consolas");

    const Logger& logger = sim.getLogger();
    int count = logger.size();
    int lineH = 16;
    int maxLines = (logH - 10) / lineH;
    int startIdx = count > maxLines ? count - maxLines : 0;

    int yPos = logY + 5;
    for (int i = startIdx; i < count; ++i) {
        const LogEntry& entry = logger.get(i);
        std::wstring line = L"[" + std::to_wstring(entry.tick) + L"s] " + entry.text;
        // 根据日志类型着色
        if (entry.text.find(L"派单成功") != std::wstring::npos) {
            settextcolor(RGB(100, 255, 100));
        } else if (entry.text.find(L"取消") != std::wstring::npos) {
            settextcolor(RGB(255, 120, 120));
        } else if (entry.text.find(L"调度") != std::wstring::npos) {
            settextcolor(RGB(200, 150, 255));
        } else if (entry.text.find(L"警报") != std::wstring::npos ||
                   entry.text.find(L"热点") != std::wstring::npos) {
            settextcolor(RGB(255, 180, 80));
        } else if (entry.text.find(L"完成") != std::wstring::npos) {
            settextcolor(RGB(120, 200, 255));
        } else {
            settextcolor(RGB(200, 200, 210));
        }
        outtextxy(logX + 8, yPos, line.c_str());
        yPos += lineH;
        if (yPos >= logY + logH - 5) break;
    }
}

void EasyXRenderer::drawButton(const Button& btn) {
    COLORREF bg, border, text;
    if (btn.active) {
        bg = RGB(70, 110, 160);
        border = RGB(120, 180, 255);
        text = RGB(255, 255, 255);
    } else if (btn.hover) {
        bg = RGB(60, 64, 78);
        border = RGB(140, 140, 170);
        text = RGB(240, 240, 250);
    } else {
        bg = RGB(45, 48, 60);
        border = RGB(90, 90, 110);
        text = RGB(210, 210, 220);
    }
    setfillcolor(bg);
    fillrectangle(btn.x, btn.y, btn.x + btn.w, btn.y + btn.h);
    setlinecolor(border);
    rectangle(btn.x, btn.y, btn.x + btn.w, btn.y + btn.h);
    settextcolor(text);
    settextstyle(14, 0, L"微软雅黑");
    // 文字居中
    int tw = textwidth(btn.label);
    int th = textheight(btn.label);
    outtextxy(btn.x + (btn.w - tw) / 2, btn.y + (btn.h - th) / 2, btn.label);
}

void EasyXRenderer::drawButtons(Simulator& sim) {
    // 根据模拟器状态更新按钮标签和激活态
    buttons_[0].label = sim.isPaused() ? L"继续运行" : L"暂停模拟";
    buttons_[0].active = sim.isPaused();

    buttons_[5].label = sim.isHotspotEnabled() ? L"热点: 开" : L"热点: 关";
    buttons_[5].active = sim.isHotspotEnabled();

    buttons_[6].label = (sim.getMatchMode() == MatchMode::Traffic)
        ? L"模式: 路况" : L"模式: 基础";
    buttons_[6].active = (sim.getMatchMode() == MatchMode::Traffic);

    buttons_[8].label = sim.getShowPoints() ? L"隐藏点" : L"显示点";
    buttons_[8].active = sim.getShowPoints();

    // 注意：鼠标悬停检测在 handleEvents 中统一处理，此处不消费鼠标事件
    for (int i = 0; i < buttonCount_; ++i) {
        drawButton(buttons_[i]);
    }

    // 显示当前速度
    settextcolor(RGB(180, 180, 200));
    settextstyle(14, 0, L"微软雅黑");
    outtextxy(buttons_[3].x, buttons_[3].y + buttons_[3].h + 4,
              (L"速度: " + std::to_wstring(sim.getSpeed()) + L"x").c_str());
}

void EasyXRenderer::drawGridDetail(Simulator& sim) {
    if (!hasSelection_) return;
    if (selectedRow_ < 0 || selectedRow_ >= GRID_COUNT) return;
    if (selectedCol_ < 0 || selectedCol_ >= GRID_COUNT) return;

    Simulator::GridInfo info = sim.getGridInfo(selectedRow_, selectedCol_);

    // 在选中网格上绘制高亮框
    int x = MAP_X + selectedCol_ * CELL_PIXEL;
    int y = MAP_Y + selectedRow_ * CELL_PIXEL;
    setlinecolor(RGB(255, 255, 0));
    setlinestyle(PS_SOLID, 2);
    rectangle(x, y, x + CELL_PIXEL, y + CELL_PIXEL);
    setlinestyle(PS_SOLID, 1);

    // 在地图右上角显示详情
    int boxX = MAP_X + MAP_PIXEL_SIZE - 160;
    int boxY = MAP_Y + 10;
    setfillcolor(RGB(0, 0, 0));
    fillrectangle(boxX, boxY, boxX + 150, boxY + 95);
    setlinecolor(RGB(255, 255, 0));
    rectangle(boxX, boxY, boxX + 150, boxY + 95);

    settextstyle(13, 0, L"微软雅黑");
    settextcolor(RGB(255, 255, 100));
    std::wstring title = L"网格(" + std::to_wstring(info.row) + L"," +
                         std::to_wstring(info.col) + L")";
    outtextxy(boxX + 8, boxY + 5, title.c_str());

    settextcolor(RGB(220, 220, 230));
    outtextxy(boxX + 8, boxY + 24,
              (L"订单数: " + std::to_wstring(info.orderCount)).c_str());
    outtextxy(boxX + 8, boxY + 40,
              (L"司机数: " + std::to_wstring(info.driverCount)).c_str());
    outtextxy(boxX + 8, boxY + 56,
              (L"供需差: " + std::to_wstring(info.imbalance)).c_str());
    outtextxy(boxX + 8, boxY + 72,
              info.isAlert ? L"状态: 警报" :
              (info.isHotspot ? L"状态: 热点" : L"状态: 正常"));
}

void EasyXRenderer::render(Simulator& sim) {
    BeginBatchDraw();

    // 清屏
    setfillcolor(RGB(30, 30, 38));
    fillrectangle(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    drawHeatmap(sim);
    drawPoints(sim);
    drawRepositionLines(sim);
    drawGridDetail(sim);
    drawDashboard(sim);
    drawLog(sim);
    drawButtons(sim);

    FlushBatchDraw();
    EndBatchDraw();
}

bool EasyXRenderer::handleEvents(Simulator& sim) {
    // 统一处理键盘：所有按键在一个循环中处理，避免被前一个循环消费
    while (_kbhit()) {
        int key = _getch();
        if (key == 27) {  // ESC 退出
            return true;
        }
        if (key == ' ') {  // 空格 暂停/继续
            sim.setPaused(!sim.isPaused());
        }
        else if (key == 's' || key == 'S') {  // S 单步
            if (sim.isPaused()) {
                sim.tick();
            }
        }
        else if (key == 'r' || key == 'R') {  // R 重置
            sim.reset();
        }
        else if (key == '1' && hasSelection_) {  // 1 加单
            sim.addOrderAtGrid(selectedRow_, selectedCol_);
        }
        else if (key == '2' && hasSelection_) {  // 2 加车
            sim.addDriverAtGrid(selectedRow_, selectedCol_);
        }
    }

    // 处理鼠标
    while (MouseHit()) {
        MOUSEMSG m = GetMouseMsg();

        if (m.uMsg == WM_MOUSEMOVE) {
            // 更新悬停状态
            for (int i = 0; i < buttonCount_; ++i) {
                buttons_[i].hover = buttons_[i].contains(m.x, m.y);
            }
        }

        if (m.uMsg == WM_LBUTTONDOWN) {
            // 检查是否点击了按钮
            for (int i = 0; i < buttonCount_; ++i) {
                if (buttons_[i].contains(m.x, m.y)) {
                    switch (buttons_[i].id) {
                        case BTN_PAUSE:
                            sim.setPaused(!sim.isPaused());
                            break;
                        case BTN_STEP:
                            if (sim.isPaused()) sim.tick();
                            break;
                        case BTN_RESET:
                            sim.reset();
                            break;
                        case BTN_SPEED_DOWN: {
                            double cur = sim.getSpeed();
                            // 找到当前速度在选项中的位置，减一档
                            int idx = 0;
                            for (int s = 0; s < SPEED_OPTION_COUNT; ++s) {
                                if (SPEED_OPTIONS[s] == cur) { idx = s; break; }
                            }
                            if (idx > 0) sim.setSpeed(SPEED_OPTIONS[idx - 1]);
                            break;
                        }
                        case BTN_SPEED_UP: {
                            double cur = sim.getSpeed();
                            int idx = 0;
                            for (int s = 0; s < SPEED_OPTION_COUNT; ++s) {
                                if (SPEED_OPTIONS[s] == cur) { idx = s; break; }
                            }
                            if (idx < SPEED_OPTION_COUNT - 1) sim.setSpeed(SPEED_OPTIONS[idx + 1]);
                            break;
                        }
                        case BTN_HOTSPOT:
                            sim.setHotspotEnabled(!sim.isHotspotEnabled());
                            break;
                        case BTN_MODE:
                            sim.setMatchMode(
                                sim.getMatchMode() == MatchMode::Basic
                                    ? MatchMode::Traffic
                                    : MatchMode::Basic);
                            break;
                        case BTN_CLEAR_LOG:
                            sim.getLoggerMut().clear();
                            break;
                        case BTN_TOGGLE_POINTS:
                            sim.setShowPoints(!sim.getShowPoints());
                            break;
                    }
                    return false;  // 消费了事件
                }
            }

            // 检查是否点击了地图
            if (m.x >= MAP_X && m.x < MAP_X + MAP_PIXEL_SIZE &&
                m.y >= MAP_Y && m.y < MAP_Y + MAP_PIXEL_SIZE) {
                int col = (m.x - MAP_X) / CELL_PIXEL;
                int row = (m.y - MAP_Y) / CELL_PIXEL;
                if (row >= 0 && row < GRID_COUNT && col >= 0 && col < GRID_COUNT) {
                    selectedRow_ = row;
                    selectedCol_ = col;
                    hasSelection_ = true;
                }
            }
        }

        if (m.uMsg == WM_RBUTTONDOWN) {
            // 右键点击地图：在该网格手动增加订单
            if (m.x >= MAP_X && m.x < MAP_X + MAP_PIXEL_SIZE &&
                m.y >= MAP_Y && m.y < MAP_Y + MAP_PIXEL_SIZE) {
                int col = (m.x - MAP_X) / CELL_PIXEL;
                int row = (m.y - MAP_Y) / CELL_PIXEL;
                if (row >= 0 && row < GRID_COUNT && col >= 0 && col < GRID_COUNT) {
                    sim.addOrderAtGrid(row, col);
                }
            }
        }
    }

    return false;
}
