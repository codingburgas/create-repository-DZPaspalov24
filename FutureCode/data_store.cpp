// MSVC's CRT flags rename/etc as "unsafe" - silence that.
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "data_store.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>

namespace {
    std::vector<std::string> split(const std::string& s, char delim) {
        std::vector<std::string> out;
        std::string cur;
        for (char c : s) {
            if (c == delim) { out.push_back(cur); cur.clear(); }
            else cur.push_back(c);
        }
        out.push_back(cur);
        return out;
    }

    // sanitize: replace '|', '\n', '\r' with spaces so they don't break the
    // pipe-delimited format.
    std::string sanitize(const std::string& s) {
        std::string out = s;
        for (char& c : out) {
            if (c == '|' || c == '\n' || c == '\r') c = ' ';
        }
        return out;
    }

    std::string joinIds(const std::vector<int>& ids) {
        std::string out;
        for (size_t i = 0; i < ids.size(); ++i) {
            if (i > 0) out += ",";
            out += std::to_string(ids[i]);
        }
        return out;
    }

    std::vector<int> splitIds(const std::string& s) {
        std::vector<int> out;
        if (s.empty()) return out;
        for (auto& p : split(s, ',')) {
            if (!p.empty()) {
                try { out.push_back(std::stoi(p)); } catch (...) {}
            }
        }
        return out;
    }

    int toInt(const std::string& s, int defv = 0) {
        if (s.empty()) return defv;
        try { return std::stoi(s); } catch (...) { return defv; }
    }

    long long toLL(const std::string& s, long long defv = 0) {
        if (s.empty()) return defv;
        try { return std::stoll(s); } catch (...) { return defv; }
    }

    // Convenient accessor: returns the i-th field of a parsed line, or
    // an empty string if i is out of bounds (allows growing the schema
    // without breaking older lines).
    std::string getField(const std::vector<std::string>& parts, size_t i) {
        return i < parts.size() ? parts[i] : std::string();
    }
}

