#pragma once
#include "models.h"
#include <vector>
#include <string>
#include <functional>

// Owns all in-memory data and persists it to a single text file.
// Custom format chosen over JSON to avoid an external dependency.

// Type of a class meeting - lecture, lab work or seminar.
enum class ClassKind { LECTURE, LAB, SEMINAR };

// One weekly recurring class block.
struct ScheduleEntry {
    int  id = 0;
    int  subjectId = 0;
    int  dayOfWeek = 0;       // 0 = Monday .. 5 = Saturday (we don't use Sunday)
    int  startMinutes = 0;    // minutes from midnight (e.g. 08:30 = 510)
    int  endMinutes = 0;
    std::string room;
    std::string lecturer;
    ClassKind   kind = ClassKind::LECTURE;
    int  semester = 1;        // which semester this entry is part of
    std::string program;      // restrict by program (empty = all programs)
};

// Exam-session entry for a course.
enum class ExamSessionType { REGULAR, MAKEUP };

struct ExamSession {
    int id = 0;
    int subjectId = 0;
    int year = 0, month = 0, day = 0;   // exam date
    int startMinutes = 0;               // start time
    int durationMinutes = 120;
    std::string room;
    std::string lecturer;
    ExamSessionType type = ExamSessionType::REGULAR;
    int  semester = 1;
    std::string program;                // empty = all programs
};

// Status of a student-submitted document request.
enum class DocStatus { PENDING, PROCESSING, READY, COLLECTED };

enum class DocKind { TRANSCRIPT, ENROLLMENT, SEMESTER_CERT, DIPLOMA_DUP };

struct DocumentRequest {
    int id = 0;
    int userId = 0;            // student who filed the request
    DocKind kind = DocKind::ENROLLMENT;
    std::string purpose;       // free-text: what the document is for
    long long submittedAt = 0;
    long long updatedAt = 0;
    DocStatus status = DocStatus::PENDING;
};

class DataStore {
public:
    void Load(const std::string& path);     // creates seed data if file missing
    void Save();                             // saves to last-loaded path

    // ---- users
    User*       FindUserById(int id);
    User*       FindUserByUsername(const std::string& uname);
    User*       FindUserByFacultyNumber(const std::string& fn);
    User*       FindUserByLogin(const std::string& login); // tries fn first, then username
    int         CreateUser(const User& u);   // returns new id
    bool        UpdateUser(const User& u);   // by id
    bool        DeleteUser(int id);          // also cleans up grades
    std::vector<User>& Users() { return users; }

    // ---- subjects
    Subject*    FindSubject(int id);
    int         CreateSubject(const std::string& name, int colorIndex);
    bool        UpdateSubject(const Subject& s);
    bool        DeleteSubject(int id);       // also deletes related grades
    std::vector<Subject>& Subjects() { return subjects; }

    // ---- grades
    int         AddGrade(int studentId, int subjectId, int value, const std::string& comment);
    bool        UpdateGrade(const Grade& g);
    bool        DeleteGrade(int id);
    std::vector<Grade>& Grades() { return grades; }

    // ---- notifications
    int         PushNotification(int userId, NotificationKind k, const std::string& text);
    void        MarkAllRead(int userId);
    std::vector<Notification>& Notifications() { return notifications; }

    // ---- activity feed
    struct Activity {
        long long when;
        std::string text;
    };
    std::vector<Activity>& ActivityFeed() { return activity; }

    // ---- schedule
    int         CreateScheduleEntry(const ScheduleEntry& e);
    bool        DeleteScheduleEntry(int id);
    std::vector<ScheduleEntry>& Schedule() { return schedule; }

    // ---- exams
    int         CreateExam(const ExamSession& e);
    bool        DeleteExam(int id);
    std::vector<ExamSession>& Exams() { return exams; }

    // ---- document requests
    int         SubmitDocumentRequest(int userId, DocKind k, const std::string& purpose);
    bool        UpdateDocumentRequest(const DocumentRequest& d);
    std::vector<DocumentRequest>& DocumentRequests() { return docRequests; }

private:
    std::string filePath;
    std::vector<User> users;
    std::vector<Subject> subjects;
    std::vector<Grade> grades;
    std::vector<Notification> notifications;
    std::vector<DataStore::Activity> activity;
    std::vector<ScheduleEntry> schedule;
    std::vector<ExamSession> exams;
    std::vector<DocumentRequest> docRequests;

    int nextUserId = 1, nextSubjectId = 1, nextGradeId = 1, nextNotifId = 1;
    int nextScheduleId = 1, nextExamId = 1, nextDocReqId = 1;

    void SeedDefaults();
    void RecomputeNextIds();
    void PushActivity(const std::string& text);
};
