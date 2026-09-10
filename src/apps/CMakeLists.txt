project(apps
  VERSION 0.1.0
  DESCRIPTION "simplnx command line applications"
  LANGUAGES CXX
)

# -----------------------------------------------------------------------
# Define install destination directories
# -----------------------------------------------------------------------
set(DEST_DIR ".") # Windows we dump everything into the top level dir

if((UNIX AND NOT APPLE) OR SIMPLNX_CONDA_BUILD) # Linux we obey standard unix layout
  set(DEST_DIR "bin") # Apple we have a custom script somewhere
endif()

# -----------------------------------------------------------------------
# Applies the properties, link libraries, install rules and Linux RPATH
# settings that every simplnx CLI application shares.
# -----------------------------------------------------------------------
function(simplnx_configure_cli_app TARGET_NAME)
  set_target_properties(${TARGET_NAME}
    PROPERTIES
      DEBUG_POSTFIX "${SIMPLNX_DEBUG_POSTFIX}"
  )

  target_link_libraries(${TARGET_NAME} PRIVATE simplnx::simplnx)

  install(TARGETS ${TARGET_NAME}
    COMPONENT Applications
    RUNTIME DESTINATION ${DEST_DIR}
    LIBRARY DESTINATION ${DEST_DIR}
    ARCHIVE DESTINATION ${DEST_DIR}
    BUNDLE DESTINATION ${DEST_DIR}
  )

  if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set_target_properties(${TARGET_NAME}
      PROPERTIES
        INSTALL_RPATH \$ORIGIN/../lib
    )
    target_link_options(${TARGET_NAME} PUBLIC "-Wl,--disable-new-dtags")
    target_compile_options(${TARGET_NAME} PUBLIC "-no-pie")
  endif()
endfunction()

# -----------------------------------------------------------------------
# nxrunner: preflight / execute / convert pipelines
# -----------------------------------------------------------------------
set(NXRUNNER_HDRS
  ${CMAKE_CURRENT_SOURCE_DIR}/src/CliObserver.hpp
)

set(NXRUNNER_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/src/nxrunner.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/CliObserver.cpp
)

add_executable(nxrunner)

target_sources(nxrunner
  PRIVATE
    ${NXRUNNER_HDRS}
    ${NXRUNNER_SRCS}
)

source_group("nxrunner" FILES ${NXRUNNER_HDRS} ${NXRUNNER_SRCS})

simplnx_configure_cli_app(nxrunner)

if(SIMPLNX_EMBED_PYTHON)
  find_package(pybind11 2.10.0 CONFIG REQUIRED)

  target_link_libraries(nxrunner
    PRIVATE
      NxPythonEmbed
  )
endif()