void DataStore::Load(const std::string& path) {
    filePath = path;
    users.clear(); subjects.clear(); grades.clear();
    notifications.clear(); activity.clear();
    schedule.clear(); exams.clear(); docRequests.clear();

    std::ifstream in(path);
    if (!in.is_open()) {
        SeedDefaults();
        Save();
        return;
    }

    // Schema check. v3 added University fields, schedule, exams, documents.
    // Older files get backed up and the seed regenerated.
    std::string firstLine;
    std::getline(in, firstLine);
    bool isCurrentSchema = firstLine.find("v3") != std::string::npos;
    if (!isCurrentSchema) {
        in.close();
        try {
            std::filesystem::rename(path, path + ".bak");
        } catch (...) { /* best-effort */ }
        SeedDefaults();
        Save();
        return;
    }

    std::string line, section;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        if (line[0] == '[') { section = line; continue; }
        if (line[0] == '#') continue;

        auto p = split(line, '|');
        if (section == "[USERS]" && p.size() >= 8) {
            User u;
            u.id          = toInt(p[0]);
            u.role        = (p[1] == "teacher") ? Role::TEACHER : Role::STUDENT;
            u.username    = p[2];
            u.passwordEnc = p[3];
            u.fullName    = p[4];
            u.className   = p[5];
            u.favoriteSubjects = splitIds(p[6]);
            u.pinnedStudents   = splitIds(p[7]);
            u.darkMode    = (getField(p, 8) == "1");
            u.facultyNumber  = getField(p, 9);
            u.faculty        = getField(p, 10);
            u.currentSemester = toInt(getField(p, 11), 1);
            u.year            = toInt(getField(p, 12), 1);
            users.push_back(u);
        } else if (section == "[SUBJECTS]" && p.size() >= 3) {
            Subject s;
            s.id         = toInt(p[0]);
            s.name       = p[1];
            s.colorIndex = toInt(p[2]);
            s.code           = getField(p, 3);
            s.credits        = toInt(getField(p, 4));
            s.hoursPerWeek   = toInt(getField(p, 5));
            s.semester       = toInt(getField(p, 6), 1);
            s.elective       = (getField(p, 7) == "1");
            s.lecturer       = getField(p, 8);
            subjects.push_back(s);
        } else if (section == "[GRADES]" && p.size() >= 8) {
            Grade g;
            g.id        = toInt(p[0]);
            g.studentId = toInt(p[1]);
            g.subjectId = toInt(p[2]);
            g.value     = toInt(p[3]);
            g.year      = toInt(p[4]);
            g.month     = toInt(p[5]);
            g.day       = toInt(p[6]);
            g.comment   = p[7];
            g.createdAt = toLL(getField(p, 8));
            grades.push_back(g);
        } else if (section == "[NOTIFICATIONS]" && p.size() >= 6) {
            Notification n;
            n.id        = toInt(p[0]);
            n.userId    = toInt(p[1]);
            int k       = toInt(p[2]);
            n.kind      = (NotificationKind)k;
            n.text      = p[3];
            n.createdAt = toLL(p[4]);
            n.read      = (p[5] == "1");
            notifications.push_back(n);
        } else if (section == "[ACTIVITY]" && p.size() >= 2) {
            Activity a;
            a.when = toLL(p[0]);
            a.text = p[1];
            activity.push_back(a);
        } else if (section == "[SCHEDULE]" && p.size() >= 9) {
            ScheduleEntry e;
            e.id           = toInt(p[0]);
            e.subjectId    = toInt(p[1]);
            e.dayOfWeek    = toInt(p[2]);
            e.startMinutes = toInt(p[3]);
            e.endMinutes   = toInt(p[4]);
            e.room         = p[5];
            e.lecturer     = p[6];
            e.kind         = (ClassKind)toInt(p[7]);
            e.semester     = toInt(p[8], 1);
            e.program      = getField(p, 9);
            schedule.push_back(e);
        } else if (section == "[EXAMS]" && p.size() >= 9) {
            ExamSession e;
            e.id              = toInt(p[0]);
            e.subjectId       = toInt(p[1]);
            e.year            = toInt(p[2]);
            e.month           = toInt(p[3]);
            e.day             = toInt(p[4]);
            e.startMinutes    = toInt(p[5]);
            e.durationMinutes = toInt(p[6]);
            e.room            = p[7];
            e.lecturer        = p[8];
            e.type            = (ExamSessionType)toInt(getField(p, 9));
            e.semester        = toInt(getField(p, 10), 1);
            e.program         = getField(p, 11);
            exams.push_back(e);
        } else if (section == "[DOCS]" && p.size() >= 7) {
            DocumentRequest d;
            d.id          = toInt(p[0]);
            d.userId      = toInt(p[1]);
            d.kind        = (DocKind)toInt(p[2]);
            d.purpose     = p[3];
            d.submittedAt = toLL(p[4]);
            d.updatedAt   = toLL(p[5]);
            d.status      = (DocStatus)toInt(p[6]);
            docRequests.push_back(d);
        }
    }
    RecomputeNextIds();
}

