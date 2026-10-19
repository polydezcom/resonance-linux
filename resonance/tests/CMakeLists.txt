function(resonance_add_test name)
    add_executable(${name} ${ARGN})
    target_include_directories(${name} PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_compile_options(${name} PRIVATE ${RESONANCE_WARNING_FLAGS})
    target_link_libraries(${name} PRIVATE Qt6::Test ${RESONANCE_TEST_LIBRARIES})
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
endfunction()

set(RESONANCE_TEST_LIBRARIES resonance_core)

resonance_add_test(test_ring_buffer test_ring_buffer.cpp)
resonance_add_test(test_biquad test_biquad.cpp)
resonance_add_test(test_audio_engine test_audio_engine.cpp)

set(RESONANCE_TEST_LIBRARIES resonance_core resonance_library)
resonance_add_test(test_natural_sort test_natural_sort.cpp)
resonance_add_test(test_folder_scanner test_folder_scanner.cpp)
