#pragma once

#include "complex/Plugin/AbstractPlugin.hpp"

#include <nlohmann/json.hpp>

#include <map>
#include <string>

// clang-format off
#include "ComplexCore/Filters/WriteAbaqusHexahedronFilter.hpp"
#include "ComplexCore/Filters/AddBadDataFilter.hpp"
#include "ComplexCore/Filters/AlignGeometries.hpp"
#include "ComplexCore/Filters/AlignSectionsFeatureCentroidFilter.hpp"
#include "ComplexCore/Filters/AlignSectionsListFilter.hpp"
#include "ComplexCore/Filters/AppendImageGeometryZSliceFilter.hpp"
#include "ComplexCore/Filters/ApplyTransformationToGeometryFilter.hpp"
#include "ComplexCore/Filters/ApplyTransformationToGeometryFilter.hpp"
#include "ComplexCore/Filters/ApproximatePointCloudHull.hpp"
#include "ComplexCore/Filters/ArrayCalculatorFilter.hpp"
#include "ComplexCore/Filters/WriteAvizoRectilinearCoordinateFilter.hpp"
#include "ComplexCore/Filters/WriteAvizoUniformCoordinateFilter.hpp"
#include "ComplexCore/Filters/CalculateArrayHistogramFilter.hpp"
#include "ComplexCore/Filters/CalculateFeatureSizesFilter.hpp"
#include "ComplexCore/Filters/CalculateTriangleAreasFilter.hpp"
#include "ComplexCore/Filters/ChangeAngleRepresentation.hpp"
#include "ComplexCore/Filters/CombineAttributeArraysFilter.hpp"
#include "ComplexCore/Filters/CombineStlFilesFilter.hpp"
#include "ComplexCore/Filters/ComputeFeatureRectFilter.hpp"
#include "ComplexCore/Filters/ComputeMomentInvariants2DFilter.hpp"
#include "ComplexCore/Filters/ConditionalSetValue.hpp"
#include "ComplexCore/Filters/ConvertColorToGrayScaleFilter.hpp"
#include "ComplexCore/Filters/ConvertDataFilter.hpp"
#include "ComplexCore/Filters/CopyDataObjectFilter.hpp"
#include "ComplexCore/Filters/CopyFeatureArrayToElementArray.hpp"
#include "ComplexCore/Filters/CreateAttributeMatrixFilter.hpp"
#include "ComplexCore/Filters/CreateDataArray.hpp"
#include "ComplexCore/Filters/CreateDataGroup.hpp"
#include "ComplexCore/Filters/CreateFeatureArrayFromElementArray.hpp"
#include "ComplexCore/Filters/CreateImageGeometry.hpp"
#include "ComplexCore/Filters/CropImageGeometry.hpp"
#include "ComplexCore/Filters/CropVertexGeometry.hpp"
#include "ComplexCore/Filters/DeleteData.hpp"
#include "ComplexCore/Filters/ErodeDilateBadDataFilter.hpp"
#include "ComplexCore/Filters/ErodeDilateCoordinationNumberFilter.hpp"
#include "ComplexCore/Filters/ErodeDilateMaskFilter.hpp"
#include "ComplexCore/Filters/ExecuteProcessFilter.hpp"
#include "ComplexCore/Filters/ExecuteProcessFilter.hpp"
#include "ComplexCore/Filters/WriteDREAM3DFilter.hpp"
#include "ComplexCore/Filters/ExtractComponentAsArrayFilter.hpp"
#include "ComplexCore/Filters/ExtractInternalSurfacesFromTriangleGeometry.hpp"
#include "ComplexCore/Filters/WriteFeatureDataCSVFilter.hpp"
#include "ComplexCore/Filters/FillBadDataFilter.hpp"
#include "ComplexCore/Filters/FindArrayStatisticsFilter.hpp"
#include "ComplexCore/Filters/FindBiasedFeaturesFilter.hpp"
#include "ComplexCore/Filters/FindBoundaryCellsFilter.hpp"
#include "ComplexCore/Filters/FindBoundaryElementFractionsFilter.hpp"
#include "ComplexCore/Filters/FindDifferencesMap.hpp"
#include "ComplexCore/Filters/FindEuclideanDistMapFilter.hpp"
#include "ComplexCore/Filters/FindFeatureCentroidsFilter.hpp"
#include "ComplexCore/Filters/FindFeatureClusteringFilter.hpp"
#include "ComplexCore/Filters/FindFeaturePhasesBinaryFilter.hpp"
#include "ComplexCore/Filters/FindFeaturePhasesFilter.hpp"
#include "ComplexCore/Filters/FindLargestCrossSectionsFilter.hpp"
#include "ComplexCore/Filters/FindNeighborhoodsFilter.hpp"
#include "ComplexCore/Filters/FindNeighborListStatistics.hpp"
#include "ComplexCore/Filters/FindNeighbors.hpp"
#include "ComplexCore/Filters/FindNumFeaturesFilter.hpp"
#include "ComplexCore/Filters/FindSurfaceAreaToVolumeFilter.hpp"
#include "ComplexCore/Filters/FindSurfaceFeatures.hpp"
#include "ComplexCore/Filters/FindVertexToTriangleDistancesFilter.hpp"
#include "ComplexCore/Filters/FindVolFractionsFilter.hpp"
#include "ComplexCore/Filters/GenerateColorTableFilter.hpp"
#include "ComplexCore/Filters/GenerateVectorColorsFilter.hpp"
#include "ComplexCore/Filters/IdentifySample.hpp"
#include "ComplexCore/Filters/ReadBinaryCTNorthstarFilter.hpp"
#include "ComplexCore/Filters/ReadCSVFileFilter.hpp"
#include "ComplexCore/Filters/ReadDeformKeyFileV12Filter.hpp"
#include "ComplexCore/Filters/ReadDREAM3DFilter.hpp"
#include "ComplexCore/Filters/ReadHDF5Dataset.hpp"
#include "ComplexCore/Filters/ReadTextDataArrayFilter.hpp"
#include "ComplexCore/Filters/ReadVolumeGraphicsFileFilter.hpp"
#include "ComplexCore/Filters/InitializeImageGeomCellData.hpp"
#include "ComplexCore/Filters/InterpolatePointCloudToRegularGridFilter.hpp"
#include "ComplexCore/Filters/IterativeClosestPointFilter.hpp"
#include "ComplexCore/Filters/KMeansFilter.hpp"
#include "ComplexCore/Filters/KMedoidsFilter.hpp"
#include "ComplexCore/Filters/LaplacianSmoothingFilter.hpp"
#include "ComplexCore/Filters/WriteLosAlamosFFTFilter.hpp"
#include "ComplexCore/Filters/MapPointCloudToRegularGridFilter.hpp"
#include "ComplexCore/Filters/MinNeighbors.hpp"
#include "ComplexCore/Filters/MoveData.hpp"
#include "ComplexCore/Filters/MultiThresholdObjects.hpp"
#include "ComplexCore/Filters/NearestPointFuseRegularGridsFilter.hpp"
#include "ComplexCore/Filters/PointSampleTriangleGeometryFilter.hpp"
#include "ComplexCore/Filters/QuickSurfaceMeshFilter.hpp"
#include "ComplexCore/Filters/ReadRawBinaryFilter.hpp"
#include "ComplexCore/Filters/RemoveFlaggedTrianglesFilter.hpp"
#include "ComplexCore/Filters/RemoveFlaggedVertices.hpp"
#include "ComplexCore/Filters/RemoveMinimumSizeFeaturesFilter.hpp"
#include "ComplexCore/Filters/RenameDataObject.hpp"
#include "ComplexCore/Filters/ReplaceElementAttributesWithNeighborValuesFilter.hpp"
#include "ComplexCore/Filters/ResampleImageGeomFilter.hpp"
#include "ComplexCore/Filters/ResampleRectGridToImageGeomFilter.hpp"
#include "ComplexCore/Filters/ReverseTriangleWindingFilter.hpp"
#include "ComplexCore/Filters/RobustAutomaticThreshold.hpp"
#include "ComplexCore/Filters/RotateSampleRefFrameFilter.hpp"
#include "ComplexCore/Filters/ScalarSegmentFeaturesFilter.hpp"
#include "ComplexCore/Filters/SetImageGeomOriginScalingFilter.hpp"
#include "ComplexCore/Filters/SilhouetteFilter.hpp"
#include "ComplexCore/Filters/SplitAttributeArrayFilter.hpp"
#include "ComplexCore/Filters/ReadStlFileFilter.hpp"
#include "ComplexCore/Filters/TriangleCentroidFilter.hpp"
#include "ComplexCore/Filters/TriangleDihedralAngleFilter.hpp"
#include "ComplexCore/Filters/TriangleNormalFilter.hpp"
#include "ComplexCore/Filters/UncertainRegularGridSampleSurfaceMeshFilter.hpp"
#include "ComplexCore/Filters/WriteVtkRectilinearGridFilter.hpp"
#include "ComplexCore/Filters/WriteASCIIDataFilter.hpp"
#include "ComplexCore/Filters/WriteStlFileFilter.hpp"
#include "ComplexCore/Filters/AddBadDataFilter.hpp"
#include "ComplexCore/Filters/AppendImageGeometryZSliceFilter.hpp"
#include "ComplexCore/Filters/FindFeatureClusteringFilter.hpp"
#include "ComplexCore/Filters/WriteAbaqusHexahedronFilter.hpp"
#include "ComplexCore/Filters/NearestPointFuseRegularGridsFilter.hpp"
#include "ComplexCore/Filters/ResampleRectGridToImageGeomFilter.hpp"
#include "ComplexCore/Filters/CombineStlFilesFilter.hpp"
#include "ComplexCore/Filters/WriteAvizoUniformCoordinateFilter.hpp"
#include "ComplexCore/Filters/WriteAvizoRectilinearCoordinateFilter.hpp"
#include "ComplexCore/Filters/WriteVtkRectilinearGridFilter.hpp"
#include "ComplexCore/Filters/RegularGridSampleSurfaceMeshFilter.hpp"
#include "ComplexCore/Filters/UncertainRegularGridSampleSurfaceMeshFilter.hpp"
#include "ComplexCore/Filters/FindBoundaryElementFractionsFilter.hpp"
#include "ComplexCore/Filters/ReverseTriangleWindingFilter.hpp"
#include "ComplexCore/Filters/WriteLosAlamosFFTFilter.hpp"
#include "ComplexCore/Filters/GenerateVectorColorsFilter.hpp"
#include "ComplexCore/Filters/KMedoidsFilter.hpp"
#include "ComplexCore/Filters/KMeansFilter.hpp"
#include "ComplexCore/Filters/SilhouetteFilter.hpp"
#include "ComplexCore/Filters/LabelTriangleGeometryFilter.hpp"
#include "ComplexCore/Filters/RemoveFlaggedFeaturesFilter.hpp"
// @@__HEADER__TOKEN__DO__NOT__DELETE__@@

