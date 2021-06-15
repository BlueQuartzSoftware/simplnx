cmake_minimum_required(VERSION 3.18)

project(complex
    VERSION 0.0.1
    DESCRIPTION "SIMPL Redesign")

# Setup
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN YES)

cmake_policy(SET CMP0090 NEW)

# turn on using solution folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(complex_INCLUDE_DIR ${complex_SOURCE_DIR}/Complex)

set(complex_HDRS
    ${complex_INCLUDE_DIR}/Common/BoundingBox.hpp
    ${complex_INCLUDE_DIR}/Common/EulerAngle.h
    ${complex_INCLUDE_DIR}/Common/Point2D.hpp
    ${complex_INCLUDE_DIR}/Common/Point3D.hpp
    ${complex_INCLUDE_DIR}/Common/Ray.hpp
    ${complex_INCLUDE_DIR}/Common/RgbColor.h
    ${complex_INCLUDE_DIR}/Common/Array.hpp

    ${complex_INCLUDE_DIR}/Core/Application.h
    ${complex_INCLUDE_DIR}/Core/FilterHandle.h
    ${complex_INCLUDE_DIR}/Core/FilterList.h

    ${complex_INCLUDE_DIR}/DataStructure/Messaging/AbstractDataStructureMessage.h
    ${complex_INCLUDE_DIR}/DataStructure/Messaging/DataAddedMessage.h
    ${complex_INCLUDE_DIR}/DataStructure/Messaging/DataRemovedMessage.h
    ${complex_INCLUDE_DIR}/DataStructure/Messaging/DataRenamedMessage.h
    ${complex_INCLUDE_DIR}/DataStructure/Messaging/DataReparentedMessage.h

    ${complex_INCLUDE_DIR}/DataStructure/Observers/AbstractDataStructureObserver.h

    ${complex_INCLUDE_DIR}/DataStructure/BaseGroup.h
    ${complex_INCLUDE_DIR}/DataStructure/DataArray.hpp
    ${complex_INCLUDE_DIR}/DataStructure/DataGroup.h
    ${complex_INCLUDE_DIR}/DataStructure/DataMap.h
    ${complex_INCLUDE_DIR}/DataStructure/DataObject.h
    ${complex_INCLUDE_DIR}/DataStructure/DataPath.h
    ${complex_INCLUDE_DIR}/DataStructure/DataStore.hpp
    ${complex_INCLUDE_DIR}/DataStructure/DataStructure.h
    ${complex_INCLUDE_DIR}/DataStructure/LinkedPath.h
    ${complex_INCLUDE_DIR}/DataStructure/MetaData.h
    ${complex_INCLUDE_DIR}/DataStructure/ScalarData.hpp

    ${complex_INCLUDE_DIR}/Plugin/AbstractPlugin.h
    ${complex_INCLUDE_DIR}/Plugin/PluginLoader.h

    # ${complex_INCLUDE_DIR}/Rest/PipelineController.h
    # ${complex_INCLUDE_DIR}/Rest/PipelineHandler.h
    # ${complex_INCLUDE_DIR}/Rest/RestManager.h
    # ${complex_INCLUDE_DIR}/Rest/RestServer.h

    ${complex_INCLUDE_DIR}/Utilities/Math/GeometryMath.hpp

    ${complex_INCLUDE_DIR}/Utilities/Parsing/HDF5/H5.h
    
    ${complex_INCLUDE_DIR}/Utilities/TooltipGenerator.h
    ${complex_INCLUDE_DIR}/Utilities/TooltipRowItem.h
)

set(complex_SRCS
    ${complex_INCLUDE_DIR}/Common/RgbColor.cpp

    ${complex_INCLUDE_DIR}/Core/Application.cpp
    ${complex_INCLUDE_DIR}/Core/FilterHandle.cpp
    ${complex_INCLUDE_DIR}/Core/FilterList.cpp

    ${complex_INCLUDE_DIR}/DataStructure/Messaging/AbstractDataStructureMessage.cpp
    ${complex_INCLUDE_DIR}/DataStructure/Messaging/DataAddedMessage.cpp
    ${complex_INCLUDE_DIR}/DataStructure/Messaging/DataRemovedMessage.cpp
    ${complex_INCLUDE_DIR}/DataStructure/Messaging/DataRenamedMessage.cpp
    ${complex_INCLUDE_DIR}/DataStructure/Messaging/DataReparentedMessage.cpp

    ${complex_INCLUDE_DIR}/DataStructure/Observers/AbstractDataStructureObserver.cpp

    ${complex_INCLUDE_DIR}/DataStructure/BaseGroup.cpp
    ${complex_INCLUDE_DIR}/DataStructure/DataGroup.cpp
    ${complex_INCLUDE_DIR}/DataStructure/DataMap.cpp
    ${complex_INCLUDE_DIR}/DataStructure/DataObject.cpp
    ${complex_INCLUDE_DIR}/DataStructure/DataPath.cpp
    ${complex_INCLUDE_DIR}/DataStructure/DataStructure.cpp
    ${complex_INCLUDE_DIR}/DataStructure/LinkedPath.cpp
    ${complex_INCLUDE_DIR}/DataStructure/MetaData.cpp

    ${complex_INCLUDE_DIR}/Plugin/AbstractPlugin.cpp

    # ${complex_INCLUDE_DIR}/Rest/PipelineController.cpp
    # ${complex_INCLUDE_DIR}/Rest/PipelineHandler.cpp
    # ${complex_INCLUDE_DIR}/Rest/RestManager.cpp
    # ${complex_INCLUDE_DIR}/Rest/RestServer.cpp

    ${complex_INCLUDE_DIR}/Utilities/TooltipGenerator.cpp
    ${complex_INCLUDE_DIR}/Utilities/TooltipRowItem.cpp
)

if(WIN32)
    set(complex_SRCS
        ${complex_SRCS}
        ${complex_INCLUDE_DIR}/Plugin/PluginLoader_Win.cpp
    )
elseif(UNIX)
    set(complex_SRCS
        ${complex_SRCS}
        ${complex_INCLUDE_DIR}/Plugin/PluginLoader_Linux.cpp
    )
endif()

source_group(TREE ${complex_INCLUDE_DIR} PREFIX "Complex" FILES ${complex_HDRS})
source_group(TREE ${complex_INCLUDE_DIR} PREFIX "Complex" FILES ${complex_SRCS})

add_library(${PROJECT_NAME} STATIC ${complex_HDRS} ${complex_SRCS})

include(GenerateExportHeader)
generate_export_header(${PROJECT_NAME})

set_target_properties(${PROJECT_NAME} PROPERTIES
    LINKER_LANGUAGE CXX
    PUBLIC_HEADER "${complex_HDRS}"
)

# Set include directories
target_include_directories(${PROJECT_NAME}
    PUBLIC
    ${complex_SOURCE_DIR}/include
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/${PROJECT_NAME}>
)

include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
    # PUBLIC_HEADER DESTINATION include/Complex
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT   complex_Runtime
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        COMPONENT   complex_Runtime
        NAMELINK_COMPONENT complex_Development
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        COMPONENT   complex_Development
)

install(DIRECTORY ${complex_INCLUDE_DIR} DESTINATION include)

include(CTest)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()