project(PipelineRunner
  VERSION 0.1.0
  DESCRIPTION "complex::PipelineRunner"
  LANGUAGES CXX)


set(PipelineRunner_HDRS
  ${CMAKE_CURRENT_SOURCE_DIR}/src/PRObserver.hpp
)

set(PipelineRunner_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/src/PipelineRunner.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/PRObserver.cpp
)

add_executable(PipelineRunner)


set_target_properties(PipelineRunner
  PROPERTIES
    DEBUG_POSTFIX "_d"
)


target_sources(PipelineRunner
  PRIVATE
    ${PipelineRunner_HDRS}
    ${PipelineRunner_SRCS}
    )

target_link_libraries(PipelineRunner PRIVATE complex::complex)

source_group("PiplineRunner" FILES ${PipelineRunner_HDRS} ${PipelineRunner_SRCS})