namespace complex
{
  static const AbstractPlugin::SIMPLMapType k_SIMPL_to_ComplexCore
  {
    // syntax std::make_pair {Dream3d UUID , Dream3dnx UUID, {}}}, // dream3d-class-name
    {complex::Uuid::FromString("886f8b46-51b6-5682-a289-6febd10b7ef0").value(), {complex::FilterTraits<AlignSectionsFeatureCentroidFilter>::uuid, &AlignSectionsFeatureCentroidFilter::FromSIMPLJson}}, // AlignSectionsFeatureCentroid
    {complex::Uuid::FromString("ce1ee404-0336-536c-8aad-f9641c9458be").value(), {complex::FilterTraits<AlignGeometries>::uuid, &AlignGeometries::FromSIMPLJson}}, // AlignGeometries
    {complex::Uuid::FromString("accf8f6c-0551-5da3-9a3d-e4be41c3985c").value(), {complex::FilterTraits<AlignSectionsListFilter>::uuid, &AlignSectionsListFilter::FromSIMPLJson}}, // AlignSectionsListFilter
    {complex::Uuid::FromString("7ff0ebb3-7b0d-5ff7-b9d8-5147031aca10").value(), {complex::FilterTraits<ArrayCalculatorFilter>::uuid, &ArrayCalculatorFilter::FromSIMPLJson}}, // ArrayCalculatorFilter
    {complex::Uuid::FromString("c681caf4-22f2-5885-bbc9-a0476abc72eb").value(), {complex::FilterTraits<ApplyTransformationToGeometryFilter>::uuid, &ApplyTransformationToGeometryFilter::FromSIMPLJson}}, // ApplyTransformationToGeometry
    {complex::Uuid::FromString("fab669ad-66c6-5a39-bdb7-fc47b94311ed").value(), {complex::FilterTraits<ApproximatePointCloudHull>::uuid, &ApproximatePointCloudHull::FromSIMPLJson}}, // ApproximatePointCloudHull
    {complex::Uuid::FromString("289f0d8c-29ab-5fbc-91bd-08aac01e37c5").value(), {complex::FilterTraits<CalculateArrayHistogramFilter>::uuid, &CalculateArrayHistogramFilter::FromSIMPLJson}}, // CalculateArrayHistogram
    {complex::Uuid::FromString("656f144c-a120-5c3b-bee5-06deab438588").value(), {complex::FilterTraits<CalculateFeatureSizesFilter>::uuid, &CalculateFeatureSizesFilter::FromSIMPLJson}}, // FindSizes
    {complex::Uuid::FromString("a9900cc3-169e-5a1b-bcf4-7569e1950d41").value(), {complex::FilterTraits<CalculateTriangleAreasFilter>::uuid, &CalculateTriangleAreasFilter::FromSIMPLJson}}, // TriangleAreaFilter
    {complex::Uuid::FromString("f7bc0e1e-0f50-5fe0-a9e7-510b6ed83792").value(), {complex::FilterTraits<ChangeAngleRepresentation>::uuid, &ChangeAngleRepresentation::FromSIMPLJson}}, // ChangeAngleRepresentation
    {complex::Uuid::FromString("a6b50fb0-eb7c-5d9b-9691-825d6a4fe772").value(), {complex::FilterTraits<CombineAttributeArraysFilter>::uuid, &CombineAttributeArraysFilter::FromSIMPLJson}}, // CombineAttributeArrays
    //{complex::Uuid::FromString("47cafe63-83cc-5826-9521-4fb5bea684ef").value(), {complex::FilterTraits<ConditionalSetValue>::uuid, &ConditionalSetValue::FromSIMPLJson}}, // ConditionalSetValue
    {complex::Uuid::FromString("eb5a89c4-4e71-59b1-9719-d10a652d961e").value(), {complex::FilterTraits<ConvertColorToGrayScaleFilter>::uuid, &ConvertColorToGrayScaleFilter::FromSIMPLJson}}, // ConvertColorToGrayScale
    {complex::Uuid::FromString("f4ba5fa4-bb5c-5dd1-9429-0dd86d0ecb37").value(), {complex::FilterTraits<ConvertDataFilter>::uuid, &ConvertDataFilter::FromSIMPLJson}}, // ConvertData
    {complex::Uuid::FromString("a37f2e24-7400-5005-b9a7-b2224570cbe9").value(), {complex::FilterTraits<ConditionalSetValue>::uuid, &ConditionalSetValue::FromSIMPLJson}}, // ReplaceValueInArray
    {complex::Uuid::FromString("99836b75-144b-5126-b261-b411133b5e8a").value(), {complex::FilterTraits<CopyFeatureArrayToElementArray>::uuid, &CopyFeatureArrayToElementArray::FromSIMPLJson}}, // CopyFeatureArrayToElementArray
    {complex::Uuid::FromString("93375ef0-7367-5372-addc-baa019b1b341").value(), {complex::FilterTraits<CreateAttributeMatrixFilter>::uuid, &CreateAttributeMatrixFilter::FromSIMPLJson}}, // CreateAttributeMatrix
    {complex::Uuid::FromString("77f392fb-c1eb-57da-a1b1-e7acf9239fb8").value(), {complex::FilterTraits<CreateDataArray>::uuid, &CreateDataArray::FromSIMPLJson}}, // CreateDataArray
    {complex::Uuid::FromString("816fbe6b-7c38-581b-b149-3f839fb65b93").value(), {complex::FilterTraits<CreateDataGroup>::uuid, &CreateDataGroup::FromSIMPLJson}}, // CreateDataContainer
    {complex::Uuid::FromString("94438019-21bb-5b61-a7c3-66974b9a34dc").value(), {complex::FilterTraits<CreateFeatureArrayFromElementArray>::uuid, &CreateFeatureArrayFromElementArray::FromSIMPLJson}}, // CreateFeatureArrayFromElementArray
    {complex::Uuid::FromString("f2132744-3abb-5d66-9cd9-c9a233b5c4aa").value(), {complex::FilterTraits<CreateImageGeometry>::uuid, &CreateImageGeometry::FromSIMPLJson}}, // CreateImageGeometry
    {complex::Uuid::FromString("baa4b7fe-31e5-5e63-a2cb-0bb9d844cfaf").value(), {complex::FilterTraits<CropImageGeometry>::uuid, &CropImageGeometry::FromSIMPLJson}}, // CropImageGeometry
    {complex::Uuid::FromString("f28cbf07-f15a-53ca-8c7f-b41a11dae6cc").value(), {complex::FilterTraits<CropVertexGeometry>::uuid, &CropVertexGeometry::FromSIMPLJson}}, // CropVertexGeometry
    {complex::Uuid::FromString("7b1c8f46-90dd-584a-b3ba-34e16958a7d0").value(), {complex::FilterTraits<DeleteData>::uuid, &DeleteData::FromSIMPLJson}}, // RemoveArrays   
    {complex::Uuid::FromString("3fcd4c43-9d75-5b86-aad4-4441bc914f37").value(), {complex::FilterTraits<WriteDREAM3DFilter>::uuid, &WriteDREAM3DFilter::FromSIMPLJson}}, // DataContainerWriter
    {complex::Uuid::FromString("52a069b4-6a46-5810-b0ec-e0693c636034").value(), {complex::FilterTraits<ExtractInternalSurfacesFromTriangleGeometry>::uuid, &ExtractInternalSurfacesFromTriangleGeometry::FromSIMPLJson}}, // ExtractInternalSurfacesFromTriangleGeometry
    {complex::Uuid::FromString("737b8d5a-8622-50f9-9a8a-bfdb57608891").value(), {complex::FilterTraits<WriteFeatureDataCSVFilter>::uuid, &WriteFeatureDataCSVFilter::FromSIMPLJson}}, // FeatureDataCSVWriter
    {complex::Uuid::FromString("bf35f515-294b-55ed-8c69-211b7e69cb56").value(), {complex::FilterTraits<FindArrayStatisticsFilter>::uuid, &FindArrayStatisticsFilter::FromSIMPLJson}}, // FindArrayStatistics
    {complex::Uuid::FromString("8a1106d4-c67f-5e09-a02a-b2e9b99d031e").value(), {complex::FilterTraits<FindBoundaryCellsFilter>::uuid, &FindBoundaryCellsFilter::FromSIMPLJson}}, // FindBoundaryCellsFilter
    {complex::Uuid::FromString("450c2f00-9ddf-56e1-b4c1-0e74e7ad2349").value(), {complex::FilterTraits<FindBiasedFeaturesFilter>::uuid, &FindBiasedFeaturesFilter::FromSIMPLJson}}, // FindBiasedFeaturesFilter
    {complex::Uuid::FromString("29086169-20ce-52dc-b13e-824694d759aa").value(), {complex::FilterTraits<FindDifferencesMap>::uuid, &FindDifferencesMap::FromSIMPLJson}}, // FindDifferenceMap
    {complex::Uuid::FromString("933e4b2d-dd61-51c3-98be-00548ba783a3").value(), {complex::FilterTraits<FindEuclideanDistMapFilter>::uuid, &FindEuclideanDistMapFilter::FromSIMPLJson}}, // FindEuclideanDistMap
    {complex::Uuid::FromString("6f8ca36f-2995-5bd3-8672-6b0b80d5b2ca").value(), {complex::FilterTraits<FindFeatureCentroidsFilter>::uuid, &FindFeatureCentroidsFilter::FromSIMPLJson}}, // FindFeatureCentroids
    {complex::Uuid::FromString("6334ce16-cea5-5643-83b5-9573805873fa").value(), {complex::FilterTraits<FindFeaturePhasesFilter>::uuid, &FindFeaturePhasesFilter::FromSIMPLJson}}, // FindFeaturePhases
    {complex::Uuid::FromString("64d20c7b-697c-5ff1-9d1d-8a27b071f363").value(), {complex::FilterTraits<FindFeaturePhasesBinaryFilter>::uuid, &FindFeaturePhasesBinaryFilter::FromSIMPLJson}}, // FindFeaturePhasesBinary
    {complex::Uuid::FromString("697ed3de-db33-5dd1-a64b-04fb71e7d63e").value(), {complex::FilterTraits<FindNeighborhoodsFilter>::uuid, &FindNeighborhoodsFilter::FromSIMPLJson}}, // FindNeighborhoods
    {complex::Uuid::FromString("73ee33b6-7622-5004-8b88-4d145514fb6a").value(), {complex::FilterTraits<FindNeighborListStatistics>::uuid, &FindNeighborListStatistics::FromSIMPLJson}}, // FindNeighborListStatistics
    {complex::Uuid::FromString("97cf66f8-7a9b-5ec2-83eb-f8c4c8a17bac").value(), {complex::FilterTraits<FindNeighbors>::uuid, &FindNeighbors::FromSIMPLJson}}, // FindNeighbors
    {complex::Uuid::FromString("529743cf-d5d5-5d5a-a79f-95c84a5ddbb5").value(), {complex::FilterTraits<FindNumFeaturesFilter>::uuid, &FindNumFeaturesFilter::FromSIMPLJson}}, // FindNumFeatures
    {complex::Uuid::FromString("5d586366-6b59-566e-8de1-57aa9ae8a91c").value(), {complex::FilterTraits<FindSurfaceAreaToVolumeFilter>::uuid, &FindSurfaceAreaToVolumeFilter::FromSIMPLJson}}, // FindSurfaceAreaToVolume
    {complex::Uuid::FromString("d2b0ae3d-686a-5dc0-a844-66bc0dc8f3cb").value(), {complex::FilterTraits<FindSurfaceFeatures>::uuid, &FindSurfaceFeatures::FromSIMPLJson}}, // FindSurfaceFeatures
    {complex::Uuid::FromString("68246a67-7f32-5c80-815a-bec82008d7bc").value(), {complex::FilterTraits<FindVolFractionsFilter>::uuid, &FindVolFractionsFilter::FromSIMPLJson}}, // FindVolFractions
    {complex::Uuid::FromString("0d0a6535-6565-51c5-a3fc-fbc00008606d").value(), {complex::FilterTraits<GenerateColorTableFilter>::uuid, &GenerateColorTableFilter::FromSIMPLJson}}, // GenerateColorTable
    {complex::Uuid::FromString("0e8c0818-a3fb-57d4-a5c8-7cb8ae54a40a").value(), {complex::FilterTraits<IdentifySample>::uuid, &IdentifySample::FromSIMPLJson}}, // IdentifySample
    {complex::Uuid::FromString("f2259481-5011-5f22-9fcb-c92fb6f8be10").value(), {complex::FilterTraits<ReadBinaryCTNorthstarFilter>::uuid, &ReadBinaryCTNorthstarFilter::FromSIMPLJson}}, // ImportBinaryCTNorthstarFilter
    {complex::Uuid::FromString("bdb978bc-96bf-5498-972c-b509c38b8d50").value(), {complex::FilterTraits<ReadCSVFileFilter>::uuid, &ReadCSVFileFilter::FromSIMPLJson}}, // ReadASCIIData
    {complex::Uuid::FromString("043cbde5-3878-5718-958f-ae75714df0df").value(), {complex::FilterTraits<ReadDREAM3DFilter>::uuid, &ReadDREAM3DFilter::FromSIMPLJson}}, // DataContainerReader
    {complex::Uuid::FromString("9e98c3b0-5707-5a3b-b8b5-23ef83b02896").value(), {complex::FilterTraits<ReadHDF5Dataset>::uuid, &ReadHDF5Dataset::FromSIMPLJson}}, // ImportHDF5Dataset
    {complex::Uuid::FromString("a7007472-29e5-5d0a-89a6-1aed11b603f8").value(), {complex::FilterTraits<ReadTextDataArrayFilter>::uuid, &ReadTextDataArrayFilter::FromSIMPLJson}}, // ImportAsciDataArray
    {complex::Uuid::FromString("5fa10d81-94b4-582b-833f-8eabe659069e").value(), {complex::FilterTraits<ReadVolumeGraphicsFileFilter>::uuid, &ReadVolumeGraphicsFileFilter::FromSIMPLJson}}, // ImportVolumeGraphicsFileFilter
    {complex::Uuid::FromString("dfab9921-fea3-521c-99ba-48db98e43ff8").value(), {complex::FilterTraits<InitializeImageGeomCellData>::uuid, &InitializeImageGeomCellData::FromSIMPLJson}}, // InitializeData
    {complex::Uuid::FromString("4b551c15-418d-5081-be3f-d3aeb62408e5").value(), {complex::FilterTraits<InterpolatePointCloudToRegularGridFilter>::uuid, &InterpolatePointCloudToRegularGridFilter::FromSIMPLJson}}, // InterpolatePointCloudToRegularGrid
    {complex::Uuid::FromString("6c8fb24b-5b12-551c-ba6d-ae2fa7724764").value(), {complex::FilterTraits<IterativeClosestPointFilter>::uuid, &IterativeClosestPointFilter::FromSIMPLJson}}, // IterativeClosestPoint
    {complex::Uuid::FromString("601c4885-c218-5da6-9fc7-519d85d241ad").value(), {complex::FilterTraits<LaplacianSmoothingFilter>::uuid, &LaplacianSmoothingFilter::FromSIMPLJson}}, // LaplacianSmoothing
    {complex::Uuid::FromString("9fe34deb-99e1-5f3a-a9cc-e90c655b47ee").value(), {complex::FilterTraits<MapPointCloudToRegularGridFilter>::uuid, &MapPointCloudToRegularGridFilter::FromSIMPLJson}}, // MapPointCloudToRegularGrid
    {complex::Uuid::FromString("dab5de3c-5f81-5bb5-8490-73521e1183ea").value(), {complex::FilterTraits<MinNeighbors>::uuid, &MinNeighbors::FromSIMPLJson}}, // MinNeighbors
    {complex::Uuid::FromString("fe2cbe09-8ae1-5bea-9397-fd5741091fdb").value(), {complex::FilterTraits<MoveData>::uuid, &MoveData::FromSIMPLJson}}, // MoveData
    {complex::Uuid::FromString("014b7300-cf36-5ede-a751-5faf9b119dae").value(), {complex::FilterTraits<MultiThresholdObjects>::uuid, &MultiThresholdObjects::FromSIMPLJson}}, // MultiThresholdObjects
    {complex::Uuid::FromString("686d5393-2b02-5c86-b887-dd81a8ae80f2").value(), {complex::FilterTraits<MultiThresholdObjects>::uuid, &MultiThresholdObjects::FromSIMPLJson}}, // MultiThresholdObjects2
    {complex::Uuid::FromString("119861c5-e303-537e-b210-2e62936222e9").value(), {complex::FilterTraits<PointSampleTriangleGeometryFilter>::uuid, &PointSampleTriangleGeometryFilter::FromSIMPLJson}}, // PointSampleTriangleGeometry
    {complex::Uuid::FromString("07b49e30-3900-5c34-862a-f1fb48bad568").value(), {complex::FilterTraits<QuickSurfaceMeshFilter>::uuid, &QuickSurfaceMeshFilter::FromSIMPLJson}}, // QuickSurfaceMesh
    {complex::Uuid::FromString("0791f556-3d73-5b1e-b275-db3f7bb6850d").value(), {complex::FilterTraits<ReadRawBinaryFilter>::uuid, &ReadRawBinaryFilter::FromSIMPLJson}}, // RawBinaryReader
    {complex::Uuid::FromString("379ccc67-16dd-530a-984f-177db2314bac").value(), {complex::FilterTraits<RemoveFlaggedVertices>::uuid, &RemoveFlaggedVertices::FromSIMPLJson}}, // RemoveFlaggedVertices
    {complex::Uuid::FromString("53ac1638-8934-57b8-b8e5-4b91cdda23ec").value(), {complex::FilterTraits<RemoveMinimumSizeFeaturesFilter>::uuid, &RemoveMinimumSizeFeaturesFilter::FromSIMPLJson}}, // MinSize
    {complex::Uuid::FromString("53a5f731-2858-5e3e-bd43-8f2cf45d90ec").value(), {complex::FilterTraits<RenameDataObject>::uuid, &RenameDataObject::FromSIMPLJson}}, // RenameAttributeArray
    {complex::Uuid::FromString("ee29e6d6-1f59-551b-9350-a696523261d5").value(), {complex::FilterTraits<RenameDataObject>::uuid, &RenameDataObject::FromSIMPLJson}}, // RenameAttributeMatrix
    {complex::Uuid::FromString("d53c808f-004d-5fac-b125-0fffc8cc78d6").value(), {complex::FilterTraits<RenameDataObject>::uuid, &RenameDataObject::FromSIMPLJson}}, // RenameDataContainer
    {complex::Uuid::FromString("3062fc2c-76b2-5c50-92b7-edbbb424c41d").value(), {complex::FilterTraits<RobustAutomaticThreshold>::uuid, &RobustAutomaticThreshold::FromSIMPLJson}}, // RobustAutomaticThreshold
    {complex::Uuid::FromString("2c5edebf-95d8-511f-b787-90ee2adf485c").value(), {complex::FilterTraits<ScalarSegmentFeaturesFilter>::uuid, &ScalarSegmentFeaturesFilter::FromSIMPLJson}}, // ScalarSegmentFeatures
    {complex::Uuid::FromString("6d3a3852-6251-5d2e-b749-6257fd0d8951").value(), {complex::FilterTraits<SetImageGeomOriginScalingFilter>::uuid, &SetImageGeomOriginScalingFilter::FromSIMPLJson}}, // SetOriginResolutionImageGeom
    {complex::Uuid::FromString("5ecf77f4-a38a-52ab-b4f6-0fb8a9c5cb9c").value(), {complex::FilterTraits<SplitAttributeArrayFilter>::uuid, &SplitAttributeArrayFilter::FromSIMPLJson}}, // SplitAttributeArray
    {complex::Uuid::FromString("980c7bfd-20b2-5711-bc3b-0190b9096c34").value(), {complex::FilterTraits<ReadStlFileFilter>::uuid, &ReadStlFileFilter::FromSIMPLJson}}, // ReadStlFile
    {complex::Uuid::FromString("5fbf9204-2c6c-597b-856a-f4612adbac38").value(), {complex::FilterTraits<WriteASCIIDataFilter>::uuid, &WriteASCIIDataFilter::FromSIMPLJson}}, // WriteASCIIData
    {complex::Uuid::FromString("0541c5eb-1976-5797-9468-be50a93d44e2").value(), {complex::FilterTraits<TriangleDihedralAngleFilter>::uuid, &TriangleDihedralAngleFilter::FromSIMPLJson}}, // TriangleDihedralAngleFilter
    {complex::Uuid::FromString("928154f6-e4bc-5a10-a9dd-1abb6a6c0f6b").value(), {complex::FilterTraits<TriangleNormalFilter>::uuid, &TriangleNormalFilter::FromSIMPLJson}}, // TriangleNormalFilter
    {complex::Uuid::FromString("088ef69b-ca98-51a9-97ac-369862015d71").value(), {complex::FilterTraits<CopyDataObjectFilter>::uuid, &CopyDataObjectFilter::FromSIMPLJson}}, // CopyObject
    {complex::Uuid::FromString("79d59b85-01e8-5c4a-a6e1-3fd3e2ceffb4").value(), {complex::FilterTraits<ExtractComponentAsArrayFilter>::uuid, &ExtractComponentAsArrayFilter::FromSIMPLJson}}, // ExtractComponentAsArray
    {complex::Uuid::FromString("8a2308ec-86cd-5636-9a0a-6c7d383e9e7f").value(), {complex::FilterTraits<ExecuteProcessFilter>::uuid, &ExecuteProcessFilter::FromSIMPLJson}}, // ExecuteProcessFilter
    {complex::Uuid::FromString("1b4b9941-62e4-52f2-9918-15d48147ab88").value(), {complex::FilterTraits<ExtractComponentAsArrayFilter>::uuid, &ExtractComponentAsArrayFilter::FromSIMPLJson}}, // RemoveComponentFromArray
    {complex::Uuid::FromString("7aa33007-4186-5d7f-ba9d-d0a561b3351d").value(), {complex::FilterTraits<TriangleCentroidFilter>::uuid, &TriangleCentroidFilter::FromSIMPLJson}}, // TriangleCentroid
    {complex::Uuid::FromString("1966e540-759c-5798-ae26-0c6a3abc65c0").value(), {complex::FilterTraits<ResampleImageGeomFilter>::uuid, &ResampleImageGeomFilter::FromSIMPLJson}}, // ResampleImageGeom
    {complex::Uuid::FromString("e25d9b4c-2b37-578c-b1de-cf7032b5ef19").value(), {complex::FilterTraits<RotateSampleRefFrameFilter>::uuid, &RotateSampleRefFrameFilter::FromSIMPLJson}}, // RotateSampleRefFrame
    {complex::Uuid::FromString("30ae0a1e-3d94-5dab-b279-c5727ab5d7ff").value(), {complex::FilterTraits<FillBadDataFilter>::uuid, &FillBadDataFilter::FromSIMPLJson}}, // FillBadData
    {complex::Uuid::FromString("7aa33007-4186-5d7f-ba9d-d0a561b3351d").value(), {complex::FilterTraits<TriangleCentroidFilter>::uuid, {}}}, // TriangleCentroid  
    {complex::Uuid::FromString("17410178-4e5f-58b9-900e-8194c69200ab").value(), {complex::FilterTraits<ReplaceElementAttributesWithNeighborValuesFilter>::uuid, &ReplaceElementAttributesWithNeighborValuesFilter::FromSIMPLJson}}, // ReplaceElementAttributesWithNeighborValues
    {complex::Uuid::FromString("4fff1aa6-4f62-56c4-8ee9-8e28ec2fcbba").value(), {complex::FilterTraits<ErodeDilateMaskFilter>::uuid, &ErodeDilateMaskFilter::FromSIMPLJson}}, // ErodeDilateMask
    {complex::Uuid::FromString("d26e85ff-7e52-53ae-b095-b1d969c9e73c").value(), {complex::FilterTraits<ErodeDilateCoordinationNumberFilter>::uuid, &ErodeDilateCoordinationNumberFilter::FromSIMPLJson}}, // ErodeDilateCoordinationNumber
    {complex::Uuid::FromString("3adfe077-c3c9-5cd0-ad74-cf5f8ff3d254").value(), {complex::FilterTraits<ErodeDilateBadDataFilter>::uuid, &ErodeDilateBadDataFilter::FromSIMPLJson}}, // ErodeDilateBadData
    {complex::Uuid::FromString("a8463056-3fa7-530b-847f-7f4cb78b8602").value(), {complex::FilterTraits<RemoveFlaggedFeaturesFilter>::uuid, &RemoveFlaggedFeaturesFilter::FromSIMPLJson}}, // RemoveFlaggedFeatures
    {complex::Uuid::FromString("e0555de5-bdc6-5bea-ba2f-aacfbec0a022").value(), {complex::FilterTraits<RemoveFlaggedFeaturesFilter>::uuid, &RemoveFlaggedFeaturesFilter::FromSIMPLJson}}, // ExtractFlaggedFeatures
    {complex::Uuid::FromString("27a132b2-a592-519a-8cb7-38599a7f28ec").value(), {complex::FilterTraits<ComputeMomentInvariants2DFilter>::uuid, &ComputeMomentInvariants2DFilter::FromSIMPLJson}}, // ComputeMomentInvariants2D
    {complex::Uuid::FromString("fcdde553-36b4-5731-bc88-fc499806cb4e").value(), {complex::FilterTraits<FindVertexToTriangleDistancesFilter>::uuid, &FindVertexToTriangleDistancesFilter::FromSIMPLJson}}, // FindVertexToTriangleDistances
    {complex::Uuid::FromString("c681caf4-22f2-5885-bbc9-a0476abc72eb").value(), {complex::FilterTraits<ApplyTransformationToGeometryFilter>::uuid, {}}}, // ApplyTransformationToGeometry
    {complex::Uuid::FromString("6eda8dbf-dbd8-562a-ae1a-f2904157c189").value(), {complex::FilterTraits<ComputeFeatureRectFilter>::uuid, &ComputeFeatureRectFilter::FromSIMPLJson}}, // ComputeFeatureRect
    {complex::Uuid::FromString("9f77b4a9-6416-5220-a688-115f4e14c90d").value(), {complex::FilterTraits<FindLargestCrossSectionsFilter>::uuid, &FindLargestCrossSectionsFilter::FromSIMPLJson}}, // FindLargestCrossSections
    {complex::Uuid::FromString("b9134758-d5e5-59dd-9907-28d23e0e0143").value(), {complex::FilterTraits<WriteStlFileFilter>::uuid, &WriteStlFileFilter::FromSIMPLJson}}, // WriteStlFile
    {complex::Uuid::FromString("ac99b706-d1e0-5f78-9246-fbbe1efd93d2").value(), {complex::FilterTraits<AddBadDataFilter>::uuid, &AddBadDataFilter::FromSIMPLJson}}, // AddBadData
    {complex::Uuid::FromString("52b2918a-4fb5-57aa-97d4-ccc084b89572").value(), {complex::FilterTraits<AppendImageGeometryZSliceFilter>::uuid, &AppendImageGeometryZSliceFilter::FromSIMPLJson}}, // AppendImageGeometryZSlice
    {complex::Uuid::FromString("a1e9cf6d-2d1b-573e-98b8-0314c993d2b6").value(), {complex::FilterTraits<FindFeatureClusteringFilter>::uuid, &FindFeatureClusteringFilter::FromSIMPLJson}}, // FindFeatureClustering
    {complex::Uuid::FromString("0559aa37-c5ad-549a-82d4-bff4bfcb6cc6").value(), {complex::FilterTraits<WriteAbaqusHexahedronFilter>::uuid, &WriteAbaqusHexahedronFilter::FromSIMPLJson}}, // AbaqusHexahedronWriter
    {complex::Uuid::FromString("cbaf9e68-5ded-560c-9440-509289100ea8").value(), {complex::FilterTraits<NearestPointFuseRegularGridsFilter>::uuid, &NearestPointFuseRegularGridsFilter::FromSIMPLJson}}, // NearestPointFuseRegularGrids
    {complex::Uuid::FromString("77befd69-4536-5856-9f81-02996d038f73").value(), {complex::FilterTraits<ResampleRectGridToImageGeomFilter>::uuid, &ResampleRectGridToImageGeomFilter::FromSIMPLJson}}, // ResampleRectGridToImageGeom
    {complex::Uuid::FromString("71d46128-1d2d-58fd-9924-1714695768c3").value(), {complex::FilterTraits<CombineStlFilesFilter>::uuid, &CombineStlFilesFilter::FromSIMPLJson}}, // CombineStlFiles
    {complex::Uuid::FromString("339f1349-9236-5023-9a56-c82fb8eafd12").value(), {complex::FilterTraits<WriteAvizoUniformCoordinateFilter>::uuid, &WriteAvizoUniformCoordinateFilter::FromSIMPLJson}}, // AvizoUniformCoordinateWriter
    {complex::Uuid::FromString("2861f4b4-8d50-5e69-9575-68c9d35f1256").value(), {complex::FilterTraits<WriteAvizoRectilinearCoordinateFilter>::uuid, &WriteAvizoRectilinearCoordinateFilter::FromSIMPLJson}}, // AvizoRectilinearCoordinateWriter
    {complex::Uuid::FromString("a043bd66-2681-5126-82e1-5fdc46694bf4").value(), {complex::FilterTraits<WriteVtkRectilinearGridFilter>::uuid, &WriteVtkRectilinearGridFilter::FromSIMPLJson}}, // VtkRectilinearGridWriter
    {complex::Uuid::FromString("0df3da89-9106-538e-b1a9-6bbf1cf0aa92").value(), {complex::FilterTraits<RegularGridSampleSurfaceMeshFilter>::uuid, &RegularGridSampleSurfaceMeshFilter::FromSIMPLJson}}, // RegularGridSampleSurfaceMesh
    {complex::Uuid::FromString("75cfeb9b-cd4b-5a20-a344-4170b39bbfaf").value(), {complex::FilterTraits<UncertainRegularGridSampleSurfaceMeshFilter>::uuid, &UncertainRegularGridSampleSurfaceMeshFilter::FromSIMPLJson}}, // UncertainRegularGridSampleSurfaceMesh
    {complex::Uuid::FromString("6357243e-41a6-52c4-be2d-2f6894c39fcc").value(), {complex::FilterTraits<FindBoundaryElementFractionsFilter>::uuid, &FindBoundaryElementFractionsFilter::FromSIMPLJson}}, // FindBoundaryElementFractions
    {complex::Uuid::FromString("9b9fb9e1-074d-54b6-a6ce-0be21ab4496d").value(), {complex::FilterTraits<ReverseTriangleWindingFilter>::uuid, &ReverseTriangleWindingFilter::FromSIMPLJson}}, // ReverseTriangleWinding
    {complex::Uuid::FromString("158ebe9e-f772-57e2-ac1b-71ff213cf890").value(), {complex::FilterTraits<WriteLosAlamosFFTFilter>::uuid, &WriteLosAlamosFFTFilter::FromSIMPLJson}}, // LosAlamosFFTWriter
    {complex::Uuid::FromString("ef28de7e-5bdd-57c2-9318-60ba0dfaf7bc").value(), {complex::FilterTraits<GenerateVectorColorsFilter>::uuid, &GenerateVectorColorsFilter::FromSIMPLJson}}, // GenerateVectorColors
    {complex::Uuid::FromString("3c6337da-e232-4420-a5ca-451496748d88").value(), {complex::FilterTraits<ReadDeformKeyFileV12Filter>::uuid, &ReadDeformKeyFileV12Filter::FromSIMPLJson}}, // ImportDeformKeyFileV12Filter
    {complex::Uuid::FromString("f7486aa6-3049-5be7-8511-ae772b70c90b").value(), {complex::FilterTraits<KMedoidsFilter>::uuid, &KMedoidsFilter::FromSIMPLJson}}, // KMedoids
    {complex::Uuid::FromString("b56a04de-0ca0-509d-809f-52219fca9c98").value(), {complex::FilterTraits<KMeansFilter>::uuid, &KMeansFilter::FromSIMPLJson}}, // KMeans
    {complex::Uuid::FromString("f84d4d69-9ea5-54b6-a71c-df76d76d50cf").value(), {complex::FilterTraits<SilhouetteFilter>::uuid, &SilhouetteFilter::FromSIMPLJson}}, // Silhouette
    {complex::Uuid::FromString("a250a228-3b6b-5b37-a6e4-8687484f04c4").value(), {complex::FilterTraits<LabelTriangleGeometryFilter>::uuid, &LabelTriangleGeometryFilter::FromSIMPLJson}}, // LabelTriangleGeometry
    {complex::Uuid::FromString("379ccc67-16dd-530a-984f-177db9351bac").value(), {complex::FilterTraits<RemoveFlaggedTrianglesFilter>::uuid, &RemoveFlaggedTrianglesFilter::FromSIMPLJson}}, // RemoveFlaggedTriangles
    // @@__MAP__UPDATE__TOKEN__DO__NOT__DELETE__@@
  };

} // namespace complex
// clang-format on
