#pragma once

#include <map>
#include <string>

// clang-format off
#include "ComplexCore/Filters/AlignGeometries.hpp"
#include "ComplexCore/Filters/ApplyTransformationToGeometryFilter.hpp"
#include "ComplexCore/Filters/ApproximatePointCloudHull.hpp"
#include "ComplexCore/Filters/CalculateFeatureSizesFilter.hpp"
#include "ComplexCore/Filters/CalculateTriangleAreasFilter.hpp"
#include "ComplexCore/Filters/ChangeAngleRepresentation.hpp"
#include "ComplexCore/Filters/CombineAttributeArraysFilter.hpp"
#include "ComplexCore/Filters/ConditionalSetValue.hpp"
#include "ComplexCore/Filters/CopyDataGroup.hpp"
#include "ComplexCore/Filters/CopyFeatureArrayToElementArray.hpp"
#include "ComplexCore/Filters/CreateAttributeMatrixFilter.hpp"
#include "ComplexCore/Filters/CreateDataArray.hpp"
#include "ComplexCore/Filters/CreateDataGroup.hpp"
#include "ComplexCore/Filters/CreateFeatureArrayFromElementArray.hpp"
#include "ComplexCore/Filters/CreateImageGeometry.hpp"
#include "ComplexCore/Filters/CropImageGeometry.hpp"
#include "ComplexCore/Filters/CropVertexGeometry.hpp"
#include "ComplexCore/Filters/DeleteData.hpp"
#include "ComplexCore/Filters/ExportDREAM3DFilter.hpp"
#include "ComplexCore/Filters/ExtractInternalSurfacesFromTriangleGeometry.hpp"
#include "ComplexCore/Filters/FindArrayStatisticsFilter.hpp"
#include "ComplexCore/Filters/FindDifferencesMap.hpp"
#include "ComplexCore/Filters/FindFeaturePhasesFilter.hpp"
#include "ComplexCore/Filters/FindNeighborListStatistics.hpp"
#include "ComplexCore/Filters/FindNeighbors.hpp"
#include "ComplexCore/Filters/FindSurfaceFeatures.hpp"
#include "ComplexCore/Filters/IdentifySample.hpp"
#include "ComplexCore/Filters/ImportCSVDataFilter.hpp"
#include "ComplexCore/Filters/ImportDREAM3DFilter.hpp"
#include "ComplexCore/Filters/ImportHDF5Dataset.hpp"
#include "ComplexCore/Filters/ImportTextFilter.hpp"
#include "ComplexCore/Filters/InitializeData.hpp"
#include "ComplexCore/Filters/InterpolatePointCloudToRegularGridFilter.hpp"
#include "ComplexCore/Filters/IterativeClosestPointFilter.hpp"
#include "ComplexCore/Filters/LaplacianSmoothingFilter.hpp"
#include "ComplexCore/Filters/MapPointCloudToRegularGridFilter.hpp"
#include "ComplexCore/Filters/MinNeighbors.hpp"
#include "ComplexCore/Filters/MoveData.hpp"
#include "ComplexCore/Filters/MultiThresholdObjects.hpp"
#include "ComplexCore/Filters/PointSampleTriangleGeometryFilter.hpp"
#include "ComplexCore/Filters/QuickSurfaceMeshFilter.hpp"
#include "ComplexCore/Filters/RawBinaryReaderFilter.hpp"
#include "ComplexCore/Filters/RemoveFlaggedVertices.hpp"
#include "ComplexCore/Filters/RemoveMinimumSizeFeaturesFilter.hpp"
#include "ComplexCore/Filters/RenameDataObject.hpp"
#include "ComplexCore/Filters/RobustAutomaticThreshold.hpp"
#include "ComplexCore/Filters/ScalarSegmentFeaturesFilter.hpp"
#include "ComplexCore/Filters/SetImageGeomOriginScalingFilter.hpp"
#include "ComplexCore/Filters/StlFileReaderFilter.hpp"
#include "ComplexCore/Filters/TriangleDihedralAngleFilter.hpp"
#include "ComplexCore/Filters/TriangleNormalFilter.hpp"
// #include "Filters/FilterName.hpp"

