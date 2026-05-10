#pragma once
#include <cstring>

// Tiny, allocation-free translation system. The default language is Bulgarian
// (BG) so the app feels like a Bulgarian university SIS out of the box. The
// user can flip to English with the flag toggle in the header. Calls are
// cheap - one strcmp chain per lookup, called sparingly per frame.

enum class Lang { BG, EN };

namespace I18n {

inline Lang& g_lang() { static Lang l = Lang::BG; return l; }

inline Lang Current()        { return g_lang(); }
inline void SetLang(Lang l)  { g_lang() = l; }
inline void Toggle()         { g_lang() = (g_lang() == Lang::BG ? Lang::EN : Lang::BG); }

// Macro-keyed lookup. If a key is missing in EN it falls back to BG;
// if missing entirely, returns the key itself (so unfound keys are obvious).
inline const char* T(const char* key) {
    bool en = (g_lang() == Lang::EN);

    #define TR(k, bg, en_) if (std::strcmp(key, k) == 0) return en ? en_ : bg;

    // ------------ App / brand ------------
    TR("app.name",                  "Student",                                  "Student")
    TR("app.subtitle",              "Студентска информационна система",         "Student Information System")
    TR("app.university",            "FutureCode",                               "FutureCode")
    TR("app.university.full",       "Технологичен университет FutureCode",      "FutureCode Technology University")

    // ------------ Login ------------
    TR("login.title",               "Вход",                                     "Sign In")
    TR("login.faculty_number",      "Факултетен номер",                         "Faculty Number")
    TR("login.password",            "Парола",                                   "Password")
    TR("login.password.alt",        "Парола / ЕГН / ЛНЧ / ЛИН",                 "Password / ID number")
    TR("login.submit",              "Вход",                                     "Sign In")
    TR("login.forgot_fn",           "Забравен факултетен номер?",               "Forgot faculty number?")
    TR("login.forgot_pass",         "Забравена парола?",                        "Forgot password?")
    TR("login.create_account",      "Създай профил",                            "Create Account")
    TR("login.what_new",            "Какво е новото?",                          "What's new?")
    TR("login.welcome",             "Добре дошли в Student",                    "Welcome to Student")
    TR("login.feature_design",      "Модерен дизайн, вдъхновен от последните тенденции",
                                    "Modern design inspired by the latest trends")
    TR("login.feature_grades",      "Всички оценки на едно място",               "All your grades in one place")
    TR("login.feature_curriculum",  "Достъп до учебния план",                   "Access to your full curriculum")
    TR("login.feature_schedule",    "Седмичен график на занятията",              "Weekly class schedule")
    TR("login.feature_exams",       "Информация за изпитната сесия",             "Exam-session information")
    TR("login.feature_docs",        "Заявки за документи онлайн",                "Online document requests")
    TR("login.go_to_student",       "Към Student",                              "Go to Student")
    TR("login.error_credentials",   "Невалиден факултетен номер или парола",    "Invalid faculty number or password")
    TR("login.error_empty",         "Моля попълнете всички полета",              "Please fill in all fields")
    TR("login.role.teacher",        "Преподавател",                             "Lecturer")
    TR("login.role.student",        "Студент",                                  "Student")
    TR("login.recovery_msg",        "Моля свържете се с администрацията.",      "Please contact the administration.")
    TR("login.welcome_back",        "Добре дошли",                              "Welcome back")

    // ------------ Signup ------------
    TR("signup.title",              "Създаване на профил",                      "Create an account")
    TR("signup.full_name",          "Име и фамилия",                            "Full name")
    TR("signup.username",           "Потребителско име",                        "Username")
    TR("signup.password",           "Парола",                                   "Password")
    TR("signup.confirm",            "Потвърди паролата",                        "Confirm password")
    TR("signup.program",            "Специалност",                              "Program")
    TR("signup.faculty_number",     "Факултетен номер",                         "Faculty Number")
    TR("signup.submit",             "Регистрация",                              "Sign Up")
    TR("signup.have_account",       "Вече имате профил? Вход",                  "Already have an account?  Sign in")
    TR("signup.error_short_pwd",    "Паролата трябва да е поне 4 знака.",       "Password must be at least 4 characters.")
    TR("signup.error_mismatch",     "Паролите не съвпадат.",                    "Passwords do not match.")
    TR("signup.error_taken",        "Този потребител вече съществува.",          "This username is already taken.")
    TR("signup.error_program",      "Студентите трябва да изберат специалност.", "Students must specify a program.")
    TR("signup.error_empty",        "Попълнете името, потребителското име и паролата.",
                                    "Please fill in name, username and password.")
    TR("signup.error_fn_taken",     "Този факултетен номер е зает.",            "This faculty number is already in use.")

    // ------------ Tabs / nav ------------
    TR("tab.grades",                "Оценки",                                   "Grades")
    TR("tab.curriculum",            "Учебен план",                              "Curriculum")
    TR("tab.schedule",              "График",                                   "Schedule")
    TR("tab.exams",                 "Изпитна сесия",                            "Exam Session")
    TR("tab.documents",             "Документи",                                "Documents")
    TR("tab.profile",               "Профил",                                   "Profile")

    // ------------ Common ------------
    TR("common.logout",             "Изход",                                    "Logout")
    TR("common.cancel",             "Отказ",                                    "Cancel")
    TR("common.save",               "Запази",                                   "Save")
    TR("common.delete",             "Изтрий",                                   "Delete")
    TR("common.edit",               "Редактирай",                               "Edit")
    TR("common.add",                "Добави",                                   "Add")
    TR("common.search",             "Търсене...",                               "Search...")
    TR("common.back",               "Назад",                                    "Back")
    TR("common.close",              "Затвори",                                  "Close")
    TR("common.no_data",            "Няма данни.",                              "No data.")
    TR("common.optional",           "(незадължително)",                          "(optional)")
    TR("common.required",           "(задължително)",                            "(required)")
    TR("common.yes",                "Да",                                       "Yes")
    TR("common.no",                 "Не",                                       "No")
    TR("common.all",                "Всички",                                   "All")
    TR("common.theme.dark",         "Тъмна",                                    "Dark")
    TR("common.theme.light",        "Светла",                                   "Light")
    TR("common.welcome_modal_title","Добре дошли в Student 🎉",                 "Welcome to Student 🎉")
    TR("common.welcome_modal_intro","Какво ново има?",                          "What's new?")

    // ------------ Student dashboard ------------
    TR("student.greeting",          "Здравейте,",                               "Hello,")
    TR("student.gpa",               "Среден успех",                             "GPA")
    TR("student.credits_earned",    "Спечелени кредити",                        "Credits earned")
    TR("student.credits_total",     "от общо",                                  "of")
    TR("student.semester",          "Семестър",                                 "Semester")
    TR("student.year",              "Курс",                                     "Year")
    TR("student.program",           "Специалност",                              "Program")
    TR("student.faculty",           "Факултет",                                 "Faculty")
    TR("student.faculty_number",    "Факултетен №",                             "Faculty №")
    TR("student.status",            "Статус",                                   "Status")
    TR("student.status.active",     "Активен",                                  "Active")
    TR("student.recent_grades",     "Последни оценки",                          "Recent grades")
    TR("student.upcoming_exams",    "Предстоящи изпити",                        "Upcoming exams")
    TR("student.no_grades",         "Все още няма поставени оценки.",            "No grades have been recorded yet.")
    TR("student.this_semester",     "Текущ семестър",                           "Current semester")

    // ------------ Curriculum ------------
    TR("curriculum.title",          "Учебен план",                              "Curriculum")
    TR("curriculum.code",           "Код",                                      "Code")
    TR("curriculum.course",         "Дисциплина",                               "Course")
    TR("curriculum.credits",        "Кредити",                                  "Credits")
    TR("curriculum.hours",          "Часове",                                   "Hours")
    TR("curriculum.semester",       "Семестър",                                 "Semester")
    TR("curriculum.compulsory",     "Задължителна",                             "Compulsory")
    TR("curriculum.elective",       "Избираема",                                "Elective")
    TR("curriculum.type",           "Вид",                                      "Type")
    TR("curriculum.grade",          "Оценка",                                   "Grade")
    TR("curriculum.lecturer",       "Преподавател",                             "Lecturer")
    TR("curriculum.passed",         "Положен",                                  "Passed")
    TR("curriculum.failed",         "Неположен",                                "Failed")
    TR("curriculum.pending",        "Предстоящ",                                "Pending")

    // ------------ Schedule ------------
    TR("schedule.title",            "Седмичен график",                          "Weekly Schedule")
    TR("schedule.day.mon",          "Понеделник",                               "Monday")
    TR("schedule.day.tue",          "Вторник",                                  "Tuesday")
    TR("schedule.day.wed",          "Сряда",                                    "Wednesday")
    TR("schedule.day.thu",          "Четвъртък",                                "Thursday")
    TR("schedule.day.fri",          "Петък",                                    "Friday")
    TR("schedule.day.sat",          "Събота",                                   "Saturday")
    TR("schedule.day.sun",          "Неделя",                                   "Sunday")
    TR("schedule.lecture",          "Лекция",                                   "Lecture")
    TR("schedule.lab",              "Упражнение",                               "Lab")
    TR("schedule.seminar",          "Семинар",                                  "Seminar")
    TR("schedule.room",             "Зала",                                     "Room")
    TR("schedule.lecturer",         "Преподавател",                             "Lecturer")
    TR("schedule.no_classes",       "Няма занятия в този ден.",                 "No classes scheduled for this day.")

    // ------------ Exams ------------
    TR("exam.title",                "Изпитна сесия",                            "Exam Session")
    TR("exam.date",                 "Дата",                                     "Date")
    TR("exam.time",                 "Час",                                      "Time")
    TR("exam.room",                 "Зала",                                     "Room")
    TR("exam.regular",              "Редовна сесия",                            "Regular session")
    TR("exam.makeup",               "Поправителна сесия",                       "Makeup session")
    TR("exam.upcoming",             "Предстоящи",                               "Upcoming")
    TR("exam.past",                 "Минали",                                   "Past")
    TR("exam.all",                  "Всички",                                   "All")
    TR("exam.lecturer",             "Преподавател",                             "Lecturer")
    TR("exam.session_type",         "Сесия",                                    "Session")
    TR("exam.no_exams",             "Няма насрочени изпити.",                   "No exams scheduled.")
    TR("exam.days_until",           "след",                                     "in")
    TR("exam.days",                 "дни",                                      "days")
    TR("exam.today",                "днес",                                     "today")
    TR("exam.tomorrow",             "утре",                                     "tomorrow")

    // ------------ Documents ------------
    TR("doc.title",                 "Документи",                                "Documents")
    TR("doc.request",               "Заявка за документ",                       "Request a document")
    TR("doc.type",                  "Вид документ",                             "Document type")
    TR("doc.transcript",            "Академична справка",                        "Academic transcript")
    TR("doc.enrollment",            "Уверение за студент",                       "Enrollment certificate")
    TR("doc.diploma",               "Дубликат на диплома",                       "Diploma duplicate")
    TR("doc.semester_cert",         "Уверение за семестър",                     "Semester certificate")
    TR("doc.purpose",               "Предназначение",                            "Purpose")
    TR("doc.purpose.placeholder",   "За какво е необходим документът?",          "What is the document for?")
    TR("doc.status",                "Статус",                                   "Status")
    TR("doc.status.pending",        "Чакащ обработка",                           "Pending review")
    TR("doc.status.processing",     "В обработка",                              "Being processed")
    TR("doc.status.ready",          "Готов за получаване",                       "Ready for collection")
    TR("doc.status.collected",      "Получен",                                  "Collected")
    TR("doc.submit",                "Подай заявка",                             "Submit Request")
    TR("doc.requested_on",          "Подадена на",                              "Submitted on")
    TR("doc.no_requests",           "Няма подадени заявки.",                    "No requests submitted.")
    TR("doc.empty_purpose",         "Моля въведете предназначение.",             "Please enter a purpose.")
    TR("doc.submitted",             "Заявката беше подадена.",                  "Your request has been submitted.")

    // ------------ Profile ------------
    TR("profile.title",             "Профил",                                   "Profile")
    TR("profile.personal",          "Лични данни",                              "Personal information")
    TR("profile.academic",          "Академични данни",                         "Academic information")
    TR("profile.preferences",       "Настройки",                                "Preferences")
    TR("profile.language",          "Език",                                     "Language")
    TR("profile.theme",             "Тема",                                     "Theme")
    TR("profile.full_name",         "Имена",                                    "Full name")
    TR("profile.username",          "Потребител",                              "Username")

    // ------------ Teacher ------------
    TR("teacher.title",             "Контролен панел на преподавателя",          "Lecturer Dashboard")
    TR("teacher.add_student",       "Добави студент",                           "Add Student")
    TR("teacher.subjects",          "Дисциплини",                               "Courses")
    TR("teacher.statistics",        "Статистика",                               "Statistics")
    TR("teacher.add_grade",         "Постави оценка",                           "Add Grade")
    TR("teacher.view_profile",      "Профил",                                   "View")
    TR("teacher.sort.name",         "По име",                                   "By Name")
    TR("teacher.sort.avg_desc",     "Успех ↓",                                  "GPA ↓")
    TR("teacher.sort.avg_asc",      "Успех ↑",                                  "GPA ↑")
    TR("teacher.sort.program",      "Специалност",                              "Program")

    #undef TR
    return key;  // not found - returns the key itself so the omission is visible
}

// Convenience: Bulgarian month names (1-indexed)
inline const char* MonthShort(int m) {
    static const char* bg[] = {"", "ян","фев","март","апр","май","юни","юли","авг","сеп","окт","ное","дек"};
    static const char* en[] = {"", "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
    if (m < 1 || m > 12) return "";
    return (g_lang() == Lang::EN ? en[m] : bg[m]);
}

// Bulgarian day-of-week short labels (0=Mon..6=Sun)
inline const char* DayShort(int d) {
    static const char* bg[] = {"Пон","Вт","Ср","Четв","Пет","Съб","Нед"};
    static const char* en[] = {"Mon","Tue","Wed","Thu","Fri","Sat","Sun"};
    if (d < 0 || d > 6) return "";
    return (g_lang() == Lang::EN ? en[d] : bg[d]);
}

inline const char* DayLong(int d) {
    static const char* keys[] = {
        "schedule.day.mon", "schedule.day.tue", "schedule.day.wed",
        "schedule.day.thu", "schedule.day.fri", "schedule.day.sat", "schedule.day.sun"
    };
    if (d < 0 || d > 6) return "";
    return T(keys[d]);
}

} // namespace I18n
