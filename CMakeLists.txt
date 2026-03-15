cmake_minimum_required(VERSION 3.26)

################################################################################
# CMAKE OPTIONS
################################################################################

option(ENABLE_LOGGING "Enable file logging" ON)
option(ENABLE_TESTS "Build unit tests" OFF)
option(ENABLE_DEBUG "Enable debug mode" OFF)
option(ENABLE_CUDA "Enable compilation with CUDA" ON)
option(ENABLE_COVERAGE "Enable code coverage reporting" OFF)

# Validate option dependencies
if(ENABLE_COVERAGE AND NOT ENABLE_TESTS)
    message(FATAL_ERROR "ENABLE_COVERAGE requires ENABLE_TESTS=ON")
endif()

################################################################################
# PROJECT SETUP
################################################################################

# Conditionally add CUDA language support
if(ENABLE_CUDA)
    project(InstagramFilters LANGUAGES CXX CUDA)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CUDA_STANDARD 17)
else()
    project(InstagramFilters LANGUAGES CXX)
    set(CMAKE_CXX_STANDARD 17)
endif()

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(PROJECT_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# Generate configuration header from template
configure_file(
    "${PROJECT_SOURCE_DIR}/include/config.h.in"
    "${PROJECT_SOURCE_DIR}/include/config.h"
)

################################################################################
# COMPILER FLAGS & BUILD TYPE
################################################################################

if(ENABLE_DEBUG)
    set(CMAKE_BUILD_TYPE Debug)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g")
    if(ENABLE_CUDA)
        set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -O0 -g -G")
    endif()
    message(STATUS "Build type: DEBUG (-O0)")
else()
    set(CMAKE_BUILD_TYPE Release)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native")
    message(STATUS "Build type: RELEASE (-O3)")
endif()

################################################################################
# INCLUDE DIRECTORIES & DEPENDENCIES
################################################################################

include_directories(include)
include_directories(lib/stb)
include_directories(lib/spdlog/spdlog-1.14.1/include)

# Find CUDA toolkit if enabled
if(ENABLE_CUDA)
    find_package(CUDAToolkit REQUIRED)
    message(STATUS "CUDA found: ${CUDAToolkit_VERSION}")
    # set(CMAKE_CUDA_ARCHITECTURES 75)  # or 80, 86, 90 depending on your GPU
    set(CMAKE_CUDA_ARCHITECTURES native)  # Automatically detects your HPC GPU
    message(STATUS "CUDA architectures set to: ${CMAKE_CUDA_ARCHITECTURES}")

endif()

################################################################################
# REUSABLE CMAKE HELPER FUNCTIONS
################################################################################

# Helper function to add common CPU target properties
function(setup_target TARGET_NAME)
    # Add include directories
    target_include_directories(${TARGET_NAME} PRIVATE include)
    
    # Add logging compile definition if enabled
    if(ENABLE_LOGGING)
        target_compile_definitions(${TARGET_NAME} PRIVATE ENABLE_LOGGING=1)
    endif()
endfunction()

# Helper function to add CUDA-specific properties
function(setup_cuda_target TARGET_NAME)
    setup_target(${TARGET_NAME})

    # Add logging compile definition if enabled
    if(ENABLE_LOGGING)
        target_compile_definitions(${TARGET_NAME} PRIVATE ENABLE_LOGGING=1)
    endif()
    
    # ENABLE RELOCATABLE DEVICE CODE (REQUIRED FOR MULTI-FILE CUDA KERNELS)
    set_target_properties(${TARGET_NAME} PROPERTIES CUDA_SEPARABLE_COMPILATION ON)

    # CUDA definitions
    target_compile_definitions(${TARGET_NAME} PRIVATE ENABLE_CUDA=1)
    target_link_libraries(${TARGET_NAME}
        CUDA::cudart
        # CUDA::nppisu  # NPP single-channel
        # CUDA::nppicc  # NPP color conversion
        # CUDA::npp    # NPP arithmetic operations
        # For HPC environments with module-loaded CUDA
        nppc
        nppial
        nppisu
        nppicc
    )

endfunction()

################################################################################
# MAIN EXECUTABLES
################################################################################

# CPU-only executable
add_executable(instagram_filters
    apps/mainCPU.cpp
    src/io/imageIO.cpp
)
setup_target(instagram_filters)

# GPU executable (only if CUDA enabled)
if(ENABLE_CUDA)
    add_executable(instagram_filtersGPU
        apps/mainGPU.cpp
        src/io/imageIO.cpp
        src/filters/filters.cu
        src/filters/grayscale.cu
        src/filters/sepia.cu
        src/filters/brightness.cu
    )
    setup_cuda_target(instagram_filtersGPU)
    message(STATUS "GPU executable enabled")
else()
    message(STATUS "GPU executable disabled (ENABLE_CUDA=OFF)")
endif()

################################################################################
# GOOGLE TEST SETUP
################################################################################

if(ENABLE_TESTS)
    enable_testing()
    include(GoogleTest)

    # Find or fetch Google Test
    find_package(GTest CONFIG QUIET)
    if(NOT GTest_FOUND)
        message(STATUS "Fetching Google Test v1.15.0...")
        include(FetchContent)
        FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.15.0.zip
        )
        set(gtest_disable_pthreads OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    endif()

    message(STATUS "Testing enabled")
endif()

################################################################################
# CPU TESTS
################################################################################

if(ENABLE_TESTS)

    add_executable(imageIO_test
        tests/test_imageIO.cpp
        src/io/imageIO.cpp
    )
    target_link_libraries(imageIO_test
        GTest::gtest
        GTest::gtest_main
    )
    setup_target(imageIO_test)

    add_executable(cpu_filters_test
        tests/test_cpu_filters.cpp
        src/io/imageIO.cpp
    )
    target_link_libraries(cpu_filters_test
        GTest::gtest
        GTest::gtest_main
    )
    setup_target(cpu_filters_test)

    # Coverage flags for test target only
    if(ENABLE_COVERAGE)
        target_compile_options(imageIO_test PRIVATE
            -O0 -g --coverage
        )
        target_link_options(imageIO_test PRIVATE --coverage)
        target_compile_options(cpu_filters_test PRIVATE
            -O0 -g --coverage
        )
        target_link_options(cpu_filters_test PRIVATE --coverage)
    endif()

    # Set working directory for tests
    set_target_properties(imageIO_test PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    set_target_properties(cpu_filters_test PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )

    # Register tests
    gtest_discover_tests(imageIO_test
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    gtest_discover_tests(cpu_filters_test
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )

    message(STATUS "CPU tests registered")
endif()

################################################################################
# GPU TESTS
################################################################################

if(ENABLE_TESTS AND ENABLE_CUDA)
    add_executable(gpu_filters_test
        tests/test_gpu_filters.cpp
        src/filters/filters.cu
        src/filters/grayscale.cu
        src/filters/sepia.cu
        src/filters/brightness.cu
        src/io/imageIO.cpp
    )

    target_link_libraries(gpu_filters_test
        GTest::gtest
        GTest::gtest_main
        CUDA::cudart
        # CUDA::nppisu
        # CUDA::nppicc
        # CUDA::npp
        nppc
        nppial
        nppisu
        nppicc
    )

    setup_cuda_target(gpu_filters_test)

    # Set working directory for tests
    set_target_properties(gpu_filters_test PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )

    # Register tests
    gtest_discover_tests(gpu_filters_test
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )

    message(STATUS "GPU tests registered")
endif()

################################################################################
# CODE COVERAGE REPORTING
################################################################################

if(ENABLE_COVERAGE)
    find_program(LCOV_BIN lcov REQUIRED)
    find_program(GENHTML_BIN genhtml REQUIRED)

    set(COVERAGE_DIR     ${CMAKE_BINARY_DIR}/coverage)
    set(COVERAGE_INFO    ${COVERAGE_DIR}/coverage.info)
    set(COVERAGE_CLEAN   ${COVERAGE_DIR}/coverage_filtered.info)

    file(MAKE_DIRECTORY ${COVERAGE_DIR})

    add_custom_target(coverage
        COMMENT "Running tests and generating coverage report..."

        # Reset counters
        COMMAND ${LCOV_BIN} --zerocounters --directory ${CMAKE_BINARY_DIR}

        # Run CPU tests
        COMMAND ${CMAKE_BINARY_DIR}/../bin/instagram_filters_test

        # Capture coverage data
        COMMAND ${LCOV_BIN}
            --capture
            --directory ${CMAKE_BINARY_DIR}
            --output-file ${COVERAGE_INFO}
            --ignore-errors unsupported,unsupported
            --ignore-errors inconsistent,inconsistent
            --ignore-errors format,format
            --ignore-errors mismatch,mismatch
            --ignore-errors unused,unused

        # Filter out noise
        COMMAND ${LCOV_BIN}
            --remove ${COVERAGE_INFO}
                '/usr/include/*'
                '/usr/local/*'
                '/opt/*'
                '*/googletest*'
                '*/stb/*'
                '*/spdlog*'
            --output-file ${COVERAGE_CLEAN}
            --ignore-errors unsupported,unsupported
            --ignore-errors inconsistent,inconsistent
            --ignore-errors format,format
            --ignore-errors mismatch,mismatch
            --ignore-errors unused,unused

        # Generate HTML report
        COMMAND ${GENHTML_BIN}
            ${COVERAGE_CLEAN}
            --output-directory ${COVERAGE_DIR}/html
            --title "InstagramFilters Coverage"
            --legend
            --show-details
            --ignore-errors category,category
            --ignore-errors format,format
            --ignore-errors mismatch,mismatch
            --ignore-errors unused,unused

        # Print status
        COMMAND ${CMAKE_COMMAND} -E echo ""
        COMMAND ${CMAKE_COMMAND} -E echo "✅ Coverage report generated:"
        COMMAND ${CMAKE_COMMAND} -E echo "   open ${COVERAGE_DIR}/html/index.html"
        COMMAND ${CMAKE_COMMAND} -E echo ""

        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )

    message(STATUS "Coverage reporting enabled")
endif()

################################################################################
# BUILD SUMMARY
################################################################################

message(STATUS "")
message(STATUS "=== Build Configuration ===")
message(STATUS "  CUDA Support:     ${ENABLE_CUDA}")
message(STATUS "  Tests:            ${ENABLE_TESTS}")
message(STATUS "  Coverage:         ${ENABLE_COVERAGE}")
message(STATUS "  Logging:          ${ENABLE_LOGGING}")
message(STATUS "  Debug Mode:       ${ENABLE_DEBUG}")
message(STATUS "  Executables Dir:  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "===========================")
message(STATUS "")