#pragma once

#include <map>
#include <string>

namespace complex
{
  // Regex to grep UUIDs : {\"([0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12})\",\s\"([0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}(?:\s,\s[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12})*)\"},\s\/\/\s(\w+)
  static const std::map<std::string, std::string> k_SIMPL_to_ComplexCore
  {
    // syntax std::make_pair {Dream3d UUID , Dream3dnx UUID}, // dream3d-class-name
    {"ce1ee404-0336-536c-8aad-f9641c9458be", "87fa9e07-6c66-45b0-80a0-cf80cc0def5d"}, // AlignGeometries
    {"c681caf4-22f2-5885-bbc9-a0476abc72eb", "f5bbc16b-3426-4ae0-b27b-ba7862dc40fe"}, // ApplyTransformationToGeometry
    {"fab669ad-66c6-5a39-bdb7-fc47b94311ed", "c19203b7-2217-4e52-bff4-7f611695421a"}, // ApproximatePointCloudHull
    {"656f144c-a120-5c3b-bee5-06deab438588", "c666ee17-ca58-4969-80d0-819986c72485"}, // FindSizes
    {"a9900cc3-169e-5a1b-bcf4-7569e1950d41", "b149addd-c0c8-4010-a264-596005eaf2a5"}, // TriangleAreaFilter
    {"f7bc0e1e-0f50-5fe0-a9e7-510b6ed83792", "565e06e2-6fd0-4232-89c4-ee672926d565"}, // ChangeAngleRepresentation
    {"a6b50fb0-eb7c-5d9b-9691-825d6a4fe772", "2436b614-e96d-47f0-9f6f-41d6fe97acd4"}, // CombineAttributeArrays
    {"47cafe63-83cc-5826-9521-4fb5bea684ef", "bad9b7bd-1dc9-4f21-a889-6520e7a41881"}, // ConditionalSetValue
    {"99836b75-144b-5126-b261-b411133b5e8a", "4c8c976a-993d-438b-bd8e-99f71114b9a1"}, // CopyFeatureArrayToElementArray
    {"93375ef0-7367-5372-addc-baa019b1b341", "a6a28355-ee69-4874-bcac-76ed427423ed"}, // CreateAttributeMatrix
    {"77f392fb-c1eb-57da-a1b1-e7acf9239fb8", "67041f9b-bdc6-4122-acc6-c9fe9280e90d"}, // CreateDataArray
    {"816fbe6b-7c38-581b-b149-3f839fb65b93", "e7d2f9b8-4131-4b28-a843-ea3c6950f101"}, // CreateDataContainer
    {"94438019-21bb-5b61-a7c3-66974b9a34dc", "50e1be47-b027-4f40-8f70-1283682ee3e7"}, // CreateFeatureArrayFromElementArray
    {"f2132744-3abb-5d66-9cd9-c9a233b5c4aa", "c4320659-1a84-461d-939e-c7c10229a504"}, // CreateImageGeometry
    {"baa4b7fe-31e5-5e63-a2cb-0bb9d844cfaf", "e6476737-4aa7-48ba-a702-3dfab82c96e2"}, // CropImageGeometry
    {"f28cbf07-f15a-53ca-8c7f-b41a11dae6cc", "8b16452f-f75e-4918-9460-d3914fdc0d08"}, // CropVertexGeometry
    {"7b1c8f46-90dd-584a-b3ba-34e16958a7d0", "bf286740-e987-49fe-a7c8-6e566e3a0606"}, // RemoveArrays
    {"3fcd4c43-9d75-5b86-aad4-4441bc914f37", "b3a95784-2ced-41ec-8d3d-0242ac130003"}, // DataContainerWriter
    {"52a069b4-6a46-5810-b0ec-e0693c636034", "e020f76f-a77f-4999-8bf1-9b7529f06d0a"}, // ExtractInternalSurfacesFromTriangleGeometry
    {"bf35f515-294b-55ed-8c69-211b7e69cb56", "645ecae2-cb30-4b53-8165-c9857dfa754f"}, // FindArrayStatistics
    {"29086169-20ce-52dc-b13e-824694d759aa", "5a0ee5b5-c135-4535-85d0-9c2058943099"}, // FindDifferenceMap
    {"6334ce16-cea5-5643-83b5-9573805873fa", "da5bb20e-4a8e-49d9-9434-fbab7bc434fc"}, // FindFeaturePhases
    {"73ee33b6-7622-5004-8b88-4d145514fb6a", "270a824e-414b-455e-bb7e-b38a0848990d"}, // FindNeighborListStatistics
    {"97cf66f8-7a9b-5ec2-83eb-f8c4c8a17bac", "7177e88c-c3ab-4169-abe9-1fdaff20e598"}, // FindNeighbors
    {"d2b0ae3d-686a-5dc0-a844-66bc0dc8f3cb", "0893e490-5d24-4c21-95e7-e8372baa8948"}, // FindSurfaceFeatures
    {"0e8c0818-a3fb-57d4-a5c8-7cb8ae54a40a", "94d47495-5a89-4c7f-a0ee-5ff20e6bd273"}, // IdentifySample
    {"bdb978bc-96bf-5498-972c-b509c38b8d50", "373be1f8-31cf-49f6-aa5d-e356f4f3f261"}, // ReadASCIIData
    {"043cbde5-3878-5718-958f-ae75714df0df", "0dbd31c7-19e0-4077-83ef-f4a6459a0e2d"}, // DataContainerReader
    {"9e98c3b0-5707-5a3b-b8b5-23ef83b02896", "8027f145-c7d5-4589-900e-b909fb3a059c"}, // ImportHDF5Dataset
    {"a7007472-29e5-5d0a-89a6-1aed11b603f8", "25f7df3e-ca3e-4634-adda-732c0e56efd4"}, // ImportAsciDataArray
    {"dfab9921-fea3-521c-99ba-48db98e43ff8", "447b8909-661f-446a-8c1f-72e0cb568fcf"}, // InitializeData
    {"4b551c15-418d-5081-be3f-d3aeb62408e5", "996c4464-08f0-4268-a8a6-f9796c88cf58"}, // InterpolatePointCloudToRegularGrid
    {"6c8fb24b-5b12-551c-ba6d-ae2fa7724764", "3a206668-fa44-418d-b78e-9fd803b8fb98"}, // IterativeClosestPoint
    {"601c4885-c218-5da6-9fc7-519d85d241ad", "0dd0916e-9305-4a7b-98cf-a6cfb97cb501"}, // LaplacianSmoothing
    {"9fe34deb-99e1-5f3a-a9cc-e90c655b47ee", "af53ac60-092f-4e4a-9e13-57f0034ce2c7"}, // MapPointCloudToRegularGrid
    {"dab5de3c-5f81-5bb5-8490-73521e1183ea", "4ab5153f-6014-4e6d-bbd6-194068620389"}, // MinNeighbors
    {"fe2cbe09-8ae1-5bea-9397-fd5741091fdb", "651e5894-ab7c-4176-b7f0-ea466c521753"}, // MoveData
    {"014b7300-cf36-5ede-a751-5faf9b119dae", "4246245e-1011-4add-8436-0af6bed19228"}, // MultiThresholdObjects
    {"686d5393-2b02-5c86-b887-dd81a8ae80f2", "4246245e-1011-4add-8436-0af6bed19228"}, // MultiThresholdObjects2
    {"119861c5-e303-537e-b210-2e62936222e9", "ee34ef95-aa04-4ad3-8232-5783a880d279"}, // PointSampleTriangleGeometry
    {"07b49e30-3900-5c34-862a-f1fb48bad568", "13dd00bd-ad49-4e04-95eb-3267952fd6e5"}, // QuickSurfaceMesh
    {"0791f556-3d73-5b1e-b275-db3f7bb6850d", "dd159366-5f12-42db-af6d-a33592ae8a89"}, // RawBinaryReader
    {"379ccc67-16dd-530a-984f-177db2314bac", "46099b4c-ef90-4fb3-b5e9-6c8c543c5be1"}, // RemoveFlaggedVertices
    {"53ac1638-8934-57b8-b8e5-4b91cdda23ec", "074472d3-ba8d-4a1a-99f2-2d56a0d082a0"}, // MinSize
    {"53a5f731-2858-5e3e-bd43-8f2cf45d90ec", "911a3aa9-d3c2-4f66-9451-8861c4b726d5"}, // RenameAttributeArray
    {"ee29e6d6-1f59-551b-9350-a696523261d5", "911a3aa9-d3c2-4f66-9451-8861c4b726d5"}, // RenameAttributeMatrix
    {"d53c808f-004d-5fac-b125-0fffc8cc78d6", "911a3aa9-d3c2-4f66-9451-8861c4b726d5"}, // RenameDataContainer
    {"3062fc2c-76b2-5c50-92b7-edbbb424c41d", "ade392e6-f0da-4cf3-bf11-dfe69e200b49"}, // RobustAutomaticThreshold
    {"2c5edebf-95d8-511f-b787-90ee2adf485c", "e067cd97-9bbf-4c92-89a6-3cb4fdb76c93"}, // ScalarSegmentFeatures
    {"6d3a3852-6251-5d2e-b749-6257fd0d8951", "057bc7fd-c84a-4902-9397-87e51b1b1fe0"}, // SetOriginResolutionImageGeom
    {"980c7bfd-20b2-5711-bc3b-0190b9096c34", "2f64bd45-9d28-4254-9e07-6aa7c6d3d015"}, // ReadStlFile
    {"0541c5eb-1976-5797-9468-be50a93d44e2", "dd42c521-4ae5-485d-ad35-d1276547d2f1"}, // TriangleDihedralAngleFilter
    {"8133d419-1919-4dbf-a5bf-1c97282ba63f", "928154f6-e4bc-5a10-a9dd-1abb6a6c0f6b"}, // TriangleNormalFilter
    // {insert DREAM3D UUID here, insert DREAM3DNX UUID here}, // dream3d-class-name
  };

