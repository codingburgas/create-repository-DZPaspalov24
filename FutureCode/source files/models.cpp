// MSVC's CRT flags localtime/strftime/strcpy as "unsafe" and refuses to
// compile by default. We already use the *_s/*_r variants, but defining
// this guards us against any other CRT call we add later.
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "models.h"
#include "i18n.h"
#include <algorithm>
#include <cstdio>
#include <ctime>

namespace Models {

float StudentSubjectAverage(int studentId, int subjectId, const std::vector<Grade>& grades) {
    int sum = 0, cnt = 0;
    for (const auto& g : grades) {
        if (g.studentId == studentId && g.subjectId == subjectId) {
            sum += g.value; cnt++;
        }
    }
    return cnt == 0 ? -1.0f : (float)sum / (float)cnt;
}

float StudentOverallAverage(int studentId, const std::vector<Grade>& grades) {
    int sum = 0, cnt = 0;
    for (const auto& g : grades) {
        if (g.studentId == studentId) { sum += g.value; cnt++; }
    }
    return cnt == 0 ? -1.0f : (float)sum / (float)cnt;
}

float SubjectClassAverage(int subjectId, const std::vector<Grade>& grades) {
    int sum = 0, cnt = 0;
    for (const auto& g : grades) {
        if (g.subjectId == subjectId) { sum += g.value; cnt++; }
    }
    return cnt == 0 ? -1.0f : (float)sum / (float)cnt;
}

void GradeDistribution(const std::vector<Grade>& grades, int out[5]) {
    for (int i = 0; i < 5; ++i) out[i] = 0;
    for (const auto& g : grades) {
        int idx = g.value - 2;
        if (idx >= 0 && idx <= 4) out[idx]++;
    }
}

float TrendDelta(int studentId, int subjectId, const std::vector<Grade>& grades) {
    // Find two most recent for that pair
    std::vector<const Grade*> filtered;
    filtered.reserve(8);
    for (const auto& g : grades) {
        if (g.studentId == studentId && g.subjectId == subjectId) filtered.push_back(&g);
    }
    if (filtered.size() < 2) return 0.0f;
    std::sort(filtered.begin(), filtered.end(),
              [](const Grade* a, const Grade* b){ return a->createdAt < b->createdAt; });
    int n = (int)filtered.size();
    return (float)filtered[n-1]->value - (float)filtered[n-2]->value;
}

std::vector<float> RecentGradeSeries(int studentId, const std::vector<Grade>& grades, int maxPoints) {
    std::vector<const Grade*> mine;
    for (const auto& g : grades) if (g.studentId == studentId) mine.push_back(&g);
    std::sort(mine.begin(), mine.end(),
              [](const Grade* a, const Grade* b){ return a->createdAt < b->createdAt; });
    int start = (int)mine.size() - maxPoints;
    if (start < 0) start = 0;
    std::vector<float> out;
    out.reserve(mine.size() - start);
    for (size_t i = (size_t)start; i < mine.size(); ++i) out.push_back((float)mine[i]->value);
    return out;
}

std::string ObfuscatePassword(const std::string& plain) {
    // Simple XOR with a rotating key, then hex-encode. Reversible.
    static const char key[] = "gm-2026-not-secure";
    const int kn = (int)sizeof(key) - 1;
    std::string out; out.reserve(plain.size() * 2);
    char buf[3];
    for (size_t i = 0; i < plain.size(); ++i) {
        unsigned char c = (unsigned char)plain[i] ^ (unsigned char)key[i % kn];
        std::snprintf(buf, sizeof(buf), "%02x", c);
        out.append(buf, 2);
    }
    return out;
}

bool VerifyPassword(const std::string& plain, const std::string& enc) {
    return ObfuscatePassword(plain) == enc;
}

void TodayYMD(int& y, int& m, int& d) {
    std::time_t t = std::time(nullptr);
    std::tm lt{};
#if defined(_WIN32)
    // MSVC flags localtime() as unsafe and refuses to compile by default.
    localtime_s(&lt, &t);
#else
    // POSIX
    localtime_r(&t, &lt);
#endif
    y = lt.tm_year + 1900;
    m = lt.tm_mon + 1;
    d = lt.tm_mday;
}

void NowYMDHM(int& y, int& m, int& d, int& hr, int& mn) {
    std::time_t t = std::time(nullptr);
    std::tm lt{};
#if defined(_WIN32)
    localtime_s(&lt, &t);
#else
    localtime_r(&t, &lt);
#endif
    y  = lt.tm_year + 1900;
    m  = lt.tm_mon + 1;
    d  = lt.tm_mday;
    hr = lt.tm_hour;
    mn = lt.tm_min;
}

int DayOfWeek(int y, int m, int d) {
    std::tm tm{};
    tm.tm_year = y - 1900; tm.tm_mon = m - 1; tm.tm_mday = d;
    tm.tm_hour = 12;  // mid-day to avoid DST edge cases
    std::time_t t = std::mktime(&tm);
    std::tm lt{};
#if defined(_WIN32)
    localtime_s(&lt, &t);
#else
    localtime_r(&t, &lt);
#endif
    int w = lt.tm_wday;       // 0=Sun..6=Sat
    return (w + 6) % 7;        // 0=Mon..6=Sun
}

static long long _jdn(int y, int m, int d) {
    long long a = (14 - m) / 12;
    long long yy = y + 4800 - a;
    long long mm = m + 12 * a - 3;
    return d + (153 * mm + 2) / 5 + 365 * yy + yy / 4 - yy / 100 + yy / 400 - 32045;
}

int DaysBetween(int ay, int am, int ad, int by, int bm, int bd) {
    return (int)(_jdn(by, bm, bd) - _jdn(ay, am, ad));
}

std::string AcademicYear(int y, int m) {
    // Bulgarian academic year runs October->September.
    int startY = (m >= 10) ? y : (y - 1);
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%d/%d", startY, startY + 1);
    return buf;
}

long long NowSeconds() {
    return (long long)std::time(nullptr);
}

std::string FormatDate(int y, int m, int d) {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d", y, m, d);
    return std::string(buf);
}

std::string RelativeTime(long long t) {
    long long now = NowSeconds();
    long long diff = now - t;
    if (diff < 0) diff = 0;
    bool en = (I18n::Current() == Lang::EN);
    if (diff < 60)        return en ? "just now" : "току-що";
    char b[32];
    if (diff < 3600)      { std::snprintf(b,sizeof(b), en ? "%lldm ago" : "преди %lld мин",  diff/60);   return b; }
    if (diff < 86400)     { std::snprintf(b,sizeof(b), en ? "%lldh ago" : "преди %lld ч",   diff/3600); return b; }
    if (diff < 86400*7)   { std::snprintf(b,sizeof(b), en ? "%lldd ago" : "преди %lld дни", diff/86400);return b; }
    if (diff < 86400*30)  { std::snprintf(b,sizeof(b), en ? "%lldw ago" : "преди %lld седм", diff/(86400*7)); return b; }
    std::snprintf(b,sizeof(b), en ? "%lldmo ago" : "преди %lld мес", diff/(86400*30)); return b;
}

} // namespace Models