void DataStore::Save() {
    if (filePath.empty()) return;
    try {
        std::filesystem::path p(filePath);
        if (p.has_parent_path()) {
            std::error_code ec;
            std::filesystem::create_directories(p.parent_path(), ec);
            (void)ec;
        }
    } catch (...) { /* best-effort */ }
    std::ofstream out(filePath, std::ios::trunc);
    if (!out.is_open()) return;

    out << "# Student SIS data file v3 (university edition). Edit by hand at your own risk.\n";

    out << "[USERS]\n";
    for (const auto& u : users) {
        out << u.id << '|'
            << (u.role == Role::TEACHER ? "teacher" : "student") << '|'
            << sanitize(u.username) << '|'
            << u.passwordEnc << '|'
            << sanitize(u.fullName) << '|'
            << sanitize(u.className) << '|'
            << joinIds(u.favoriteSubjects) << '|'
            << joinIds(u.pinnedStudents) << '|'
            << (u.darkMode ? "1" : "0") << '|'
            << sanitize(u.facultyNumber) << '|'
            << sanitize(u.faculty) << '|'
            << u.currentSemester << '|'
            << u.year << '\n';
    }

    out << "[SUBJECTS]\n";
    for (const auto& s : subjects) {
        out << s.id << '|'
            << sanitize(s.name) << '|'
            << s.colorIndex << '|'
            << sanitize(s.code) << '|'
            << s.credits << '|'
            << s.hoursPerWeek << '|'
            << s.semester << '|'
            << (s.elective ? "1" : "0") << '|'
            << sanitize(s.lecturer) << '\n';
    }

    out << "[GRADES]\n";
    for (const auto& g : grades) {
        out << g.id << '|'
            << g.studentId << '|'
            << g.subjectId << '|'
            << g.value << '|'
            << g.year << '|'
            << g.month << '|'
            << g.day << '|'
            << sanitize(g.comment) << '|'
            << g.createdAt << '\n';
    }

    out << "[NOTIFICATIONS]\n";
    int notifKept = 0;
    for (auto it = notifications.rbegin(); it != notifications.rend(); ++it) {
        if (notifKept++ >= 200) break;
        out << it->id << '|'
            << it->userId << '|'
            << (int)it->kind << '|'
            << sanitize(it->text) << '|'
            << it->createdAt << '|'
            << (it->read ? "1" : "0") << '\n';
    }

    out << "[ACTIVITY]\n";
    int actKept = 0;
    for (auto it = activity.rbegin(); it != activity.rend(); ++it) {
        if (actKept++ >= 200) break;
        out << it->when << '|' << sanitize(it->text) << '\n';
    }

    out << "[SCHEDULE]\n";
    for (const auto& e : schedule) {
        out << e.id << '|'
            << e.subjectId << '|'
            << e.dayOfWeek << '|'
            << e.startMinutes << '|'
            << e.endMinutes << '|'
            << sanitize(e.room) << '|'
            << sanitize(e.lecturer) << '|'
            << (int)e.kind << '|'
            << e.semester << '|'
            << sanitize(e.program) << '\n';
    }

    out << "[EXAMS]\n";
    for (const auto& e : exams) {
        out << e.id << '|'
            << e.subjectId << '|'
            << e.year << '|'
            << e.month << '|'
            << e.day << '|'
            << e.startMinutes << '|'
            << e.durationMinutes << '|'
            << sanitize(e.room) << '|'
            << sanitize(e.lecturer) << '|'
            << (int)e.type << '|'
            << e.semester << '|'
            << sanitize(e.program) << '\n';
    }

    out << "[DOCS]\n";
    for (const auto& d : docRequests) {
        out << d.id << '|'
            << d.userId << '|'
            << (int)d.kind << '|'
            << sanitize(d.purpose) << '|'
            << d.submittedAt << '|'
            << d.updatedAt << '|'
            << (int)d.status << '\n';
    }
}

// ---------- Users ----------

User* DataStore::FindUserById(int id) {
    for (auto& u : users) if (u.id == id) return &u;
    return nullptr;
}
User* DataStore::FindUserByUsername(const std::string& uname) {
    for (auto& u : users) if (u.username == uname) return &u;
    return nullptr;
}
User* DataStore::FindUserByFacultyNumber(const std::string& fn) {
    if (fn.empty()) return nullptr;
    for (auto& u : users) if (!u.facultyNumber.empty() && u.facultyNumber == fn) return &u;
    return nullptr;
}
User* DataStore::FindUserByLogin(const std::string& login) {
    if (login.empty()) return nullptr;
    if (User* u = FindUserByFacultyNumber(login)) return u;
    return FindUserByUsername(login);
}

int DataStore::CreateUser(const User& u) {
    User cp = u;
    cp.id = nextUserId++;
    users.push_back(cp);
    Save();
    PushActivity(std::string("Регистриран нов потребител: ") + cp.fullName);
    return cp.id;
}
bool DataStore::UpdateUser(const User& u) {
    for (auto& x : users) if (x.id == u.id) { x = u; Save(); return true; }
    return false;
}
bool DataStore::DeleteUser(int id) {
    auto it = std::remove_if(users.begin(), users.end(),
        [&](const User& u){ return u.id == id; });
    if (it == users.end()) return false;
    users.erase(it, users.end());
    grades.erase(std::remove_if(grades.begin(), grades.end(),
        [&](const Grade& g){ return g.studentId == id; }), grades.end());
    notifications.erase(std::remove_if(notifications.begin(), notifications.end(),
        [&](const Notification& n){ return n.userId == id; }), notifications.end());
    docRequests.erase(std::remove_if(docRequests.begin(), docRequests.end(),
        [&](const DocumentRequest& d){ return d.userId == id; }), docRequests.end());
    Save();
    return true;
}

