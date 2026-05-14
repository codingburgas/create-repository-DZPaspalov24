#pragma once
#include <string>
#include <vector>
#include <ctime>

// Tiny, plain-data models. All persistence and business logic uses these.

enum class Role { TEACHER, STUDENT };

struct User {
    int id = 0;
    Role role = Role::STUDENT;
    std::string username;
    std::string passwordEnc;     // lightly obfuscated, NOT secure (school project)
    std::string fullName;
    std::string className;       // university program name, e.g. "Информатика"
    std::vector<int> favoriteSubjects;  // student: pinned subjects
    std::vector<int> pinnedStudents;    // teacher: pinned students
    bool darkMode = false;

    // University-specific fields
    std::string facultyNumber;    // e.g. "F12345" - primary student identifier
    std::string faculty;          // e.g. "Технически колеж" / "Faculty of Engineering"
    int currentSemester = 1;      // 1..8 for a 4-year bachelor
    int year = 1;                 // 1..4 academic year (derived from semester but stored)
};

struct Subject {
    int id = 0;
    std::string name;
    int colorIndex = 0;           // index into a fixed palette

    // University-specific fields
    std::string code;             // e.g. "INF101"
    int credits = 0;              // ECTS
    int hoursPerWeek = 0;         // contact hours
    int semester = 1;             // 1..8 - which semester the course belongs to
    bool elective = false;        // false = compulsory, true = elective
    std::string lecturer;         // e.g. "проф. д-р Иван Иванов"
};

struct Grade {
    int id = 0;
    int studentId = 0;
    int subjectId = 0;
    int value = 6;                // 2..6
    int year = 0, month = 0, day = 0;   // simple date
    std::string comment;
    long long createdAt = 0;      // seconds since epoch, used for sort/feed
};

enum class NotificationKind { GRADE_ADDED, GRADE_UPDATED, INSIGHT, INFO };

struct Notification {
    int id = 0;
    int userId = 0;               // who sees this (0 = global)
    NotificationKind kind = NotificationKind::INFO;
    std::string text;
    long long createdAt = 0;
    bool read = false;
};

// Helpers operating on the above
namespace Models {
    // Average of all grades for a student in a subject. Returns -1 if none.
    float StudentSubjectAverage(int studentId, int subjectId, const std::vector<Grade>& grades);
    // Overall average across all subjects for a student. Returns -1 if none.
    float StudentOverallAverage(int studentId, const std::vector<Grade>& grades);
    // Class-wide average per subject (teacher view).
    float SubjectClassAverage(int subjectId, const std::vector<Grade>& grades);
    // Distribution buckets: index 0 = grade 2, ..., index 4 = grade 6.
    void GradeDistribution(const std::vector<Grade>& grades, int outBuckets[5]);
    // Trend: difference between latest grade and previous one for a student/subject.
    // Returns 0 if not enough data. Used for "smart insights" feed.
    float TrendDelta(int studentId, int subjectId, const std::vector<Grade>& grades);

    // Returns up to `maxPoints` recent grade values for a student in
    // chronological order. Used to draw GPA-tile sparklines.
    std::vector<float> RecentGradeSeries(int studentId, const std::vector<Grade>& grades, int maxPoints = 12);

    // Light, reversible obfuscation. Stops casual file inspection from reading
    // passwords - NOT real security.
    std::string ObfuscatePassword(const std::string& plain);
    bool        VerifyPassword(const std::string& plain, const std::string& enc);

    // Date helpers (no time-of-day, just YYYY-MM-DD).
    void TodayYMD(int& y, int& m, int& d);
    void NowYMDHM(int& y, int& m, int& d, int& hr, int& mn);
    long long NowSeconds();
    std::string FormatDate(int y, int m, int d);
    // "2 days ago" style relative time
    std::string RelativeTime(long long t);

    // Day of the week for date (0=Mon..6=Sun).
    int DayOfWeek(int y, int m, int d);
    // Returns days(b - a) for two ISO dates.
    int DaysBetween(int ay, int am, int ad, int by, int bm, int bd);
    // Returns the academic year string for a given calendar date.
    // E.g. (2024, 11, 5) -> "2024/2025"; (2025, 3, 5) -> "2024/2025";
    //      (2025, 9, 5) -> "2025/2026". September is the cut-off.
    std::string AcademicYear(int y, int m);
}
