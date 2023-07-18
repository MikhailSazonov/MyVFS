#include <cassert>
#include <iostream>
#include <string>

void success(const std::string& section) {
    std::cout << "Section '" << section << "' passed successfully!\n===========\n";
}

void test(auto f, const std::string& test_name) {
    f();
    success(test_name);
}

template <typename T, typename U>
void assert_equal(const T& left, const U& right) {
    if (left != right) {
        std::cout << "Equal failed:\nexpected\n\n" << right <<
            "\nbut got\n\n" << left << '\n';
        assert(false);
    }
}

template <typename T, typename U>
void assert_equal_containers(const T& left, const U& right, size_t S) {
    for (size_t i = 0; i < S; ++i) {
        if (std::abs(left[i] - right[i]) > 1e-9) {
            std::cout << "Equal failed:\nexpected\n\n" << right[i] <<
                "\nbut got\n\n" << left[i] << " at " << i << '\n';
            assert(false);
        }
    }
}

template <typename T>
void assert_zero(const T& obj) {
    assert_equal(obj, T(0., 0., 0.));
}

void assert_throw(auto op) {
    try {
        op();
        assert(false); // unreachable
    } catch (...) {}
}

void section(const std::string& section_name) {  
    std::cout << "Section " << section_name << "\n###########\n";
}
