add_executable(DataStructureTest DataStructTest.cpp DataStructObserver.h DataStructObserver.cpp)
target_link_libraries(DataStructureTest PUBLIC complex)
set_target_properties(DataStructureTest PROPERTIES
    FOLDER "Tests"
)

add_test(NAME DataStructure COMMAND DataStructureTest)