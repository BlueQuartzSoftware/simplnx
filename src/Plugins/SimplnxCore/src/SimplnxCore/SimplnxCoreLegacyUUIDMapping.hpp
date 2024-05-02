#pragma once

#include "simplnx/Plugin/AbstractPlugin.hpp"

#include <nlohmann/json.hpp>

#include <map>
#include <string>

// clang-format off
#include "SimplnxCore/Filters/AlignGeometriesFilter.hpp"
#include "SimplnxCore/Filters/AlignSectionsFeatureCentroidFilter.hpp"
#include "SimplnxCore/Filters/AlignSectionsListFilter.hpp"
#include "SimplnxCore/Filters/ApplyTransformationToGeometryFilter.hpp"
#include "SimplnxCore/Filters/ApproximatePointCloudHullFilter.hpp"
#include "SimplnxCore/Filters/ArrayCalculatorFilter.hpp"
#include "SimplnxCore/Filters/CalculateArrayHistogramFilter.hpp"
#include "SimplnxCore/Filters/CalculateFeatureSizesFilter.hpp"
#include "SimplnxCore/Filters/CalculateTriangleAreasFilter.hpp"
#include "SimplnxCore/Filters/ChangeAngleRepresentationFilter.hpp"
#include "SimplnxCore/Filters/CombineAttributeArraysFilter.hpp"
#include "SimplnxCore/Filters/ConditionalSetValueFilter.hpp"
#include "SimplnxCore/Filters/ConvertColorToGrayScaleFilter.hpp"
#include "SimplnxCore/Filters/ConvertDataFilter.hpp"
#include "SimplnxCore/Filters/CopyDataObjectFilter.hpp"
#include "SimplnxCore/Filters/CopyFeatureArrayToElementArrayFilter.hpp"
#include "SimplnxCore/Filters/CreateAttributeMatrixFilter.hpp"
#include "SimplnxCore/Filters/CreateDataArrayFilter.hpp"
#include "SimplnxCore/Filters/CreateDataGroupFilter.hpp"
#include "SimplnxCore/Filters/CreateFeatureArrayFromElementArrayFilter.hpp"
#include "SimplnxCore/Filters/CreateImageGeometryFilter.hpp"
#include "SimplnxCore/Filters/CropImageGeometryFilter.hpp"
#include "SimplnxCore/Filters/CropVertexGeometryFilter.hpp"
#include "SimplnxCore/Filters/DeleteDataFilter.hpp"
#include "SimplnxCore/Filters/ErodeDilateCoordinationNumberFilter.hpp"
#include "SimplnxCore/Filters/ErodeDilateMaskFilter.hpp"
#include "SimplnxCore/Filters/ErodeDilateBadDataFilter.hpp"
#include "SimplnxCore/Filters/ExecuteProcessFilter.hpp"
#include "SimplnxCore/Filters/WriteDREAM3DFilter.hpp"
#include "SimplnxCore/Filters/ExtractComponentAsArrayFilter.hpp"
#include "SimplnxCore/Filters/ExtractInternalSurfacesFromTriangleGeometryFilter.hpp"
#include "SimplnxCore/Filters/WriteFeatureDataCSVFilter.hpp"
#include "SimplnxCore/Filters/FillBadDataFilter.hpp"
#include "SimplnxCore/Filters/FindArrayStatisticsFilter.hpp"
#include "SimplnxCore/Filters/FindBoundaryCellsFilter.hpp"
#include "SimplnxCore/Filters/FindBiasedFeaturesFilter.hpp"
#include "SimplnxCore/Filters/FindDifferencesMapFilter.hpp"
#include "SimplnxCore/Filters/FindEuclideanDistMapFilter.hpp"
#include "SimplnxCore/Filters/FindFeatureCentroidsFilter.hpp"
#include "SimplnxCore/Filters/FindFeaturePhasesFilter.hpp"
#include "SimplnxCore/Filters/FindFeaturePhasesBinaryFilter.hpp"
#include "SimplnxCore/Filters/FindNeighborhoodsFilter.hpp"
#include "SimplnxCore/Filters/FindNeighborListStatisticsFilter.hpp"
#include "SimplnxCore/Filters/FindFeatureNeighborsFilter.hpp"
#include "SimplnxCore/Filters/FindNumFeaturesFilter.hpp"
#include "SimplnxCore/Filters/FindSurfaceAreaToVolumeFilter.hpp"
#include "SimplnxCore/Filters/FindSurfaceFeaturesFilter.hpp"
#include "SimplnxCore/Filters/FindVolFractionsFilter.hpp"
#include "SimplnxCore/Filters/GenerateColorTableFilter.hpp"
#include "SimplnxCore/Filters/IdentifySampleFilter.hpp"
#include "SimplnxCore/Filters/ReadBinaryCTNorthstarFilter.hpp"
#include "SimplnxCore/Filters/ReadCSVFileFilter.hpp"
#include "SimplnxCore/Filters/ReadDeformKeyFileV12Filter.hpp"
#include "SimplnxCore/Filters/ReadDREAM3DFilter.hpp"
#include "SimplnxCore/Filters/ReadHDF5DatasetFilter.hpp"
#include "SimplnxCore/Filters/ReadTextDataArrayFilter.hpp"
#include "SimplnxCore/Filters/ReadVolumeGraphicsFileFilter.hpp"
#include "SimplnxCore/Filters/InitializeImageGeomCellDataFilter.hpp"
#include "SimplnxCore/Filters/InterpolatePointCloudToRegularGridFilter.hpp"
#include "SimplnxCore/Filters/IterativeClosestPointFilter.hpp"
#include "SimplnxCore/Filters/LaplacianSmoothingFilter.hpp"
#include "SimplnxCore/Filters/MapPointCloudToRegularGridFilter.hpp"
#include "SimplnxCore/Filters/MinNeighborsFilter.hpp"
#include "SimplnxCore/Filters/MoveDataFilter.hpp"
#include "SimplnxCore/Filters/MultiThresholdObjectsFilter.hpp"
#include "SimplnxCore/Filters/PointSampleTriangleGeometryFilter.hpp"
#include "SimplnxCore/Filters/QuickSurfaceMeshFilter.hpp"
#include "SimplnxCore/Filters/ReadRawBinaryFilter.hpp"
#include "SimplnxCore/Filters/RemoveFlaggedVerticesFilter.hpp"
#include "SimplnxCore/Filters/RemoveMinimumSizeFeaturesFilter.hpp"
#include "SimplnxCore/Filters/RenameDataObjectFilter.hpp"
#include "SimplnxCore/Filters/ReplaceElementAttributesWithNeighborValuesFilter.hpp"
#include "SimplnxCore/Filters/ResampleImageGeomFilter.hpp"
#include "SimplnxCore/Filters/RobustAutomaticThresholdFilter.hpp"
#include "SimplnxCore/Filters/RotateSampleRefFrameFilter.hpp"
#include "SimplnxCore/Filters/ScalarSegmentFeaturesFilter.hpp"
#include "SimplnxCore/Filters/SetImageGeomOriginScalingFilter.hpp"
#include "SimplnxCore/Filters/SplitAttributeArrayFilter.hpp"
#include "SimplnxCore/Filters/ReadStlFileFilter.hpp"
#include "SimplnxCore/Filters/TriangleCentroidFilter.hpp"
#include "SimplnxCore/Filters/TriangleDihedralAngleFilter.hpp"
#include "SimplnxCore/Filters/TriangleNormalFilter.hpp"
#include "SimplnxCore/Filters/WriteASCIIDataFilter.hpp"
#include "SimplnxCore/Filters/RemoveFlaggedFeaturesFilter.hpp"
#include "SimplnxCore/Filters/ComputeMomentInvariants2DFilter.hpp"
#include "SimplnxCore/Filters/ExecuteProcessFilter.hpp"
#include "SimplnxCore/Filters/FindVertexToTriangleDistancesFilter.hpp"
#include "SimplnxCore/Filters/ApplyTransformationToGeometryFilter.hpp"
#include "SimplnxCore/Filters/CalculateFeatureBoundingBoxesFilter.hpp"
#include "SimplnxCore/Filters/FindLargestCrossSectionsFilter.hpp"
#include "SimplnxCore/Filters/WriteStlFileFilter.hpp"
#include "SimplnxCore/Filters/AddBadDataFilter.hpp"
#include "SimplnxCore/Filters/AppendImageGeometryZSliceFilter.hpp"
#include "SimplnxCore/Filters/FindFeatureClusteringFilter.hpp"
#include "SimplnxCore/Filters/WriteAbaqusHexahedronFilter.hpp"
#include "SimplnxCore/Filters/NearestPointFuseRegularGridsFilter.hpp"
#include "SimplnxCore/Filters/ResampleRectGridToImageGeomFilter.hpp"
#include "SimplnxCore/Filters/CombineStlFilesFilter.hpp"
#include "SimplnxCore/Filters/WriteAvizoUniformCoordinateFilter.hpp"
#include "SimplnxCore/Filters/WriteAvizoRectilinearCoordinateFilter.hpp"
#include "SimplnxCore/Filters/WriteVtkRectilinearGridFilter.hpp"
#include "SimplnxCore/Filters/WriteVtkStructuredPointsFilter.hpp"
#include "SimplnxCore/Filters/RegularGridSampleSurfaceMeshFilter.hpp"
#include "SimplnxCore/Filters/UncertainRegularGridSampleSurfaceMeshFilter.hpp"
#include "SimplnxCore/Filters/FindBoundaryElementFractionsFilter.hpp"
#include "SimplnxCore/Filters/ReverseTriangleWindingFilter.hpp"
#include "SimplnxCore/Filters/WriteLosAlamosFFTFilter.hpp"
#include "SimplnxCore/Filters/GenerateVectorColorsFilter.hpp"
#include "SimplnxCore/Filters/KMedoidsFilter.hpp"
#include "SimplnxCore/Filters/KMeansFilter.hpp"
#include "SimplnxCore/Filters/SilhouetteFilter.hpp"
#include "SimplnxCore/Filters/LabelTriangleGeometryFilter.hpp"
#include "SimplnxCore/Filters/RemoveFlaggedTrianglesFilter.hpp"
#include "SimplnxCore/Filters/ReadVtkStructuredPointsFilter.hpp"
// @@__HEADER__TOKEN__DO__NOT__DELETE__@@

