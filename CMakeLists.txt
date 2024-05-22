cmake_minimum_required(VERSION 3.10)
project(ex_project)

set(CMAKE_C_STANDARD 99)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(ex ex.c)
target_link_libraries(ex PRIVATE Threads::Threads)

add_custom_target(run_ex
    COMMAND ex 100 123
    DEPENDS ex
    WORKING_DIRECTORY ${CMAKE_PROJECT_DIR}
)