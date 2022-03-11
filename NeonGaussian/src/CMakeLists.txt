
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/inc")
add_executable(gaussian_example ${CMAKE_CURRENT_SOURCE_DIR}/test/example.c ${CMAKE_CURRENT_SOURCE_DIR}/test/gaussian3x3_neon.c)
install(TARGETS gaussian_example DESTINATION bin)
