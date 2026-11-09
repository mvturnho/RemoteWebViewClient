cmake_minimum_required(VERSION 3.16)
project(remote_webview_tests LANGUAGES CXX)

enable_testing()

add_executable(rgb565_contract_test rgb565_contract_test.cpp)
target_compile_features(rgb565_contract_test PRIVATE cxx_std_17)
target_include_directories(rgb565_contract_test PRIVATE ../components/remote_webview)

add_test(NAME rgb565_contract_test COMMAND rgb565_contract_test)
