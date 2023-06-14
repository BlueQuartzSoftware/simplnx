project(nxrunner
  VERSION 0.1.0
  DESCRIPTION "nxrunner"
  LANGUAGES CXX
)

set(CLI_HDRS
  ${CMAKE_CURRENT_SOURCE_DIR}/src/CliObserver.hpp
)

set(CLI_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/src/nxrunner.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/CliObserver.cpp
)

add_executable(nxrunner)

set_target_properties(nxrunner
  PROPERTIES
    DEBUG_POSTFIX "${COMPLEX_DEBUG_POSTFIX}"
)

target_sources(nxrunner
  PRIVATE
    ${CLI_HDRS}
    ${CLI_SRCS}
)

target_link_libraries(nxrunner PRIVATE complex::complex)

source_group("nxrunner" FILES ${CLI_HDRS} ${CLI_SRCS})

if(COMPLEX_EMBED_PYTHON)
  find_package(pybind11 2.10.0 CONFIG REQUIRED)

  target_link_libraries(nxrunner
    PRIVATE
      pybind11::embed
  )
endif()

# -----------------------------------------------------------------------
# Define install destination directories
# -----------------------------------------------------------------------
set(DEST_DIR ".") # Windows we dump everything into the top level dir

if(UNIX AND NOT APPLE) # Linux we obey standard unix layout
  set(DEST_DIR "bin") # Apple we have a custom script somewhere
endif()

# -- Create an Install Rule for the main app bundle target
install(TARGETS nxrunner
  COMPONENT Applications
  RUNTIME DESTINATION ${DEST_DIR}
  LIBRARY DESTINATION ${DEST_DIR}
  ARCHIVE DESTINATION ${DEST_DIR}
  BUNDLE DESTINATION ${DEST_DIR}
)

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  set(CMAKE_INSTALL_RPATH "\$ORIGIN/../lib")
  set_target_properties(nxrunner
    PROPERTIES
    INSTALL_RPATH \$ORIGIN/../lib
  )
  target_link_options(nxrunner PUBLIC "-Wl,--disable-new-dtags")
  target_compile_options(nxrunner PUBLIC "-no-pie")
endif()
