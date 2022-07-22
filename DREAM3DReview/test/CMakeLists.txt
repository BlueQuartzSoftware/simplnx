#------------------------------------------------------------------------------
# If there are additional library packages that need to be found, this is where
# to do that using the usual find_package(....) call
# find_package(....)

#------------------------------------------------------------------------------
# Define the list of unit test source files
set(${PLUGIN_NAME}UnitTest_SRCS
    AdaptiveAlignmentFeatureTest.cpp
    AdaptiveAlignmentMisorientationTest.cpp
    AdaptiveAlignmentMutualInformationTest.cpp
    AlignGeometriesTest.cpp
    #ApplyTransformationToGeometryTest.cpp    # MISSING 1 or more Parameter Implementations
    ApproximatePointCloudHullTest.cpp
    AverageEdgeFaceCellArrayToVertexArrayTest.cpp
    AverageVertexArrayToEdgeFaceCellArrayTest.cpp
    CombineStlFilesTest.cpp
    ComputeFeatureEigenstrainsTest.cpp
    ComputeUmeyamaTransformTest.cpp
    CreateArrayofIndicesTest.cpp
    DBSCANTest.cpp
    DelaunayTriangulationTest.cpp
    DiscretizeDDDomainTest.cpp
    DownsampleVertexGeometryTest.cpp
    EstablishFoamMorphologyTest.cpp
    ExportCLIFileTest.cpp
    ExtractTripleLinesFromTriangleGeometryTest.cpp
    FFTHDFWriterFilterTest.cpp
    FindCSLBoundariesTest.cpp
    FindElementCentroidsTest.cpp
    FindLayerStatisticsTest.cpp
    FindMinkowskiBouligandDimensionTest.cpp
    FindNeighborListStatisticsTest.cpp
    FindNormTest.cpp
    FindSurfaceRoughnessTest.cpp
    FindVertexToTriangleDistancesTest.cpp
    GenerateFeatureIDsbyBoundingBoxesTest.cpp
    GenerateMaskFromSimpleShapesTest.cpp
    IdentifyDislocationSegmentsTest.cpp
    ImportCLIFileTest.cpp
    ImportMASSIFDataTest.cpp
    ImportPrintRiteHDF5FileTest.cpp
    ImportPrintRiteTDMSFilesTest.cpp
    #ImportQMMeltpoolH5FileTest.cpp    # MISSING 1 or more Parameter Implementations
    ImportQMMeltpoolTDMSFileTest.cpp
    ImportVolumeGraphicsFileTest.cpp
    InsertTransformationPhasesTest.cpp
    InterpolatePointCloudToRegularGridTest.cpp
    IterativeClosestPointTest.cpp
    KDistanceGraphTest.cpp
    KMeansTest.cpp
    KMedoidsTest.cpp
    LabelTriangleGeometryTest.cpp
    LaplacianSmoothPointCloudTest.cpp
    LocalDislocationDensityCalculatorTest.cpp
    MapPointCloudToRegularGridTest.cpp
    NormalizeArraysTest.cpp
    ParaDisReaderTest.cpp
    PointSampleTriangleGeometryTest.cpp
    PottsModelTest.cpp
    PrincipalComponentAnalysisTest.cpp
    ReadBinaryCTNorthStarTest.cpp
    ReadMicDataTest.cpp
    RemoveFlaggedVerticesTest.cpp
    SilhouetteTest.cpp
    SliceTriangleGeometryTest.cpp
    SteinerCompactTest.cpp
    TesselateFarFieldGrainsTest.cpp
    TiDwellFatigueCrystallographicAnalysisTest.cpp
    WaveFrontObjectFileWriterTest.cpp

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
