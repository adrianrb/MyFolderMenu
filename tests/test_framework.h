#pragma once

#include <windows.h>

#include <cstdio>
#include <sstream>
#include <string>
#include <vector>

namespace test {

using TestFn = void (*)();

struct TestCase {
    std::string name;
    TestFn fn;
};

std::vector<TestCase>& Registry();

struct Registrar {
    Registrar(const char* name, TestFn fn);
};

void ReportFailure(const char* file, int line, const std::string& message);
void CountCheck();
int RunAll();

std::string ToUtf8(const std::wstring& text);

inline std::string Describe(const std::wstring& value) {
    return "\"" + ToUtf8(value) + "\"";
}

inline std::string Describe(const wchar_t* value) {
    return value ? Describe(std::wstring(value)) : std::string("null");
}

template <typename T>
std::string Describe(const T& value) {
    std::ostringstream stream;
    stream << value;
    return stream.str();
}

} // namespace test

#define TEST(name)                                                          \
    static void name();                                                     \
    static ::test::Registrar test_registrar_##name(#name, name);            \
    static void name()

#define CHECK(condition)                                                    \
    do {                                                                    \
        ::test::CountCheck();                                               \
        if (!(condition)) {                                                 \
            ::test::ReportFailure(__FILE__, __LINE__,                       \
                                  "CHECK failed: " #condition);             \
        }                                                                   \
    } while (0)

#define CHECK_EQ(actual, expected)                                          \
    do {                                                                    \
        ::test::CountCheck();                                               \
        auto&& check_actual_ = (actual);                                    \
        auto&& check_expected_ = (expected);                                \
        if (!(check_actual_ == check_expected_)) {                          \
            ::test::ReportFailure(                                          \
                __FILE__, __LINE__,                                         \
                std::string("CHECK_EQ failed: " #actual " == " #expected    \
                            " (got ") +                                     \
                    ::test::Describe(check_actual_) + ", expected " +       \
                    ::test::Describe(check_expected_) + ")");               \
        }                                                                   \
    } while (0)

#define CHECK_NE(actual, expected)                                          \
    do {                                                                    \
        ::test::CountCheck();                                               \
        auto&& check_actual_ = (actual);                                    \
        auto&& check_expected_ = (expected);                                \
        if (check_actual_ == check_expected_) {                             \
            ::test::ReportFailure(                                          \
                __FILE__, __LINE__,                                         \
                std::string("CHECK_NE failed: " #actual " != " #expected)); \
        }                                                                   \
    } while (0)

#define CHECK_GT(actual, expected)                                          \
    do {                                                                    \
        ::test::CountCheck();                                               \
        auto&& check_actual_ = (actual);                                    \
        auto&& check_expected_ = (expected);                                \
        if (!(check_actual_ > check_expected_)) {                           \
            ::test::ReportFailure(                                          \
                __FILE__, __LINE__,                                         \
                std::string("CHECK_GT failed: " #actual " > " #expected));  \
        }                                                                   \
    } while (0)
