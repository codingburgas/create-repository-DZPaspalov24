#pragma once
#include "screen.h"
#include "ui.h"
#include "models.h"
#include <vector>

// Statistics & visualization screen. Animated charts:
//  - Grade distribution (BarChart, 5 buckets 2..6)
//  - Subject averages    (BarChart, one bar per subject)
//  - Performance trend   (LineChart, monthly average over time)
//
// All charts re-trigger their entry animation on OnShow().

class StatisticsScreen : public Screen {
public:
    void OnEnter(App& app) override;
    void OnShow(App& app) override;
    void Update(App& app, float dt, Vector2 mouse) override;
    void Draw(App& app) override;
    const char* Name() const override { return "Statistics"; }

private:
    UI::Button backBtn{"Back"};

    UI::BarChart  distChart;
    UI::BarChart  subjectChart;
    UI::LineChart trendChart;

    // Class filter chips (-1 = all)
    std::vector<std::string> classes;
    int activeClass = -1;
    std::vector<UI::Button> classChips;

    float enterT = 0.0f;

    void RebuildData(App& app);
    void RefreshClasses(App& app);

    void DrawHeader(App& app);
    void DrawSummaryStrip(App& app, Rectangle r);
    void DrawDistribution(App& app, Rectangle r);
    void DrawSubjectAverages(App& app, Rectangle r);
    void DrawTrend(App& app, Rectangle r);

    // Helper: returns true if this user passes the active class filter
    bool PassesFilter(const User& u) const;
};