// ---------- Subjects ----------

Subject* DataStore::FindSubject(int id) {
    for (auto& s : subjects) if (s.id == id) return &s;
    return nullptr;
}
int DataStore::CreateSubject(const std::string& name, int colorIndex) {
    Subject s;
    s.id = nextSubjectId++;
    s.name = name;
    s.colorIndex = colorIndex;
    subjects.push_back(s);
    Save();
    PushActivity(std::string("Добавена дисциплина „") + name + "“");
    return s.id;
}
bool DataStore::UpdateSubject(const Subject& s) {
    for (auto& x : subjects) if (x.id == s.id) { x = s; Save(); return true; }
    return false;
}
bool DataStore::DeleteSubject(int id) {
    auto it = std::remove_if(subjects.begin(), subjects.end(),
        [&](const Subject& s){ return s.id == id; });
    if (it == subjects.end()) return false;
    subjects.erase(it, subjects.end());
    grades.erase(std::remove_if(grades.begin(), grades.end(),
        [&](const Grade& g){ return g.subjectId == id; }), grades.end());
    schedule.erase(std::remove_if(schedule.begin(), schedule.end(),
        [&](const ScheduleEntry& e){ return e.subjectId == id; }), schedule.end());
    exams.erase(std::remove_if(exams.begin(), exams.end(),
        [&](const ExamSession& e){ return e.subjectId == id; }), exams.end());
    Save();
    PushActivity("Премахната дисциплина");
    return true;
}

// ---------- Grades ----------

int DataStore::AddGrade(int studentId, int subjectId, int value, const std::string& comment) {
    Grade g;
    g.id = nextGradeId++;
    g.studentId = studentId;
    g.subjectId = subjectId;
    g.value     = value;
    Models::TodayYMD(g.year, g.month, g.day);
    g.comment   = comment;
    g.createdAt = Models::NowSeconds();
    grades.push_back(g);

    User* u    = FindUserById(studentId);
    Subject* s = FindSubject(subjectId);
    if (u && s) {
        std::string text = "Нова оценка " + std::to_string(value) + " по „" + s->name + "“";
        PushNotification(studentId, NotificationKind::GRADE_ADDED, text);

        // Smart insight: trend on this subject
        float td = Models::TrendDelta(studentId, subjectId, grades);
        if (td > 0.5f) {
            PushNotification(studentId, NotificationKind::INSIGHT,
                std::string("Подобрение по „") + s->name + "“: +" +
                std::to_string((int)(td * 10) / 10) + " спрямо последния резултат");
        } else if (td < -0.5f) {
            PushNotification(studentId, NotificationKind::INSIGHT,
                std::string("Внимание: спад по „") + s->name + "“");
        }

        PushActivity(u->fullName + " получи " + std::to_string(value) + " по " + s->name);
    }
    Save();
    return g.id;
}
bool DataStore::UpdateGrade(const Grade& g) {
    for (auto& x : grades) if (x.id == g.id) { x = g; Save(); return true; }
    return false;
}
bool DataStore::DeleteGrade(int id) {
    auto it = std::remove_if(grades.begin(), grades.end(),
        [&](const Grade& g){ return g.id == id; });
    if (it == grades.end()) return false;
    grades.erase(it, grades.end());
    Save();
    return true;
}

// ---------- Notifications ----------

int DataStore::PushNotification(int userId, NotificationKind k, const std::string& text) {
    Notification n;
    n.id        = nextNotifId++;
    n.userId    = userId;
    n.kind      = k;
    n.text      = text;
    n.createdAt = Models::NowSeconds();
    n.read      = false;
    notifications.push_back(n);
    return n.id;
}
void DataStore::MarkAllRead(int userId) {
    for (auto& n : notifications) if (n.userId == userId) n.read = true;
    Save();
}

// ---------- Activity ----------

void DataStore::PushActivity(const std::string& text) {
    Activity a; a.when = Models::NowSeconds(); a.text = text;
    activity.push_back(a);
}

// ---------- Schedule / Exams / Docs ----------

