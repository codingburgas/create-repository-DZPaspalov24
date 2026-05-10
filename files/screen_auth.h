#pragma once
#include "screen.h"
#include "ui.h"
#include "models.h"

// =====================================================================
//  LoginScreen
//
//  Modelled on Bulgarian university student information systems.
//  Two-column hero card: branding panel on the left, form on the right.
//  Login is by *faculty number* (e.g. F23001) with password.
//  A small top-bar holds language (BG/EN) and theme toggles plus a
//  "What's new?" link that opens a marketing-style modal.
// =====================================================================
class LoginScreen : public Screen {
public:
    void OnEnter(App& app) override;
    void OnShow(App& app) override;
    void Update(App& app, float dt, Vector2 mouse) override;
    void Draw(App& app) override;
    const char* Name() const override { return "Login"; }

private:
    // Form widgets
    UI::TextInput facultyInput;
    UI::TextInput passInput;
    UI::Button    loginBtn{"Sign in"};
    UI::Button    googleBtn{"Sign in with Google"};
    UI::Button    signupBtn{"Create account"};
    UI::Button    forgotFnBtn{"Forgot fac. number"};
    UI::Button    forgotPassBtn{"Forgot password"};

    // Top-bar widgets
    UI::Button    whatNewBtn{"What's new"};
    UI::Button    langBtn{"BG / EN"};

    // Welcome / what's new modal
    UI::Modal     welcomeModal;
    UI::Button    welcomeOkBtn{"Go to Student"};

    // State
    std::string errorMsg;
    float       errorFade = 0.0f;
    float       enterT = 0.0f;
    bool        showPasswordPlain = false;
};

// =====================================================================
//  SignupScreen - single centred card (allows custom registration)
// =====================================================================
class SignupScreen : public Screen {
public:
    void OnEnter(App& app) override;
    void OnShow(App& app) override;
    void Update(App& app, float dt, Vector2 mouse) override;
    void Draw(App& app) override;
    const char* Name() const override { return "Signup"; }

private:
    UI::TextInput name;
    UI::TextInput user;
    UI::TextInput facultyNo;
    UI::TextInput pass;
    UI::TextInput pass2;
    UI::TextInput klass;
    UI::Button    submitBtn{"Sign up"};
    UI::Button    backBtn{"Already have an account?  Sign in"};
    Role          chosenRole = Role::STUDENT;

    std::string   errorMsg;
    float         errorFade = 0.0f;
    float         enterT = 0.0f;
};
