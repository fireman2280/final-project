file(GLOB_RECURSE HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

add_library(platform-tiny32-mc
ethernet.cpp
display.cpp
${HEADERS})

target_include_directories(platform-tiny32-mc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(platform-tiny32-mc systemc)

add_executable(tiny32-mc
        mc_main.cpp)

target_link_libraries(tiny32-mc rv32 platform-tiny32-mc  platform-common gdb-mc ${Boost_LIBRARIES} systemc pthread)

INSTALL(TARGETS tiny32-mc RUNTIME DESTINATION bin)