int DataStore::CreateScheduleEntry(const ScheduleEntry& e) {
    ScheduleEntry cp = e;
    cp.id = nextScheduleId++;
    schedule.push_back(cp);
    Save();
    return cp.id;
}
bool DataStore::DeleteScheduleEntry(int id) {
    auto it = std::remove_if(schedule.begin(), schedule.end(),
        [&](const ScheduleEntry& e){ return e.id == id; });
    if (it == schedule.end()) return false;
    schedule.erase(it, schedule.end());
    Save();
    return true;
}

int DataStore::CreateExam(const ExamSession& e) {
    ExamSession cp = e;
    cp.id = nextExamId++;
    exams.push_back(cp);
    Save();
    return cp.id;
}
bool DataStore::DeleteExam(int id) {
    auto it = std::remove_if(exams.begin(), exams.end(),
        [&](const ExamSession& e){ return e.id == id; });
    if (it == exams.end()) return false;
    exams.erase(it, exams.end());
    Save();
    return true;
}

int DataStore::SubmitDocumentRequest(int userId, DocKind k, const std::string& purpose) {
    DocumentRequest d;
    d.id          = nextDocReqId++;
    d.userId      = userId;
    d.kind        = k;
    d.purpose     = purpose;
    d.submittedAt = Models::NowSeconds();
    d.updatedAt   = d.submittedAt;
    d.status      = DocStatus::PENDING;
    docRequests.push_back(d);
    Save();
    return d.id;
}
bool DataStore::UpdateDocumentRequest(const DocumentRequest& d) {
    for (auto& x : docRequests) if (x.id == d.id) { x = d; Save(); return true; }
    return false;
}

void DataStore::RecomputeNextIds() {
    nextUserId = 1; nextSubjectId = 1; nextGradeId = 1; nextNotifId = 1;
    nextScheduleId = 1; nextExamId = 1; nextDocReqId = 1;
    for (const auto& u : users)         if (u.id  >= nextUserId)     nextUserId     = u.id + 1;
    for (const auto& s : subjects)      if (s.id  >= nextSubjectId)  nextSubjectId  = s.id + 1;
    for (const auto& g : grades)        if (g.id  >= nextGradeId)    nextGradeId    = g.id + 1;
    for (const auto& n : notifications) if (n.id  >= nextNotifId)    nextNotifId    = n.id + 1;
    for (const auto& e : schedule)      if (e.id  >= nextScheduleId) nextScheduleId = e.id + 1;
    for (const auto& e : exams)         if (e.id  >= nextExamId)     nextExamId     = e.id + 1;
    for (const auto& d : docRequests)   if (d.id  >= nextDocReqId)   nextDocReqId   = d.id + 1;
}

