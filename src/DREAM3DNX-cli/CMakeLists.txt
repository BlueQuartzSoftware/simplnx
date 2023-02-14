project(DREAM3DNX-cli
  VERSION 0.1.0
  DESCRIPTION "DREAM.3D NX CLI"
  LANGUAGES CXX)

set(CLI_HDRS
  ${CMAKE_CURRENT_SOURCE_DIR}/src/CliObserver.hpp
)

set(CLI_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/src/DREAM3DNX_cli.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/CliObserver.cpp
)

add_executable(DREAM3DNX-cli)

set_target_properties(DREAM3DNX-cli
  PROPERTIES
  DEBUG_POSTFIX "${COMPLEX_DEBUG_POSTFIX}"
)

target_sources(DREAM3DNX-cli
  PRIVATE
  ${CLI_HDRS}
  ${CLI_SRCS}
)

target_link_libraries(DREAM3DNX-cli PRIVATE complex::complex)

source_group("DREAM.3D NX CLI" FILES ${CLI_HDRS} ${CLI_SRCS})

# -----------------------------------------------------------------------
# Define install destination directories
# -----------------------------------------------------------------------
set(DEST_DIR ".") # Windows we dump everything into the top level dir

if(UNIX AND NOT APPLE) # Linux we obey standard unix layout
  set(DEST_DIR "bin") # Apple we have a custom script somewhere
endif()

# -- Create an Install Rule for the main app bundle target
install(TARGETS DREAM3DNX-cli
  COMPONENT Applications
  RUNTIME DESTINATION ${DEST_DIR}
  LIBRARY DESTINATION ${DEST_DIR}
  ARCHIVE DESTINATION ${DEST_DIR}
  BUNDLE DESTINATION ${DEST_DIR}
)

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  set(CMAKE_INSTALL_RPATH "\$ORIGIN/../lib")
  set_target_properties(DREAM3DNX-cli
    PROPERTIES
    INSTALL_RPATH \$ORIGIN/../lib)
  target_link_options(DREAM3DNX-cli PUBLIC "-Wl,--disable-new-dtags")
  target_compile_options(DREAM3DNX-cli PUBLIC "-no-pie")
endif()
