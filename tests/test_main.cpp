#include "test_framework.h"

#include <exception>

namespace test {

namespace {

int g_checks = 0;
int g_failures = 0;

} // namespace

std::vector<TestCase>& Registry() {
    static std::vector<TestCase> registry;
    return registry;
}

Registrar::Registrar(const char* name, TestFn fn) {
    Registry().push_back(TestCase{name ? name : "<unnamed>", fn});
}

std::string ToUtf8(const std::wstring& text) {
    if (text.empty()) {
        return std::string();
    }
    int count = WideCharToMultiByte(CP_UTF8, 0, text.data(), (int)text.size(),
                                    nullptr, 0, nullptr, nullptr);
    if (count <= 0) {
        return std::string();
    }
    std::string out((size_t)count, '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), (int)text.size(),
                        &out[0], count, nullptr, nullptr);
    return out;
}

void ReportFailure(const char* file, int line, const std::string& message) {
    ++g_failures;
    std::printf("    FAIL %s:%d: %s\n", file, line, message.c_str());
}

void CountCheck() {
    ++g_checks;
}

int RunAll() {
    int tests = 0;
    int failed_tests = 0;
    for (const TestCase& testCase : Registry()) {
        ++tests;
        int failures_before = g_failures;
        std::printf("[ RUN      ] %s\n", testCase.name.c_str());
        try {
            testCase.fn();
        } catch (const std::exception& error) {
            ReportFailure(__FILE__, __LINE__,
                          std::string("uncaught exception: ") + error.what());
        } catch (...) {
            ReportFailure(__FILE__, __LINE__, "uncaught unknown exception");
        }
        if (g_failures == failures_before) {
            std::printf("[       OK ] %s\n", testCase.name.c_str());
        } else {
            ++failed_tests;
            std::printf("[  FAILED  ] %s\n", testCase.name.c_str());
        }
    }

    std::printf("\n%d test(s), %d failed, %d check(s), %d failure(s)\n",
                tests, failed_tests, g_checks, g_failures);
    return failed_tests == 0 ? 0 : 1;
}

} // namespace test

int main() {
    return test::RunAll();
}