namespace nx::core
{
  static const AbstractPlugin::SIMPLMapType k_SIMPL_to_SimplnxCore
  {
    // syntax std::make_pair {Dream3d UUID , Dream3dnx UUID, {}}}, // dream3d-class-name
    {nx::core::Uuid::FromString("886f8b46-51b6-5682-a289-6febd10b7ef0").value(), {nx::core::FilterTraits<AlignSectionsFeatureCentroidFilter>::uuid, &AlignSectionsFeatureCentroidFilter::FromSIMPLJson}}, // AlignSectionsFeatureCentroid
    {nx::core::Uuid::FromString("ce1ee404-0336-536c-8aad-f9641c9458be").value(), {nx::core::FilterTraits<AlignGeometriesFilter>::uuid, &AlignGeometriesFilter::FromSIMPLJson}}, // AlignGeometriesFilter
    {nx::core::Uuid::FromString("accf8f6c-0551-5da3-9a3d-e4be41c3985c").value(), {nx::core::FilterTraits<AlignSectionsListFilter>::uuid, &AlignSectionsListFilter::FromSIMPLJson}}, // AlignSectionsListFilter
    {nx::core::Uuid::FromString("7ff0ebb3-7b0d-5ff7-b9d8-5147031aca10").value(), {nx::core::FilterTraits<ArrayCalculatorFilter>::uuid, &ArrayCalculatorFilter::FromSIMPLJson}}, // ArrayCalculatorFilter
    {nx::core::Uuid::FromString("c681caf4-22f2-5885-bbc9-a0476abc72eb").value(), {nx::core::FilterTraits<ApplyTransformationToGeometryFilter>::uuid, &ApplyTransformationToGeometryFilter::FromSIMPLJson}}, // ApplyTransformationToGeometry
    {nx::core::Uuid::FromString("fab669ad-66c6-5a39-bdb7-fc47b94311ed").value(), {nx::core::FilterTraits<ApproximatePointCloudHullFilter>::uuid, &ApproximatePointCloudHullFilter::FromSIMPLJson}}, // ApproximatePointCloudHullFilter
    {nx::core::Uuid::FromString("289f0d8c-29ab-5fbc-91bd-08aac01e37c5").value(), {nx::core::FilterTraits<CalculateArrayHistogramFilter>::uuid, &CalculateArrayHistogramFilter::FromSIMPLJson}}, // CalculateArrayHistogram
    {nx::core::Uuid::FromString("656f144c-a120-5c3b-bee5-06deab438588").value(), {nx::core::FilterTraits<CalculateFeatureSizesFilter>::uuid, &CalculateFeatureSizesFilter::FromSIMPLJson}}, // FindSizes
    {nx::core::Uuid::FromString("a9900cc3-169e-5a1b-bcf4-7569e1950d41").value(), {nx::core::FilterTraits<CalculateTriangleAreasFilter>::uuid, &CalculateTriangleAreasFilter::FromSIMPLJson}}, // TriangleAreaFilter
    {nx::core::Uuid::FromString("f7bc0e1e-0f50-5fe0-a9e7-510b6ed83792").value(), {nx::core::FilterTraits<ChangeAngleRepresentationFilter>::uuid, &ChangeAngleRepresentationFilter::FromSIMPLJson}}, // ChangeAngleRepresentation
    {nx::core::Uuid::FromString("a6b50fb0-eb7c-5d9b-9691-825d6a4fe772").value(), {nx::core::FilterTraits<CombineAttributeArraysFilter>::uuid, &CombineAttributeArraysFilter::FromSIMPLJson}}, // CombineAttributeArrays
    //{nx::core::Uuid::FromString("47cafe63-83cc-5826-9521-4fb5bea684ef").value(), {nx::core::FilterTraits<ConditionalSetValueFilter>::uuid, &ConditionalSetValueFilter::FromSIMPLJson}}, // ConditionalSetValueFilter
    {nx::core::Uuid::FromString("eb5a89c4-4e71-59b1-9719-d10a652d961e").value(), {nx::core::FilterTraits<ConvertColorToGrayScaleFilter>::uuid, &ConvertColorToGrayScaleFilter::FromSIMPLJson}}, // ConvertColorToGrayScale
    {nx::core::Uuid::FromString("f4ba5fa4-bb5c-5dd1-9429-0dd86d0ecb37").value(), {nx::core::FilterTraits<ConvertDataFilter>::uuid, &ConvertDataFilter::FromSIMPLJson}}, // ConvertData
    {nx::core::Uuid::FromString("a37f2e24-7400-5005-b9a7-b2224570cbe9").value(), {nx::core::FilterTraits<ConditionalSetValueFilter>::uuid, &ConditionalSetValueFilter::FromSIMPLJson}}, // ReplaceValueInArray
    {nx::core::Uuid::FromString("99836b75-144b-5126-b261-b411133b5e8a").value(), {nx::core::FilterTraits<CopyFeatureArrayToElementArrayFilter>::uuid, &CopyFeatureArrayToElementArrayFilter::FromSIMPLJson}}, // CopyFeatureArrayToElementArrayFilter
    {nx::core::Uuid::FromString("93375ef0-7367-5372-addc-baa019b1b341").value(), {nx::core::FilterTraits<CreateAttributeMatrixFilter>::uuid, &CreateAttributeMatrixFilter::FromSIMPLJson}}, // CreateAttributeMatrix
    {nx::core::Uuid::FromString("77f392fb-c1eb-57da-a1b1-e7acf9239fb8").value(), {nx::core::FilterTraits<CreateDataArrayFilter>::uuid, &CreateDataArrayFilter::FromSIMPLJson}}, // CreateDataArrayFilter
    {nx::core::Uuid::FromString("816fbe6b-7c38-581b-b149-3f839fb65b93").value(), {nx::core::FilterTraits<CreateDataGroupFilter>::uuid, &CreateDataGroupFilter::FromSIMPLJson}}, // CreateDataContainer
    {nx::core::Uuid::FromString("94438019-21bb-5b61-a7c3-66974b9a34dc").value(), {nx::core::FilterTraits<CreateFeatureArrayFromElementArrayFilter>::uuid, &CreateFeatureArrayFromElementArrayFilter::FromSIMPLJson}}, // CreateFeatureArrayFromElementArrayFilter
    {nx::core::Uuid::FromString("f2132744-3abb-5d66-9cd9-c9a233b5c4aa").value(), {nx::core::FilterTraits<CreateImageGeometryFilter>::uuid, &CreateImageGeometryFilter::FromSIMPLJson}}, // CreateImageGeometryFilter
    {nx::core::Uuid::FromString("baa4b7fe-31e5-5e63-a2cb-0bb9d844cfaf").value(), {nx::core::FilterTraits<CropImageGeometryFilter>::uuid, &CropImageGeometryFilter::FromSIMPLJson}}, // CropImageGeometryFilter
    {nx::core::Uuid::FromString("f28cbf07-f15a-53ca-8c7f-b41a11dae6cc").value(), {nx::core::FilterTraits<CropVertexGeometryFilter>::uuid, &CropVertexGeometryFilter::FromSIMPLJson}}, // CropVertexGeometryFilter
    {nx::core::Uuid::FromString("7b1c8f46-90dd-584a-b3ba-34e16958a7d0").value(), {nx::core::FilterTraits<DeleteDataFilter>::uuid, &DeleteDataFilter::FromSIMPLJson}}, // RemoveArrays   
    {nx::core::Uuid::FromString("3fcd4c43-9d75-5b86-aad4-4441bc914f37").value(), {nx::core::FilterTraits<WriteDREAM3DFilter>::uuid, &WriteDREAM3DFilter::FromSIMPLJson}}, // DataContainerWriter
    {nx::core::Uuid::FromString("52a069b4-6a46-5810-b0ec-e0693c636034").value(), {nx::core::FilterTraits<ExtractInternalSurfacesFromTriangleGeometryFilter>::uuid, &ExtractInternalSurfacesFromTriangleGeometryFilter::FromSIMPLJson}}, // ExtractInternalSurfacesFromTriangleGeometryFilter
    {nx::core::Uuid::FromString("737b8d5a-8622-50f9-9a8a-bfdb57608891").value(), {nx::core::FilterTraits<WriteFeatureDataCSVFilter>::uuid, &WriteFeatureDataCSVFilter::FromSIMPLJson}}, // FeatureDataCSVWriter
    {nx::core::Uuid::FromString("bf35f515-294b-55ed-8c69-211b7e69cb56").value(), {nx::core::FilterTraits<FindArrayStatisticsFilter>::uuid, &FindArrayStatisticsFilter::FromSIMPLJson}}, // FindArrayStatistics
    {nx::core::Uuid::FromString("8a1106d4-c67f-5e09-a02a-b2e9b99d031e").value(), {nx::core::FilterTraits<FindBoundaryCellsFilter>::uuid, &FindBoundaryCellsFilter::FromSIMPLJson}}, // FindBoundaryCellsFilter
    {nx::core::Uuid::FromString("450c2f00-9ddf-56e1-b4c1-0e74e7ad2349").value(), {nx::core::FilterTraits<FindBiasedFeaturesFilter>::uuid, &FindBiasedFeaturesFilter::FromSIMPLJson}}, // FindBiasedFeaturesFilter
    {nx::core::Uuid::FromString("29086169-20ce-52dc-b13e-824694d759aa").value(), {nx::core::FilterTraits<FindDifferencesMapFilter>::uuid, &FindDifferencesMapFilter::FromSIMPLJson}}, // FindDifferenceMap
    {nx::core::Uuid::FromString("933e4b2d-dd61-51c3-98be-00548ba783a3").value(), {nx::core::FilterTraits<FindEuclideanDistMapFilter>::uuid, &FindEuclideanDistMapFilter::FromSIMPLJson}}, // FindEuclideanDistMap
    {nx::core::Uuid::FromString("6f8ca36f-2995-5bd3-8672-6b0b80d5b2ca").value(), {nx::core::FilterTraits<FindFeatureCentroidsFilter>::uuid, &FindFeatureCentroidsFilter::FromSIMPLJson}}, // FindFeatureCentroids
    {nx::core::Uuid::FromString("6334ce16-cea5-5643-83b5-9573805873fa").value(), {nx::core::FilterTraits<FindFeaturePhasesFilter>::uuid, &FindFeaturePhasesFilter::FromSIMPLJson}}, // FindFeaturePhases
    {nx::core::Uuid::FromString("64d20c7b-697c-5ff1-9d1d-8a27b071f363").value(), {nx::core::FilterTraits<FindFeaturePhasesBinaryFilter>::uuid, &FindFeaturePhasesBinaryFilter::FromSIMPLJson}}, // FindFeaturePhasesBinary
    {nx::core::Uuid::FromString("697ed3de-db33-5dd1-a64b-04fb71e7d63e").value(), {nx::core::FilterTraits<FindNeighborhoodsFilter>::uuid, &FindNeighborhoodsFilter::FromSIMPLJson}}, // FindNeighborhoods
    {nx::core::Uuid::FromString("73ee33b6-7622-5004-8b88-4d145514fb6a").value(), {nx::core::FilterTraits<FindNeighborListStatisticsFilter>::uuid, &FindNeighborListStatisticsFilter::FromSIMPLJson}}, // FindNeighborListStatisticsFilter
    {nx::core::Uuid::FromString("97cf66f8-7a9b-5ec2-83eb-f8c4c8a17bac").value(), {nx::core::FilterTraits<FindFeatureNeighborsFilter>::uuid, &FindFeatureNeighborsFilter::FromSIMPLJson}}, // FindNeighborsFilter
    {nx::core::Uuid::FromString("529743cf-d5d5-5d5a-a79f-95c84a5ddbb5").value(), {nx::core::FilterTraits<FindNumFeaturesFilter>::uuid, &FindNumFeaturesFilter::FromSIMPLJson}}, // FindNumFeatures
    {nx::core::Uuid::FromString("5d586366-6b59-566e-8de1-57aa9ae8a91c").value(), {nx::core::FilterTraits<FindSurfaceAreaToVolumeFilter>::uuid, &FindSurfaceAreaToVolumeFilter::FromSIMPLJson}}, // FindSurfaceAreaToVolume
    {nx::core::Uuid::FromString("d2b0ae3d-686a-5dc0-a844-66bc0dc8f3cb").value(), {nx::core::FilterTraits<FindSurfaceFeaturesFilter>::uuid, &FindSurfaceFeaturesFilter::FromSIMPLJson}}, // FindSurfaceFeaturesFilter
    {nx::core::Uuid::FromString("68246a67-7f32-5c80-815a-bec82008d7bc").value(), {nx::core::FilterTraits<FindVolFractionsFilter>::uuid, &FindVolFractionsFilter::FromSIMPLJson}}, // FindVolFractions
    {nx::core::Uuid::FromString("0d0a6535-6565-51c5-a3fc-fbc00008606d").value(), {nx::core::FilterTraits<GenerateColorTableFilter>::uuid, &GenerateColorTableFilter::FromSIMPLJson}}, // GenerateColorTable
    {nx::core::Uuid::FromString("0e8c0818-a3fb-57d4-a5c8-7cb8ae54a40a").value(), {nx::core::FilterTraits<IdentifySampleFilter>::uuid, &IdentifySampleFilter::FromSIMPLJson}}, // IdentifySampleFilter
    {nx::core::Uuid::FromString("f2259481-5011-5f22-9fcb-c92fb6f8be10").value(), {nx::core::FilterTraits<ReadBinaryCTNorthstarFilter>::uuid, &ReadBinaryCTNorthstarFilter::FromSIMPLJson}}, // ImportBinaryCTNorthstarFilter
    {nx::core::Uuid::FromString("bdb978bc-96bf-5498-972c-b509c38b8d50").value(), {nx::core::FilterTraits<ReadCSVFileFilter>::uuid, &ReadCSVFileFilter::FromSIMPLJson}}, // ReadASCIIData
    {nx::core::Uuid::FromString("043cbde5-3878-5718-958f-ae75714df0df").value(), {nx::core::FilterTraits<ReadDREAM3DFilter>::uuid, &ReadDREAM3DFilter::FromSIMPLJson}}, // DataContainerReader
    {nx::core::Uuid::FromString("9e98c3b0-5707-5a3b-b8b5-23ef83b02896").value(), {nx::core::FilterTraits<ReadHDF5DatasetFilter>::uuid, &ReadHDF5DatasetFilter::FromSIMPLJson}}, // ImportHDF5Dataset
    {nx::core::Uuid::FromString("a7007472-29e5-5d0a-89a6-1aed11b603f8").value(), {nx::core::FilterTraits<ReadTextDataArrayFilter>::uuid, &ReadTextDataArrayFilter::FromSIMPLJson}}, // ImportAsciDataArray
    {nx::core::Uuid::FromString("5fa10d81-94b4-582b-833f-8eabe659069e").value(), {nx::core::FilterTraits<ReadVolumeGraphicsFileFilter>::uuid, &ReadVolumeGraphicsFileFilter::FromSIMPLJson}}, // ImportVolumeGraphicsFileFilter
    {nx::core::Uuid::FromString("dfab9921-fea3-521c-99ba-48db98e43ff8").value(), {nx::core::FilterTraits<InitializeImageGeomCellDataFilter>::uuid, &InitializeImageGeomCellDataFilter::FromSIMPLJson}}, // InitializeDataFilter
    {nx::core::Uuid::FromString("4b551c15-418d-5081-be3f-d3aeb62408e5").value(), {nx::core::FilterTraits<InterpolatePointCloudToRegularGridFilter>::uuid, &InterpolatePointCloudToRegularGridFilter::FromSIMPLJson}}, // InterpolatePointCloudToRegularGrid
    {nx::core::Uuid::FromString("6c8fb24b-5b12-551c-ba6d-ae2fa7724764").value(), {nx::core::FilterTraits<IterativeClosestPointFilter>::uuid, &IterativeClosestPointFilter::FromSIMPLJson}}, // IterativeClosestPoint
    {nx::core::Uuid::FromString("601c4885-c218-5da6-9fc7-519d85d241ad").value(), {nx::core::FilterTraits<LaplacianSmoothingFilter>::uuid, &LaplacianSmoothingFilter::FromSIMPLJson}}, // LaplacianSmoothing
    {nx::core::Uuid::FromString("9fe34deb-99e1-5f3a-a9cc-e90c655b47ee").value(), {nx::core::FilterTraits<MapPointCloudToRegularGridFilter>::uuid, &MapPointCloudToRegularGridFilter::FromSIMPLJson}}, // MapPointCloudToRegularGrid
    {nx::core::Uuid::FromString("dab5de3c-5f81-5bb5-8490-73521e1183ea").value(), {nx::core::FilterTraits<MinNeighborsFilter>::uuid, &MinNeighborsFilter::FromSIMPLJson}}, // MinNeighborsFilter
    {nx::core::Uuid::FromString("fe2cbe09-8ae1-5bea-9397-fd5741091fdb").value(), {nx::core::FilterTraits<MoveDataFilter>::uuid, &MoveDataFilter::FromSIMPLJson}}, // MoveDataFilter
    {nx::core::Uuid::FromString("014b7300-cf36-5ede-a751-5faf9b119dae").value(), {nx::core::FilterTraits<MultiThresholdObjectsFilter>::uuid, &MultiThresholdObjectsFilter::FromSIMPLJson}}, // MultiThresholdObjects
    {nx::core::Uuid::FromString("686d5393-2b02-5c86-b887-dd81a8ae80f2").value(), {nx::core::FilterTraits<MultiThresholdObjectsFilter>::uuid, &MultiThresholdObjectsFilter::FromSIMPLJson}}, // MultiThresholdObjects2
    {nx::core::Uuid::FromString("119861c5-e303-537e-b210-2e62936222e9").value(), {nx::core::FilterTraits<PointSampleTriangleGeometryFilter>::uuid, &PointSampleTriangleGeometryFilter::FromSIMPLJson}}, // PointSampleTriangleGeometry
    {nx::core::Uuid::FromString("07b49e30-3900-5c34-862a-f1fb48bad568").value(), {nx::core::FilterTraits<QuickSurfaceMeshFilter>::uuid, &QuickSurfaceMeshFilter::FromSIMPLJson}}, // QuickSurfaceMesh
    {nx::core::Uuid::FromString("0791f556-3d73-5b1e-b275-db3f7bb6850d").value(), {nx::core::FilterTraits<ReadRawBinaryFilter>::uuid, &ReadRawBinaryFilter::FromSIMPLJson}}, // RawBinaryReader
    {nx::core::Uuid::FromString("379ccc67-16dd-530a-984f-177db2314bac").value(), {nx::core::FilterTraits<RemoveFlaggedVerticesFilter>::uuid, &RemoveFlaggedVerticesFilter::FromSIMPLJson}}, // RemoveFlaggedVerticesFilter
    {nx::core::Uuid::FromString("53ac1638-8934-57b8-b8e5-4b91cdda23ec").value(), {nx::core::FilterTraits<RemoveMinimumSizeFeaturesFilter>::uuid, &RemoveMinimumSizeFeaturesFilter::FromSIMPLJson}}, // MinSize
    {nx::core::Uuid::FromString("53a5f731-2858-5e3e-bd43-8f2cf45d90ec").value(), {nx::core::FilterTraits<RenameDataObjectFilter>::uuid, &RenameDataObjectFilter::FromSIMPLJson}}, // RenameAttributeArray
    {nx::core::Uuid::FromString("ee29e6d6-1f59-551b-9350-a696523261d5").value(), {nx::core::FilterTraits<RenameDataObjectFilter>::uuid, &RenameDataObjectFilter::FromSIMPLJson}}, // RenameAttributeMatrix
    {nx::core::Uuid::FromString("d53c808f-004d-5fac-b125-0fffc8cc78d6").value(), {nx::core::FilterTraits<RenameDataObjectFilter>::uuid, &RenameDataObjectFilter::FromSIMPLJson}}, // RenameDataContainer
    {nx::core::Uuid::FromString("3062fc2c-76b2-5c50-92b7-edbbb424c41d").value(), {nx::core::FilterTraits<RobustAutomaticThresholdFilter>::uuid, &RobustAutomaticThresholdFilter::FromSIMPLJson}}, // RobustAutomaticThresholdFilter
    {nx::core::Uuid::FromString("2c5edebf-95d8-511f-b787-90ee2adf485c").value(), {nx::core::FilterTraits<ScalarSegmentFeaturesFilter>::uuid, &ScalarSegmentFeaturesFilter::FromSIMPLJson}}, // ScalarSegmentFeatures
    {nx::core::Uuid::FromString("6d3a3852-6251-5d2e-b749-6257fd0d8951").value(), {nx::core::FilterTraits<SetImageGeomOriginScalingFilter>::uuid, &SetImageGeomOriginScalingFilter::FromSIMPLJson}}, // SetOriginResolutionImageGeom
    {nx::core::Uuid::FromString("5ecf77f4-a38a-52ab-b4f6-0fb8a9c5cb9c").value(), {nx::core::FilterTraits<SplitAttributeArrayFilter>::uuid, &SplitAttributeArrayFilter::FromSIMPLJson}}, // SplitAttributeArray
    {nx::core::Uuid::FromString("980c7bfd-20b2-5711-bc3b-0190b9096c34").value(), {nx::core::FilterTraits<ReadStlFileFilter>::uuid, &ReadStlFileFilter::FromSIMPLJson}}, // ReadStlFile
    {nx::core::Uuid::FromString("5fbf9204-2c6c-597b-856a-f4612adbac38").value(), {nx::core::FilterTraits<WriteASCIIDataFilter>::uuid, &WriteASCIIDataFilter::FromSIMPLJson}}, // WriteASCIIData
    {nx::core::Uuid::FromString("0541c5eb-1976-5797-9468-be50a93d44e2").value(), {nx::core::FilterTraits<TriangleDihedralAngleFilter>::uuid, &TriangleDihedralAngleFilter::FromSIMPLJson}}, // TriangleDihedralAngleFilter
    {nx::core::Uuid::FromString("928154f6-e4bc-5a10-a9dd-1abb6a6c0f6b").value(), {nx::core::FilterTraits<TriangleNormalFilter>::uuid, &TriangleNormalFilter::FromSIMPLJson}}, // TriangleNormalFilter
    {nx::core::Uuid::FromString("088ef69b-ca98-51a9-97ac-369862015d71").value(), {nx::core::FilterTraits<CopyDataObjectFilter>::uuid, &CopyDataObjectFilter::FromSIMPLJson}}, // CopyObject
    {nx::core::Uuid::FromString("79d59b85-01e8-5c4a-a6e1-3fd3e2ceffb4").value(), {nx::core::FilterTraits<ExtractComponentAsArrayFilter>::uuid, &ExtractComponentAsArrayFilter::FromSIMPLJson}}, // ExtractComponentAsArray
    {nx::core::Uuid::FromString("8a2308ec-86cd-5636-9a0a-6c7d383e9e7f").value(), {nx::core::FilterTraits<ExecuteProcessFilter>::uuid, &ExecuteProcessFilter::FromSIMPLJson}}, // ExecuteProcessFilter
    {nx::core::Uuid::FromString("1b4b9941-62e4-52f2-9918-15d48147ab88").value(), {nx::core::FilterTraits<ExtractComponentAsArrayFilter>::uuid, &ExtractComponentAsArrayFilter::FromSIMPLJson}}, // RemoveComponentFromArray
    {nx::core::Uuid::FromString("7aa33007-4186-5d7f-ba9d-d0a561b3351d").value(), {nx::core::FilterTraits<TriangleCentroidFilter>::uuid, &TriangleCentroidFilter::FromSIMPLJson}}, // TriangleCentroid
    {nx::core::Uuid::FromString("1966e540-759c-5798-ae26-0c6a3abc65c0").value(), {nx::core::FilterTraits<ResampleImageGeomFilter>::uuid, &ResampleImageGeomFilter::FromSIMPLJson}}, // ResampleImageGeom
    {nx::core::Uuid::FromString("e25d9b4c-2b37-578c-b1de-cf7032b5ef19").value(), {nx::core::FilterTraits<RotateSampleRefFrameFilter>::uuid, &RotateSampleRefFrameFilter::FromSIMPLJson}}, // RotateSampleRefFrame
    {nx::core::Uuid::FromString("30ae0a1e-3d94-5dab-b279-c5727ab5d7ff").value(), {nx::core::FilterTraits<FillBadDataFilter>::uuid, &FillBadDataFilter::FromSIMPLJson}}, // FillBadData
    {nx::core::Uuid::FromString("7aa33007-4186-5d7f-ba9d-d0a561b3351d").value(), {nx::core::FilterTraits<TriangleCentroidFilter>::uuid, {}}}, // TriangleCentroid  
    {nx::core::Uuid::FromString("17410178-4e5f-58b9-900e-8194c69200ab").value(), {nx::core::FilterTraits<ReplaceElementAttributesWithNeighborValuesFilter>::uuid, &ReplaceElementAttributesWithNeighborValuesFilter::FromSIMPLJson}}, // ReplaceElementAttributesWithNeighborValues
    {nx::core::Uuid::FromString("4fff1aa6-4f62-56c4-8ee9-8e28ec2fcbba").value(), {nx::core::FilterTraits<ErodeDilateMaskFilter>::uuid, &ErodeDilateMaskFilter::FromSIMPLJson}}, // ErodeDilateMask
    {nx::core::Uuid::FromString("d26e85ff-7e52-53ae-b095-b1d969c9e73c").value(), {nx::core::FilterTraits<ErodeDilateCoordinationNumberFilter>::uuid, &ErodeDilateCoordinationNumberFilter::FromSIMPLJson}}, // ErodeDilateCoordinationNumber
    {nx::core::Uuid::FromString("3adfe077-c3c9-5cd0-ad74-cf5f8ff3d254").value(), {nx::core::FilterTraits<ErodeDilateBadDataFilter>::uuid, &ErodeDilateBadDataFilter::FromSIMPLJson}}, // ErodeDilateBadData
    {nx::core::Uuid::FromString("a8463056-3fa7-530b-847f-7f4cb78b8602").value(), {nx::core::FilterTraits<RemoveFlaggedFeaturesFilter>::uuid, &RemoveFlaggedFeaturesFilter::FromSIMPLJson}}, // RemoveFlaggedFeatures
    {nx::core::Uuid::FromString("e0555de5-bdc6-5bea-ba2f-aacfbec0a022").value(), {nx::core::FilterTraits<RemoveFlaggedFeaturesFilter>::uuid, &RemoveFlaggedFeaturesFilter::FromSIMPLJson}}, // ExtractFlaggedFeatures
    {nx::core::Uuid::FromString("27a132b2-a592-519a-8cb7-38599a7f28ec").value(), {nx::core::FilterTraits<ComputeMomentInvariants2DFilter>::uuid, &ComputeMomentInvariants2DFilter::FromSIMPLJson}}, // ComputeMomentInvariants2D
    {nx::core::Uuid::FromString("fcdde553-36b4-5731-bc88-fc499806cb4e").value(), {nx::core::FilterTraits<FindVertexToTriangleDistancesFilter>::uuid, &FindVertexToTriangleDistancesFilter::FromSIMPLJson}}, // FindVertexToTriangleDistances
    {nx::core::Uuid::FromString("c681caf4-22f2-5885-bbc9-a0476abc72eb").value(), {nx::core::FilterTraits<ApplyTransformationToGeometryFilter>::uuid, {}}}, // ApplyTransformationToGeometry
    {nx::core::Uuid::FromString("6eda8dbf-dbd8-562a-ae1a-f2904157c189").value(), {nx::core::FilterTraits<CalculateFeatureBoundingBoxesFilter>::uuid, &CalculateFeatureBoundingBoxesFilter::FromSIMPLJson}}, // ComputeFeatureRect
    {nx::core::Uuid::FromString("9f77b4a9-6416-5220-a688-115f4e14c90d").value(), {nx::core::FilterTraits<FindLargestCrossSectionsFilter>::uuid, &FindLargestCrossSectionsFilter::FromSIMPLJson}}, // FindLargestCrossSections
    {nx::core::Uuid::FromString("b9134758-d5e5-59dd-9907-28d23e0e0143").value(), {nx::core::FilterTraits<WriteStlFileFilter>::uuid, &WriteStlFileFilter::FromSIMPLJson}}, // WriteStlFile
    {nx::core::Uuid::FromString("ac99b706-d1e0-5f78-9246-fbbe1efd93d2").value(), {nx::core::FilterTraits<AddBadDataFilter>::uuid, &AddBadDataFilter::FromSIMPLJson}}, // AddBadData
    {nx::core::Uuid::FromString("52b2918a-4fb5-57aa-97d4-ccc084b89572").value(), {nx::core::FilterTraits<AppendImageGeometryZSliceFilter>::uuid, &AppendImageGeometryZSliceFilter::FromSIMPLJson}}, // AppendImageGeometryZSlice
    {nx::core::Uuid::FromString("a1e9cf6d-2d1b-573e-98b8-0314c993d2b6").value(), {nx::core::FilterTraits<FindFeatureClusteringFilter>::uuid, &FindFeatureClusteringFilter::FromSIMPLJson}}, // FindFeatureClustering
    {nx::core::Uuid::FromString("0559aa37-c5ad-549a-82d4-bff4bfcb6cc6").value(), {nx::core::FilterTraits<WriteAbaqusHexahedronFilter>::uuid, &WriteAbaqusHexahedronFilter::FromSIMPLJson}}, // AbaqusHexahedronWriter
    {nx::core::Uuid::FromString("cbaf9e68-5ded-560c-9440-509289100ea8").value(), {nx::core::FilterTraits<NearestPointFuseRegularGridsFilter>::uuid, &NearestPointFuseRegularGridsFilter::FromSIMPLJson}}, // NearestPointFuseRegularGrids
    {nx::core::Uuid::FromString("77befd69-4536-5856-9f81-02996d038f73").value(), {nx::core::FilterTraits<ResampleRectGridToImageGeomFilter>::uuid, &ResampleRectGridToImageGeomFilter::FromSIMPLJson}}, // ResampleRectGridToImageGeom
    {nx::core::Uuid::FromString("71d46128-1d2d-58fd-9924-1714695768c3").value(), {nx::core::FilterTraits<CombineStlFilesFilter>::uuid, &CombineStlFilesFilter::FromSIMPLJson}}, // CombineStlFiles
    {nx::core::Uuid::FromString("339f1349-9236-5023-9a56-c82fb8eafd12").value(), {nx::core::FilterTraits<WriteAvizoUniformCoordinateFilter>::uuid, &WriteAvizoUniformCoordinateFilter::FromSIMPLJson}}, // AvizoUniformCoordinateWriter
    {nx::core::Uuid::FromString("2861f4b4-8d50-5e69-9575-68c9d35f1256").value(), {nx::core::FilterTraits<WriteAvizoRectilinearCoordinateFilter>::uuid, &WriteAvizoRectilinearCoordinateFilter::FromSIMPLJson}}, // AvizoRectilinearCoordinateWriter
    {nx::core::Uuid::FromString("a043bd66-2681-5126-82e1-5fdc46694bf4").value(), {nx::core::FilterTraits<WriteVtkRectilinearGridFilter>::uuid, &WriteVtkRectilinearGridFilter::FromSIMPLJson}}, // VtkRectilinearGridWriter
    {nx::core::Uuid::FromString("0df3da89-9106-538e-b1a9-6bbf1cf0aa92").value(), {nx::core::FilterTraits<RegularGridSampleSurfaceMeshFilter>::uuid, &RegularGridSampleSurfaceMeshFilter::FromSIMPLJson}}, // RegularGridSampleSurfaceMesh
    {nx::core::Uuid::FromString("75cfeb9b-cd4b-5a20-a344-4170b39bbfaf").value(), {nx::core::FilterTraits<UncertainRegularGridSampleSurfaceMeshFilter>::uuid, &UncertainRegularGridSampleSurfaceMeshFilter::FromSIMPLJson}}, // UncertainRegularGridSampleSurfaceMesh
    {nx::core::Uuid::FromString("6357243e-41a6-52c4-be2d-2f6894c39fcc").value(), {nx::core::FilterTraits<FindBoundaryElementFractionsFilter>::uuid, &FindBoundaryElementFractionsFilter::FromSIMPLJson}}, // FindBoundaryElementFractions
    {nx::core::Uuid::FromString("9b9fb9e1-074d-54b6-a6ce-0be21ab4496d").value(), {nx::core::FilterTraits<ReverseTriangleWindingFilter>::uuid, &ReverseTriangleWindingFilter::FromSIMPLJson}}, // ReverseTriangleWinding
    {nx::core::Uuid::FromString("158ebe9e-f772-57e2-ac1b-71ff213cf890").value(), {nx::core::FilterTraits<WriteLosAlamosFFTFilter>::uuid, &WriteLosAlamosFFTFilter::FromSIMPLJson}}, // LosAlamosFFTWriter
    {nx::core::Uuid::FromString("ef28de7e-5bdd-57c2-9318-60ba0dfaf7bc").value(), {nx::core::FilterTraits<GenerateVectorColorsFilter>::uuid, &GenerateVectorColorsFilter::FromSIMPLJson}}, // GenerateVectorColors
    {nx::core::Uuid::FromString("3c6337da-e232-4420-a5ca-451496748d88").value(), {nx::core::FilterTraits<ReadDeformKeyFileV12Filter>::uuid, &ReadDeformKeyFileV12Filter::FromSIMPLJson}}, // ImportDeformKeyFileV12Filter
    {nx::core::Uuid::FromString("f7486aa6-3049-5be7-8511-ae772b70c90b").value(), {nx::core::FilterTraits<KMedoidsFilter>::uuid, &KMedoidsFilter::FromSIMPLJson}}, // KMedoids
    {nx::core::Uuid::FromString("b56a04de-0ca0-509d-809f-52219fca9c98").value(), {nx::core::FilterTraits<KMeansFilter>::uuid, &KMeansFilter::FromSIMPLJson}}, // KMeans
    {nx::core::Uuid::FromString("f84d4d69-9ea5-54b6-a71c-df76d76d50cf").value(), {nx::core::FilterTraits<SilhouetteFilter>::uuid, &SilhouetteFilter::FromSIMPLJson}}, // Silhouette
    {nx::core::Uuid::FromString("a250a228-3b6b-5b37-a6e4-8687484f04c4").value(), {nx::core::FilterTraits<LabelTriangleGeometryFilter>::uuid, &LabelTriangleGeometryFilter::FromSIMPLJson}}, // LabelTriangleGeometry
    {nx::core::Uuid::FromString("379ccc67-16dd-530a-984f-177db9351bac").value(), {nx::core::FilterTraits<RemoveFlaggedTrianglesFilter>::uuid, &RemoveFlaggedTrianglesFilter::FromSIMPLJson}}, // RemoveFlaggedTriangles
    {nx::core::Uuid::FromString("f2fecbf9-636b-5ef9-89db-5cb57e4c5676").value(), {nx::core::FilterTraits<ReadVtkStructuredPointsFilter>::uuid, &ReadVtkStructuredPointsFilter::FromSIMPLJson}}, // ReadVtkStructuredPoints
    {nx::core::Uuid::FromString("a043bd66-2681-5126-82e1-5fdc46694bf4").value(), {nx::core::FilterTraits<WriteVtkStructuredPointsFilter>::uuid, &WriteVtkStructuredPointsFilter::FromSIMPLJson}}, // WriteVtkStructuredPointsFilter

    // @@__MAP__UPDATE__TOKEN__DO__NOT__DELETE__@@
  };

} // namespace nx::core
// clang-format on