  static const std::map<std::string, std::string> k_ComplexCore_to_SIMPL
  {
    // syntax std::make_pair {Dream3dnx UUID , Dream3d UUID}, // dream3dnx-class-name
    {"87fa9e07-6c66-45b0-80a0-cf80cc0def5d", "ce1ee404-0336-536c-8aad-f9641c9458be"}, // AlignGeometries
    {"f5bbc16b-3426-4ae0-b27b-ba7862dc40fe", "c681caf4-22f2-5885-bbc9-a0476abc72eb"}, // ApplyTransformationToGeometryFilter
    {"c19203b7-2217-4e52-bff4-7f611695421a", "fab669ad-66c6-5a39-bdb7-fc47b94311ed"}, // ApproximatePointCloudHull
    {"c666ee17-ca58-4969-80d0-819986c72485", "656f144c-a120-5c3b-bee5-06deab438588"}, // CalculateFeatureSizesFilter
    {"b149addd-c0c8-4010-a264-596005eaf2a5", "a9900cc3-169e-5a1b-bcf4-7569e1950d41"}, // CalculateTriangleAreasFilter
    {"565e06e2-6fd0-4232-89c4-ee672926d565", "f7bc0e1e-0f50-5fe0-a9e7-510b6ed83792"}, // ChangeAngleRepresentation
    {"2436b614-e96d-47f0-9f6f-41d6fe97acd4", "a6b50fb0-eb7c-5d9b-9691-825d6a4fe772"}, // CombineAttributeArrays
    {"bad9b7bd-1dc9-4f21-a889-6520e7a41881", "47cafe63-83cc-5826-9521-4fb5bea684ef"}, // ConditionalSetValue
    {"4c8c976a-993d-438b-bd8e-99f71114b9a1", "99836b75-144b-5126-b261-b411133b5e8a"}, // CopyFeatureArrayToElementArray
    {"a6a28355-ee69-4874-bcac-76ed427423ed", "93375ef0-7367-5372-addc-baa019b1b341"}, // CreateAttributeMatrixFilter
    {"67041f9b-bdc6-4122-acc6-c9fe9280e90d", "77f392fb-c1eb-57da-a1b1-e7acf9239fb8"}, // CreateDataArray
    {"e7d2f9b8-4131-4b28-a843-ea3c6950f101", "816fbe6b-7c38-581b-b149-3f839fb65b93"}, // CreateDataGroup
    {"50e1be47-b027-4f40-8f70-1283682ee3e7", "94438019-21bb-5b61-a7c3-66974b9a34dc"}, // CreateFeatureArrayFromElementArray
    {"c4320659-1a84-461d-939e-c7c10229a504", "f2132744-3abb-5d66-9cd9-c9a233b5c4aa"}, // CreateImageGeometry
    {"e6476737-4aa7-48ba-a702-3dfab82c96e2", "baa4b7fe-31e5-5e63-a2cb-0bb9d844cfaf"}, // CropImageGeometry
    {"8b16452f-f75e-4918-9460-d3914fdc0d08", "f28cbf07-f15a-53ca-8c7f-b41a11dae6cc"}, // CropVertexGeometry
    {"bf286740-e987-49fe-a7c8-6e566e3a0606", "7b1c8f46-90dd-584a-b3ba-34e16958a7d0"}, // DeleteData
    {"b3a95784-2ced-41ec-8d3d-0242ac130003", "3fcd4c43-9d75-5b86-aad4-4441bc914f37"}, // ExportDREAM3DFilter
    {"e020f76f-a77f-4999-8bf1-9b7529f06d0a", "52a069b4-6a46-5810-b0ec-e0693c636034"}, // ExtractInternalSurfacesFromTriangleGeometry
    {"645ecae2-cb30-4b53-8165-c9857dfa754f", "bf35f515-294b-55ed-8c69-211b7e69cb56"}, // FindArrayStatisticsFilter
    {"5a0ee5b5-c135-4535-85d0-9c2058943099", "29086169-20ce-52dc-b13e-824694d759aa"}, // FindDifferencesMap
    {"da5bb20e-4a8e-49d9-9434-fbab7bc434fc", "6334ce16-cea5-5643-83b5-9573805873fa"}, // FindFeaturePhasesFilter
    {"270a824e-414b-455e-bb7e-b38a0848990d", "73ee33b6-7622-5004-8b88-4d145514fb6a"}, // FindNeighborListStatistics
    {"7177e88c-c3ab-4169-abe9-1fdaff20e598", "97cf66f8-7a9b-5ec2-83eb-f8c4c8a17bac"}, // FindNeighbors
    {"0893e490-5d24-4c21-95e7-e8372baa8948", "d2b0ae3d-686a-5dc0-a844-66bc0dc8f3cb"}, // FindSurfaceFeatures
    {"94d47495-5a89-4c7f-a0ee-5ff20e6bd273", "0e8c0818-a3fb-57d4-a5c8-7cb8ae54a40a"}, // IdentifySample
    {"373be1f8-31cf-49f6-aa5d-e356f4f3f261", "bdb978bc-96bf-5498-972c-b509c38b8d50"}, // ImportCSVDataFilter
    {"0dbd31c7-19e0-4077-83ef-f4a6459a0e2d", "043cbde5-3878-5718-958f-ae75714df0df"}, // ImportDREAM3DFilter
    {"8027f145-c7d5-4589-900e-b909fb3a059c", "9e98c3b0-5707-5a3b-b8b5-23ef83b02896"}, // ImportHDF5Dataset
    {"25f7df3e-ca3e-4634-adda-732c0e56efd4", "a7007472-29e5-5d0a-89a6-1aed11b603f8"}, // ImportTextFilter
    {"447b8909-661f-446a-8c1f-72e0cb568fcf", "dfab9921-fea3-521c-99ba-48db98e43ff8"}, // InitializeData
    {"996c4464-08f0-4268-a8a6-f9796c88cf58", "4b551c15-418d-5081-be3f-d3aeb62408e5"}, // InterpolatePointCloudToRegularGridFilter
    {"3a206668-fa44-418d-b78e-9fd803b8fb98", "6c8fb24b-5b12-551c-ba6d-ae2fa7724764"}, // IterativeClosestPointFilter
    {"0dd0916e-9305-4a7b-98cf-a6cfb97cb501", "601c4885-c218-5da6-9fc7-519d85d241ad"}, // LaplacianSmoothingFilter
    {"af53ac60-092f-4e4a-9e13-57f0034ce2c7", "9fe34deb-99e1-5f3a-a9cc-e90c655b47ee"}, // MapPointCloudToRegularGridFilter
    {"4ab5153f-6014-4e6d-bbd6-194068620389", "dab5de3c-5f81-5bb5-8490-73521e1183ea"}, // MinNeighbors
    {"651e5894-ab7c-4176-b7f0-ea466c521753", "fe2cbe09-8ae1-5bea-9397-fd5741091fdb"}, // MoveData
    {"4246245e-1011-4add-8436-0af6bed19228", "686d5393-2b02-5c86-b887-dd81a8ae80f2"}, // MultiThresholdObjects
    {"ee34ef95-aa04-4ad3-8232-5783a880d279", "014b7300-cf36-5ede-a751-5faf9b119dae , 119861c5-e303-537e-b210-2e62936222e9"}, // PointSampleTriangleGeometryFilter
    {"13dd00bd-ad49-4e04-95eb-3267952fd6e5", "07b49e30-3900-5c34-862a-f1fb48bad568"}, // QuickSurfaceMeshFilter
    {"dd159366-5f12-42db-af6d-a33592ae8a89", "0791f556-3d73-5b1e-b275-db3f7bb6850d"}, // RawBinaryReaderFilter
    {"46099b4c-ef90-4fb3-b5e9-6c8c543c5be1", "379ccc67-16dd-530a-984f-177db2314bac"}, // RemoveFlaggedVertices
    {"074472d3-ba8d-4a1a-99f2-2d56a0d082a0", "53ac1638-8934-57b8-b8e5-4b91cdda23ec"}, // RemoveMinimumSizeFeaturesFilter
    {"911a3aa9-d3c2-4f66-9451-8861c4b726d5", "53a5f731-2858-5e3e-bd43-8f2cf45d90ec , ee29e6d6-1f59-551b-9350-a696523261d5 , d53c808f-004d-5fac-b125-0fffc8cc78d6"}, // RenameDataObject
    {"ade392e6-f0da-4cf3-bf11-dfe69e200b49", "3062fc2c-76b2-5c50-92b7-edbbb424c41d"}, // RobustAutomaticThreshold
    {"e067cd97-9bbf-4c92-89a6-3cb4fdb76c93", "2c5edebf-95d8-511f-b787-90ee2adf485c"}, // ScalarSegmentFeaturesFilter
    {"057bc7fd-c84a-4902-9397-87e51b1b1fe0", "6d3a3852-6251-5d2e-b749-6257fd0d8951"}, // SetImageGeomOriginScalingFilter
    {"2f64bd45-9d28-4254-9e07-6aa7c6d3d015", "980c7bfd-20b2-5711-bc3b-0190b9096c34"}, // StlFileReaderFilter
    {"dd42c521-4ae5-485d-ad35-d1276547d2f1", "0541c5eb-1976-5797-9468-be50a93d44e2"}, // TriangleDihedralAngleFilter
    {"928154f6-e4bc-5a10-a9dd-1abb6a6c0f6b", "8133d419-1919-4dbf-a5bf-1c97282ba63f"}, // TriangleNormalFilter
    // {insert DREAM3DNX UUID here, insert DREAM3D UUID here}, // dream3dnx-class-name
  };

} // namespace complex