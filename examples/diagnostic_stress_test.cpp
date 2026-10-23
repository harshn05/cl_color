// Intentionally broken file that produces a mix of warnings, errors and notes.
// Try:  cl_color cl /nologo /W4 /FC /diagnostics:caret /EHsc diagnostic_stress_test.cpp
#include <string>

[[deprecated("Use modern_compute() instead")]]
int legacy_compute(int x) { return x * 2; }

int modern_compute(int x) { return x * 3; }

struct NoAdditionOperator {};

template <typename T>
auto add_values(const T& left, const T& right) {
    return left + right;
}

struct Sample {
    int value = 0;
};

int warnings() {
    int signed_count = -1;
    unsigned int unsigned_count = 1;
    if (signed_count < unsigned_count) {       // Signed/unsigned comparison warning
        return 1;
    }
    double measurement = 3.14;
    int truncated_measurement = measurement;   // Conversion warning
    int old_result = legacy_compute(10);       // Deprecated-function warning
    return truncated_measurement + old_result + modern_compute(1);
}

int undeclared() {
    return variable_that_does_not_exist + 1;   // Undeclared identifier
}

int wrong_return() {
    std::string text = "hello";
    return text;                               // Cannot convert std::string to int
}

int templates() {
    NoAdditionOperator a, b;
    (void)add_values(a, b);                    // Template instantiation error with notes
    return 0;
}

int syntax() {
    int result = ;                             // Intentional syntax error
    return result;
}

int main() {
    Sample sample;
    return sample.missing_member;              // Not a member of 'Sample'
}