namespace complex
{
  static const std::map<complex::Uuid, complex::Uuid> k_SIMPL_to_ComplexCore
  {
    // syntax std::make_pair {Dream3d UUID , Dream3dnx UUID}, // dream3d-class-name
    {complex::Uuid::FromString("ce1ee404-0336-536c-8aad-f9641c9458be").value(), complex::FilterTraits<AlignGeometries>::uuid}, // AlignGeometries
    {complex::Uuid::FromString("c681caf4-22f2-5885-bbc9-a0476abc72eb").value(), complex::FilterTraits<ApplyTransformationToGeometryFilter>::uuid}, // ApplyTransformationToGeometry
    {complex::Uuid::FromString("fab669ad-66c6-5a39-bdb7-fc47b94311ed").value(), complex::FilterTraits<ApproximatePointCloudHull>::uuid}, // ApproximatePointCloudHull
    {complex::Uuid::FromString("656f144c-a120-5c3b-bee5-06deab438588").value(), complex::FilterTraits<CalculateFeatureSizesFilter>::uuid}, // FindSizes
    {complex::Uuid::FromString("a9900cc3-169e-5a1b-bcf4-7569e1950d41").value(), complex::FilterTraits<CalculateTriangleAreasFilter>::uuid}, // TriangleAreaFilter
    {complex::Uuid::FromString("f7bc0e1e-0f50-5fe0-a9e7-510b6ed83792").value(), complex::FilterTraits<ChangeAngleRepresentation>::uuid}, // ChangeAngleRepresentation
    {complex::Uuid::FromString("a6b50fb0-eb7c-5d9b-9691-825d6a4fe772").value(), complex::FilterTraits<CombineAttributeArraysFilter>::uuid}, // CombineAttributeArrays
    {complex::Uuid::FromString("47cafe63-83cc-5826-9521-4fb5bea684ef").value(), complex::FilterTraits<ConditionalSetValue>::uuid}, // ConditionalSetValue
    {complex::Uuid::FromString("99836b75-144b-5126-b261-b411133b5e8a").value(), complex::FilterTraits<CopyFeatureArrayToElementArray>::uuid}, // CopyFeatureArrayToElementArray
    {complex::Uuid::FromString("93375ef0-7367-5372-addc-baa019b1b341").value(), complex::FilterTraits<CreateAttributeMatrixFilter>::uuid}, // CreateAttributeMatrix
    {complex::Uuid::FromString("77f392fb-c1eb-57da-a1b1-e7acf9239fb8").value(), complex::FilterTraits<CreateDataArray>::uuid}, // CreateDataArray
    {complex::Uuid::FromString("816fbe6b-7c38-581b-b149-3f839fb65b93").value(), complex::FilterTraits<CreateDataGroup>::uuid}, // CreateDataContainer
    {complex::Uuid::FromString("94438019-21bb-5b61-a7c3-66974b9a34dc").value(), complex::FilterTraits<CreateFeatureArrayFromElementArray>::uuid}, // CreateFeatureArrayFromElementArray
    {complex::Uuid::FromString("f2132744-3abb-5d66-9cd9-c9a233b5c4aa").value(), complex::FilterTraits<CreateImageGeometry>::uuid}, // CreateImageGeometry
    {complex::Uuid::FromString("baa4b7fe-31e5-5e63-a2cb-0bb9d844cfaf").value(), complex::FilterTraits<CropImageGeometry>::uuid}, // CropImageGeometry
    {complex::Uuid::FromString("f28cbf07-f15a-53ca-8c7f-b41a11dae6cc").value(), complex::FilterTraits<CropVertexGeometry>::uuid}, // CropVertexGeometry
    {complex::Uuid::FromString("7b1c8f46-90dd-584a-b3ba-34e16958a7d0").value(), complex::FilterTraits<DeleteData>::uuid}, // RemoveArrays
    {complex::Uuid::FromString("3fcd4c43-9d75-5b86-aad4-4441bc914f37").value(), complex::FilterTraits<ExportDREAM3DFilter>::uuid}, // DataContainerWriter
    {complex::Uuid::FromString("52a069b4-6a46-5810-b0ec-e0693c636034").value(), complex::FilterTraits<ExtractInternalSurfacesFromTriangleGeometry>::uuid}, // ExtractInternalSurfacesFromTriangleGeometry
    {complex::Uuid::FromString("bf35f515-294b-55ed-8c69-211b7e69cb56").value(), complex::FilterTraits<FindArrayStatisticsFilter>::uuid}, // FindArrayStatistics
    {complex::Uuid::FromString("29086169-20ce-52dc-b13e-824694d759aa").value(), complex::FilterTraits<FindDifferencesMap>::uuid}, // FindDifferenceMap
    {complex::Uuid::FromString("6334ce16-cea5-5643-83b5-9573805873fa").value(), complex::FilterTraits<FindFeaturePhasesFilter>::uuid}, // FindFeaturePhases
    {complex::Uuid::FromString("73ee33b6-7622-5004-8b88-4d145514fb6a").value(), complex::FilterTraits<FindNeighborListStatistics>::uuid}, // FindNeighborListStatistics
    {complex::Uuid::FromString("97cf66f8-7a9b-5ec2-83eb-f8c4c8a17bac").value(), complex::FilterTraits<FindNeighbors>::uuid}, // FindNeighbors
    {complex::Uuid::FromString("d2b0ae3d-686a-5dc0-a844-66bc0dc8f3cb").value(), complex::FilterTraits<FindSurfaceFeatures>::uuid}, // FindSurfaceFeatures
    {complex::Uuid::FromString("0e8c0818-a3fb-57d4-a5c8-7cb8ae54a40a").value(), complex::FilterTraits<IdentifySample>::uuid}, // IdentifySample
    {complex::Uuid::FromString("bdb978bc-96bf-5498-972c-b509c38b8d50").value(), complex::FilterTraits<ImportCSVDataFilter>::uuid}, // ReadASCIIData
    {complex::Uuid::FromString("043cbde5-3878-5718-958f-ae75714df0df").value(), complex::FilterTraits<ImportDREAM3DFilter>::uuid}, // DataContainerReader
    {complex::Uuid::FromString("9e98c3b0-5707-5a3b-b8b5-23ef83b02896").value(), complex::FilterTraits<ImportHDF5Dataset>::uuid}, // ImportHDF5Dataset
    {complex::Uuid::FromString("a7007472-29e5-5d0a-89a6-1aed11b603f8").value(), complex::FilterTraits<ImportTextFilter>::uuid}, // ImportAsciDataArray
    {complex::Uuid::FromString("dfab9921-fea3-521c-99ba-48db98e43ff8").value(), complex::FilterTraits<InitializeData>::uuid}, // InitializeData
    {complex::Uuid::FromString("4b551c15-418d-5081-be3f-d3aeb62408e5").value(), complex::FilterTraits<InterpolatePointCloudToRegularGridFilter>::uuid}, // InterpolatePointCloudToRegularGrid
    {complex::Uuid::FromString("6c8fb24b-5b12-551c-ba6d-ae2fa7724764").value(), complex::FilterTraits<IterativeClosestPointFilter>::uuid}, // IterativeClosestPoint
    {complex::Uuid::FromString("601c4885-c218-5da6-9fc7-519d85d241ad").value(), complex::FilterTraits<LaplacianSmoothingFilter>::uuid}, // LaplacianSmoothing
    {complex::Uuid::FromString("9fe34deb-99e1-5f3a-a9cc-e90c655b47ee").value(), complex::FilterTraits<MapPointCloudToRegularGridFilter>::uuid}, // MapPointCloudToRegularGrid
    {complex::Uuid::FromString("dab5de3c-5f81-5bb5-8490-73521e1183ea").value(), complex::FilterTraits<MinNeighbors>::uuid}, // MinNeighbors
    {complex::Uuid::FromString("fe2cbe09-8ae1-5bea-9397-fd5741091fdb").value(), complex::FilterTraits<MoveData>::uuid}, // MoveData
    {complex::Uuid::FromString("014b7300-cf36-5ede-a751-5faf9b119dae").value(), complex::FilterTraits<MultiThresholdObjects>::uuid}, // MultiThresholdObjects
    {complex::Uuid::FromString("686d5393-2b02-5c86-b887-dd81a8ae80f2").value(), complex::FilterTraits<MultiThresholdObjects>::uuid}, // MultiThresholdObjects2
    {complex::Uuid::FromString("119861c5-e303-537e-b210-2e62936222e9").value(), complex::FilterTraits<PointSampleTriangleGeometryFilter>::uuid}, // PointSampleTriangleGeometry
    {complex::Uuid::FromString("07b49e30-3900-5c34-862a-f1fb48bad568").value(), complex::FilterTraits<QuickSurfaceMeshFilter>::uuid}, // QuickSurfaceMesh
    {complex::Uuid::FromString("0791f556-3d73-5b1e-b275-db3f7bb6850d").value(), complex::FilterTraits<RawBinaryReaderFilter>::uuid}, // RawBinaryReader
    {complex::Uuid::FromString("379ccc67-16dd-530a-984f-177db2314bac").value(), complex::FilterTraits<RemoveFlaggedVertices>::uuid}, // RemoveFlaggedVertices
    {complex::Uuid::FromString("53ac1638-8934-57b8-b8e5-4b91cdda23ec").value(), complex::FilterTraits<RemoveMinimumSizeFeaturesFilter>::uuid}, // MinSize
    {complex::Uuid::FromString("53a5f731-2858-5e3e-bd43-8f2cf45d90ec").value(), complex::FilterTraits<RenameDataObject>::uuid}, // RenameAttributeArray
    {complex::Uuid::FromString("ee29e6d6-1f59-551b-9350-a696523261d5").value(), complex::FilterTraits<RenameDataObject>::uuid}, // RenameAttributeMatrix
    {complex::Uuid::FromString("d53c808f-004d-5fac-b125-0fffc8cc78d6").value(), complex::FilterTraits<RenameDataObject>::uuid}, // RenameDataContainer
    {complex::Uuid::FromString("3062fc2c-76b2-5c50-92b7-edbbb424c41d").value(), complex::FilterTraits<RobustAutomaticThreshold>::uuid}, // RobustAutomaticThreshold
    {complex::Uuid::FromString("2c5edebf-95d8-511f-b787-90ee2adf485c").value(), complex::FilterTraits<ScalarSegmentFeaturesFilter>::uuid}, // ScalarSegmentFeatures
    {complex::Uuid::FromString("6d3a3852-6251-5d2e-b749-6257fd0d8951").value(), complex::FilterTraits<SetImageGeomOriginScalingFilter>::uuid}, // SetOriginResolutionImageGeom
    {complex::Uuid::FromString("980c7bfd-20b2-5711-bc3b-0190b9096c34").value(), complex::FilterTraits<StlFileReaderFilter>::uuid}, // ReadStlFile
    {complex::Uuid::FromString("0541c5eb-1976-5797-9468-be50a93d44e2").value(), complex::FilterTraits<TriangleDihedralAngleFilter>::uuid}, // TriangleDihedralAngleFilter
    {complex::Uuid::FromString("8133d419-1919-4dbf-a5bf-1c97282ba63f").value(), complex::FilterTraits<TriangleNormalFilter>::uuid}, // TriangleNormalFilter
    // {complex::Uuid::FromString(insert DREAM3D UUID string here).value(), complex::FilterTraits<insert DREAM3DNX filter name here>::uuid}, // dream3d-class-name
  };

} // namespace complex
// clang-format on
