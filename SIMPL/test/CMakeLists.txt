#------------------------------------------------------------------------------
# If there are additional library packages that need to be found, this is where
# to do that using the usual find_package(....) call
# find_package(....)

#------------------------------------------------------------------------------
# Define the list of unit test source files
set(${PLUGIN_NAME}UnitTest_SRCS
    #ArrayCalculatorTest.cpp    # MISSING 1 or more Parameter Implementations
    CombineAttributeArraysTest.cpp
    CombineAttributeMatricesTest.cpp
    ConvertColorToGrayScaleTest.cpp
    ConvertDataTest.cpp
    CopyFeatureArrayToElementArrayTest.cpp
    CopyObjectTest.cpp
    #CreateAttributeMatrixTest.cpp    # MISSING 1 or more Parameter Implementations
    #CreateDataArrayTest.cpp    # MISSING 1 or more Parameter Implementations
    CreateDataContainerTest.cpp
    CreateFeatureArrayFromElementArrayTest.cpp
    CreateGeometryTest.cpp
    CreateGridMontageTest.cpp
    CreateStringArrayTest.cpp
    #DataContainerReaderTest.cpp    # MISSING 1 or more Parameter Implementations
    ExecuteProcessTest.cpp
    ExtractAttributeArraysFromGeometryTest.cpp
    ExtractComponentAsArrayTest.cpp
    ExtractVertexGeometryTest.cpp
    FeatureCountDecisionTest.cpp
    FeatureDataCSVWriterTest.cpp
    FindDerivativesTest.cpp
    #GenerateColorTableTest.cpp    # MISSING 1 or more Parameter Implementations
    GenerateTiltSeriesTest.cpp
    GenerateVertexCoordinatesTest.cpp
    ImportAsciDataArrayTest.cpp
    #ImportHDF5DatasetTest.cpp    # MISSING 1 or more Parameter Implementations
    #InitializeDataTest.cpp    # MISSING 1 or more Parameter Implementations
    LinkFeatureMapToElementArrayTest.cpp
    MaskCountDecisionTest.cpp
    MoveDataTest.cpp
    #MoveMultiDataTest.cpp    # MISSING 1 or more Parameter Implementations
    #MultiThresholdObjectsTest.cpp    # MISSING 1 or more Parameter Implementations
    #MultiThresholdObjects2Test.cpp    # MISSING 1 or more Parameter Implementations
    #PipelineAnnotationTest.cpp    # MISSING 1 or more Parameter Implementations
    PostSlackMessageTest.cpp
    #ReadASCIIDataTest.cpp    # MISSING 1 or more Parameter Implementations
    #RemoveArraysTest.cpp    # MISSING 1 or more Parameter Implementations
    RemoveComponentFromArrayTest.cpp
    RenameAttributeArrayTest.cpp
    RenameAttributeMatrixTest.cpp
    RenameDataContainerTest.cpp
    ReplaceValueInArrayTest.cpp
    RequiredZThicknessTest.cpp
    #RotateSampleRefFrameTest.cpp    # MISSING 1 or more Parameter Implementations
    ScaleVolumeTest.cpp
    SplitAttributeArrayTest.cpp
    WriteASCIIDataTest.cpp
    WriteTriangleGeometryTest.cpp

)

create_complex_plugin_unit_test(PLUGIN_NAME ${PLUGIN_NAME}
                                FILTER_LIST ${${PLUGIN_NAME}UnitTest_SRCS})

#------------------------------------------------------------------------------
# If there are additional libraries that this plugin needs to link against you
# can use the target_link_libraries() cmake call
# target_link_libraries(${PLUGIN_NAME}UnitTest PUBLIC [name of library])

#------------------------------------------------------------------------------
# If there are additional source files that need to be compiled for this plugin
# can use the target_sources(...) cmake call
# target_sources(${PLUGIN_NAME}UnitTest
#   PUBLIC
#     additional header files
#   PRIVATE
#     additional source files
# )

#------------------------------------------------------------------------------
# If there are additional include directories that are needed for this plugin
# you can use the target_include_directories(.....) cmake call
# target_include_directories(${PLUGIN_NAME}UnitTest
#     PUBLIC
#     additional include directories here
# )