// ===========================================================================
//  Sample / seed data
//
//  This is the data that populates the system on first run. It paints a
//  realistic picture of a Bulgarian university CS bachelor's first year:
//  one teacher (Yanko Yanakiev), eight students split across three programs,
//  ten properly-coded courses with ECTS credits and weekly hours, a
//  weekly schedule, an upcoming exam session and a couple of seed
//  document requests so the dashboard has something to display.
// ===========================================================================
void DataStore::SeedDefaults() {
    // Default lecturer (teacher)
    User t;
    t.role = Role::TEACHER;
    t.username = "admin";
    t.fullName = "Янко Янакиев";
    t.passwordEnc = Models::ObfuscatePassword("admin");
    t.facultyNumber = "L0001";
    t.faculty = "Факултет по технически науки";
    t.id = 1;
    users.push_back(t);

    // ---- Subjects (Bulgarian-language CS curriculum) ----
    auto addSubj = [&](const char* code, const char* name, int color, int credits,
                       int hours, int semester, bool elective, const char* lecturer)
    {
        Subject s;
        s.id           = (int)subjects.size() + 1;
        s.name         = name;
        s.code         = code;
        s.colorIndex   = color;
        s.credits      = credits;
        s.hoursPerWeek = hours;
        s.semester     = semester;
        s.elective     = elective;
        s.lecturer     = lecturer;
        subjects.push_back(s);
    };

    // Semester 1
    addSubj("MAT101", "Висша математика I",                   0, 7, 6, 1, false, "проф. д-р Атанас Петров");
    addSubj("INF101", "Програмиране I",                       1, 7, 6, 1, false, "доц. д-р Янко Янакиев");
    addSubj("INF102", "Дискретна математика",                 2, 5, 4, 1, false, "доц. д-р Мария Колева");
    addSubj("INF103", "Въведение в компютърните системи",     3, 5, 4, 1, false, "гл. ас. Никола Петков");
    addSubj("LAN101", "Английски език за IT",                 4, 3, 2, 1, true,  "ст. преп. Емилия Дикова");

    // Semester 2
    addSubj("MAT102", "Висша математика II",                  5, 6, 5, 2, false, "проф. д-р Атанас Петров");
    addSubj("INF201", "Програмиране II - ООП",                6, 7, 6, 2, false, "доц. д-р Янко Янакиев");
    addSubj("INF202", "Алгоритми и структури от данни",       7, 7, 6, 2, false, "доц. д-р Стоян Кръстев");
    addSubj("INF203", "Бази от данни",                        0, 6, 5, 2, false, "доц. д-р Жулиета Райчева");
    addSubj("INF204", "Уеб технологии",                       1, 4, 3, 2, true,  "гл. ас. д-р Иво Драгнев");

    // ---- Students (Bulgarian names, faculty numbers F12345 style) ----
    struct Seed {
        const char* name;
        const char* user;
        const char* fn;       // faculty number
        const char* program;
        int currentSem;
        const char* faculty;
    };
    const Seed seeds[] = {
        { "Денис Паспалов",     "denis",    "F23001", "Информатика", 2, "Факултет по технически науки" },
        { "Стоян Колев",        "stoyan",   "F23002", "Информатика", 2, "Факултет по технически науки" },
        { "Иван Димитров",      "ivan",     "F23003", "Информатика", 2, "Факултет по технически науки" },
        { "Мария Георгиева",    "maria",    "F23004", "Софтуерно инженерство", 2, "Факултет по технически науки" },
        { "Петър Стоянов",      "petar",    "F23005", "Софтуерно инженерство", 2, "Факултет по технически науки" },
        { "Елена Тодорова",     "elena",    "F23006", "Софтуерно инженерство", 2, "Факултет по технически науки" },
        { "Николай Христов",    "nikolay",  "F23007", "Информационни системи", 2, "Факултет по технически науки" },
        { "Виктория Иванова",   "viktoria", "F23008", "Информационни системи", 2, "Факултет по технически науки" },
    };
    for (const auto& sd : seeds) {
        User s;
        s.role = Role::STUDENT;
        s.id = (int)users.size() + 1;
        s.fullName        = sd.name;
        s.username        = sd.user;
        s.facultyNumber   = sd.fn;
        s.className       = sd.program;
        s.faculty         = sd.faculty;
        s.currentSemester = sd.currentSem;
        s.year            = (sd.currentSem + 1) / 2;
        s.passwordEnc     = Models::ObfuscatePassword("student");
        users.push_back(s);
    }

    // ---- Sample grades ----
    int yyyy, mm, dd; Models::TodayYMD(yyyy, mm, dd);
    long long now = Models::NowSeconds();
    auto addGrade = [&](int sid, int subj, int val, int daysAgo, const char* c) {
        Grade g;
        g.id = (int)grades.size() + 1;
        g.studentId = sid; g.subjectId = subj;
        g.value = val; g.year = yyyy; g.month = mm; g.day = dd;
        g.comment = c;
        g.createdAt = now - (long long)daysAgo * 86400LL;
        grades.push_back(g);
    };
    // Subject IDs 1-10 in seed order. Student IDs start at 2 (teacher is 1).
    // Денис (id=2)
    addGrade(2, 1, 6, 60, "Отлично представяне на изпита");
    addGrade(2, 2, 6, 55, "Безупречен код");
    addGrade(2, 3, 5, 50, "Много добра подготовка");
    addGrade(2, 4, 5, 48, "");
    addGrade(2, 5, 6, 45, "Excellent presentation");
    // Стоян (id=3)
    addGrade(3, 1, 4, 60, "");
    addGrade(3, 2, 5, 55, "Хубава работа");
    addGrade(3, 3, 4, 50, "");
    addGrade(3, 6, 5, 14, "Подобрение!");
    // Иван (id=4)
    addGrade(4, 1, 5, 60, "");
    addGrade(4, 2, 6, 55, "Чисто решение");
    addGrade(4, 7, 6, 14, "Отличен проект по ООП");
    // Мария (id=5)
    addGrade(5, 1, 5, 60, "");
    addGrade(5, 3, 6, 50, "Силна логика");
    addGrade(5, 8, 5, 10, "");
    // Петър (id=6)
    addGrade(6, 2, 6, 55, "");
    addGrade(6, 6, 6, 14, "Изключително");
    addGrade(6, 9, 5, 7,  "");
    // Елена (id=7)
    addGrade(7, 4, 5, 48, "");
    addGrade(7, 7, 4, 14, "");
    addGrade(7, 10, 5, 5, "Чудесно представяне");
    // Николай (id=8)
    addGrade(8, 1, 4, 60, "");
    addGrade(8, 3, 5, 50, "Стабилно");
    addGrade(8, 8, 5, 10, "");
    // Виктория (id=9)
    addGrade(9, 2, 6, 55, "Топ резултат");
    addGrade(9, 6, 6, 14, "Отличен анализ");
    addGrade(9, 9, 6, 7,  "Перфектно");

    // ---- Schedule (current semester = 2 for all sample students) ----
    // 1 = MAT102, 6 = MAT102 actually MAT102 id is 6. Let me re-map by id:
    //   1 MAT101 | 2 INF101 | 3 INF102 | 4 INF103 | 5 LAN101 |
    //   6 MAT102 | 7 INF201 | 8 INF202 | 9 INF203 | 10 INF204
    //
    // We seed schedule for semester 2 (the "current" one).
    auto addSched = [&](int subjId, int day, int sh, int sm, int eh, int em,
                        const char* room, const char* lec, ClassKind k,
                        int sem, const char* program)
    {
        ScheduleEntry e;
        e.id           = (int)schedule.size() + 1;
        e.subjectId    = subjId;
        e.dayOfWeek    = day;
        e.startMinutes = sh * 60 + sm;
        e.endMinutes   = eh * 60 + em;
        e.room         = room;
        e.lecturer     = lec;
        e.kind         = k;
        e.semester     = sem;
        e.program      = program;
        schedule.push_back(e);
    };

    // Monday
    addSched(6,  0,  8, 30, 10,  0, "Зала 211", "проф. д-р Атанас Петров",  ClassKind::LECTURE, 2, "");
    addSched(6,  0, 10, 15, 11, 45, "Зала 211", "проф. д-р Атанас Петров",  ClassKind::LAB,     2, "");
    addSched(7,  0, 13,  0, 14, 30, "Зала 305", "доц. д-р Янко Янакиев",    ClassKind::LECTURE, 2, "");
    addSched(7,  0, 14, 45, 16, 15, "Зала 401", "доц. д-р Янко Янакиев",    ClassKind::LAB,     2, "");
    // Tuesday
    addSched(8,  1,  9,  0, 10, 30, "Зала 207", "доц. д-р Стоян Кръстев",   ClassKind::LECTURE, 2, "");
    addSched(8,  1, 10, 45, 12, 15, "Зала 401", "доц. д-р Стоян Кръстев",   ClassKind::LAB,     2, "");
    addSched(9,  1, 13, 30, 15,  0, "Зала 309", "доц. д-р Жулиета Райчева", ClassKind::LECTURE, 2, "");
    // Wednesday
    addSched(7,  2,  8, 30, 10,  0, "Зала 305", "доц. д-р Янко Янакиев",    ClassKind::SEMINAR, 2, "");
    addSched(9,  2, 10, 15, 11, 45, "Зала 402", "доц. д-р Жулиета Райчева", ClassKind::LAB,     2, "");
    addSched(10, 2, 13,  0, 14, 30, "Зала 308", "гл. ас. д-р Иво Драгнев",  ClassKind::LECTURE, 2, "");
    // Thursday
    addSched(6,  3,  9,  0, 10, 30, "Зала 211", "проф. д-р Атанас Петров",  ClassKind::SEMINAR, 2, "");
    addSched(8,  3, 10, 45, 12, 15, "Зала 207", "доц. д-р Стоян Кръстев",   ClassKind::SEMINAR, 2, "");
    addSched(10, 3, 13, 30, 15,  0, "Зала 402", "гл. ас. д-р Иво Драгнев",  ClassKind::LAB,     2, "");
    // Friday
    addSched(9,  4,  8, 30, 10,  0, "Зала 309", "доц. д-р Жулиета Райчева", ClassKind::SEMINAR, 2, "");
    addSched(7,  4, 10, 15, 11, 45, "Зала 401", "доц. д-р Янко Янакиев",    ClassKind::LAB,     2, "");

    // ---- Exam session (upcoming + a couple of past) ----
    auto addExam = [&](int subjId, int y, int m, int d, int h, int min,
                       const char* room, const char* lec, ExamSessionType type, int sem) {
        ExamSession e;
        e.id              = (int)exams.size() + 1;
        e.subjectId       = subjId;
        e.year = y; e.month = m; e.day = d;
        e.startMinutes    = h * 60 + min;
        e.durationMinutes = 180;
        e.room            = room;
        e.lecturer        = lec;
        e.type            = type;
        e.semester        = sem;
        exams.push_back(e);
    };
    // Compute dates relative to today: spread upcoming exams over the next 30 days
    auto addDays = [](int y, int m, int d, int days, int& outY, int& outM, int& outD){
        std::tm tm{};
        tm.tm_year = y - 1900; tm.tm_mon = m - 1; tm.tm_mday = d + days;
        tm.tm_hour = 12; // avoid DST edge cases
        std::time_t t = std::mktime(&tm);
        std::tm lt{};
#if defined(_WIN32)
        localtime_s(&lt, &t);
#else
        localtime_r(&t, &lt);
#endif
        outY = lt.tm_year + 1900; outM = lt.tm_mon + 1; outD = lt.tm_mday;
    };
    int ey, em, ed;
    addDays(yyyy, mm, dd,  7, ey, em, ed); addExam(6,  ey, em, ed,  9, 0, "Зала 211", "проф. д-р Атанас Петров", ExamSessionType::REGULAR, 2);
    addDays(yyyy, mm, dd, 12, ey, em, ed); addExam(7,  ey, em, ed, 10, 0, "Зала 305", "доц. д-р Янко Янакиев",   ExamSessionType::REGULAR, 2);
    addDays(yyyy, mm, dd, 18, ey, em, ed); addExam(8,  ey, em, ed,  9, 30,"Зала 207", "доц. д-р Стоян Кръстев",  ExamSessionType::REGULAR, 2);
    addDays(yyyy, mm, dd, 23, ey, em, ed); addExam(9,  ey, em, ed, 11, 0, "Зала 309", "доц. д-р Жулиета Райчева",ExamSessionType::REGULAR, 2);
    addDays(yyyy, mm, dd, 28, ey, em, ed); addExam(10, ey, em, ed,  9, 0, "Зала 308", "гл. ас. д-р Иво Драгнев", ExamSessionType::REGULAR, 2);
    // Past exams from sem 1
    addDays(yyyy, mm, dd, -42, ey, em, ed); addExam(1,  ey, em, ed,  9, 0, "Зала 211", "проф. д-р Атанас Петров", ExamSessionType::REGULAR, 1);
    addDays(yyyy, mm, dd, -38, ey, em, ed); addExam(2,  ey, em, ed, 10, 0, "Зала 305", "доц. д-р Янко Янакиев",   ExamSessionType::REGULAR, 1);
    addDays(yyyy, mm, dd, -32, ey, em, ed); addExam(3,  ey, em, ed,  9, 0, "Зала 207", "доц. д-р Мария Колева",   ExamSessionType::REGULAR, 1);

    // ---- Sample document requests for one student ----
    DocumentRequest dr1;
    dr1.id = 1; dr1.userId = 2; dr1.kind = DocKind::ENROLLMENT;
    dr1.purpose = "За НОИ"; dr1.submittedAt = now - 86400LL * 4;
    dr1.updatedAt = now - 86400LL * 2; dr1.status = DocStatus::READY;
    docRequests.push_back(dr1);

    DocumentRequest dr2;
    dr2.id = 2; dr2.userId = 2; dr2.kind = DocKind::TRANSCRIPT;
    dr2.purpose = "За работодател"; dr2.submittedAt = now - 86400LL * 1;
    dr2.updatedAt = dr2.submittedAt; dr2.status = DocStatus::PENDING;
    docRequests.push_back(dr2);

    PushActivity("Системата беше инициализирана с примерни данни");
    RecomputeNextIds();
}
