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
  DEBUG_POSTFIX "${COMPLEX_DEBUG_POSTFIX}"
  )


target_sources(PipelineRunner
  PRIVATE
    ${PipelineRunner_HDRS}
    ${PipelineRunner_SRCS}
    )

target_link_libraries(PipelineRunner PRIVATE complex::complex)

source_group("PiplineRunner" FILES ${PipelineRunner_HDRS} ${PipelineRunner_SRCS})


# -----------------------------------------------------------------------
# Define install destination directories
# -----------------------------------------------------------------------
set(DEST_DIR ".")  # Windows we dump everything into the top level dir
if(UNIX AND NOT APPLE) # Linux we obey standard unix layout
    set(DEST_DIR "bin") # Apple we have a custom script somewhere
endif()
#-- Create an Install Rule for the main app bundle target
install(TARGETS PipelineRunner
    COMPONENT Applications
    RUNTIME DESTINATION ${DEST_DIR}
    LIBRARY DESTINATION ${DEST_DIR}
    ARCHIVE DESTINATION ${DEST_DIR}
    BUNDLE DESTINATION ${DEST_DIR}
)

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  set(CMAKE_INSTALL_RPATH "\$ORIGIN/../lib")
  set_target_properties( PipelineRunner
        PROPERTIES
        INSTALL_RPATH \$ORIGIN/../lib )
  target_link_options(PipelineRunner PUBLIC "-Wl,--disable-new-dtags")
  target_compile_options(PipelineRunner PUBLIC "-no-pie")
endif()
