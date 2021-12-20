project(PipelineRunner
  VERSION 0.1.0
  DESCRIPTION "complex::PipelineRunner"
  LANGUAGES CXX)

add_executable(PipelineRunner)

target_sources(PipelineRunner
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PipelineRunner.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PRObserver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PRObserver.hpp
    )

target_link_libraries(PipelineRunner complex::complex)