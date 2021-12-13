#include "DREAM3DReviewPlugin.hpp"

using namespace complex;

namespace
{
// This maps previous filters from DREAM.3D Version 6.x to DREAM.3D Version 7.x
std::map<complex::Uuid, complex::Uuid> k_SimplToComplexFilterMapping = {
    {Uuid::FromString("{28977b7a-5b88-5145-abcd-d1c933f7d975}").value(), Uuid::FromString("28977b7a-5b88-5145-abcd-d1c933f7d975").value()}, /* AdaptiveAlignmentFeature */
    {Uuid::FromString("{8ef88380-ece9-5f8e-a12d-d149d0856752}").value(), Uuid::FromString("8ef88380-ece9-5f8e-a12d-d149d0856752").value()}, /* AdaptiveAlignmentMisorientation */
    {Uuid::FromString("{738c8da9-45d0-53dd-aa54-3f3a337b70d7}").value(), Uuid::FromString("738c8da9-45d0-53dd-aa54-3f3a337b70d7").value()}, /* AdaptiveAlignmentMutualInformation */
    {Uuid::FromString("{ce1ee404-0336-536c-8aad-f9641c9458be}").value(), Uuid::FromString("ce1ee404-0336-536c-8aad-f9641c9458be").value()}, /* AlignGeometries */
    {Uuid::FromString("{c681caf4-22f2-5885-bbc9-a0476abc72eb}").value(), Uuid::FromString("c681caf4-22f2-5885-bbc9-a0476abc72eb").value()}, /* ApplyTransformationToGeometry */
    {Uuid::FromString("{fab669ad-66c6-5a39-bdb7-fc47b94311ed}").value(), Uuid::FromString("fab669ad-66c6-5a39-bdb7-fc47b94311ed").value()}, /* ApproximatePointCloudHull */
    {Uuid::FromString("{9df4c18a-f51b-5698-8067-530d37f57c61}").value(), Uuid::FromString("9df4c18a-f51b-5698-8067-530d37f57c61").value()}, /* AverageEdgeFaceCellArrayToVertexArray */
    {Uuid::FromString("{caed6d53-6846-523f-a837-0692b3a81570}").value(), Uuid::FromString("caed6d53-6846-523f-a837-0692b3a81570").value()}, /* AverageVertexArrayToEdgeFaceCellArray */
    {Uuid::FromString("{71d46128-1d2d-58fd-9924-1714695768c3}").value(), Uuid::FromString("71d46128-1d2d-58fd-9924-1714695768c3").value()}, /* CombineStlFiles */
    {Uuid::FromString("{879e1eb8-40dc-5a5b-abe5-7e0baa77ed73}").value(), Uuid::FromString("879e1eb8-40dc-5a5b-abe5-7e0baa77ed73").value()}, /* ComputeFeatureEigenstrains */
    {Uuid::FromString("{3192d494-d1ec-5ee7-a345-e9963f02aaab}").value(), Uuid::FromString("3192d494-d1ec-5ee7-a345-e9963f02aaab").value()}, /* ComputeUmeyamaTransform */
    {Uuid::FromString("{3dd5d1cb-d39d-5bb6-aab5-4f64c0892e24}").value(), Uuid::FromString("3dd5d1cb-d39d-5bb6-aab5-4f64c0892e24").value()}, /* CreateArrayofIndices */
    {Uuid::FromString("{c2d4f1e8-2b04-5d82-b90f-2191e8f4262e}").value(), Uuid::FromString("c2d4f1e8-2b04-5d82-b90f-2191e8f4262e").value()}, /* DBSCAN */
    {Uuid::FromString("{10b319ca-6b2f-5118-9f4e-d19ed481cd1f}").value(), Uuid::FromString("10b319ca-6b2f-5118-9f4e-d19ed481cd1f").value()}, /* DelaunayTriangulation */
    {Uuid::FromString("{ebdfe707-0c9c-5552-89f6-6ee4a1e0891b}").value(), Uuid::FromString("ebdfe707-0c9c-5552-89f6-6ee4a1e0891b").value()}, /* DiscretizeDDDomain */
    {Uuid::FromString("{676cca06-40ab-56fb-bb15-8f31c6fa8c3e}").value(), Uuid::FromString("676cca06-40ab-56fb-bb15-8f31c6fa8c3e").value()}, /* DownsampleVertexGeometry */
    {Uuid::FromString("{14ad5792-a186-5e96-8e9e-4d623919dabc}").value(), Uuid::FromString("14ad5792-a186-5e96-8e9e-4d623919dabc").value()}, /* EstablishFoamMorphology */
    {Uuid::FromString("{351c38c9-330d-599c-8632-19d74868be86}").value(), Uuid::FromString("351c38c9-330d-599c-8632-19d74868be86").value()}, /* ExportCLIFile */
    {Uuid::FromString("{52a069b4-6a46-5810-b0ec-e0693c636034}").value(), Uuid::FromString("52a069b4-6a46-5810-b0ec-e0693c636034").value()}, /* ExtractInternalSurfacesFromTriangleGeometry */
    {Uuid::FromString("{1422a4d7-ecd3-5e84-8883-8d2c8c551675}").value(), Uuid::FromString("1422a4d7-ecd3-5e84-8883-8d2c8c551675").value()}, /* ExtractTripleLinesFromTriangleGeometry */
    {Uuid::FromString("{b6b1ba7c-14aa-5c6f-9436-8c46face6053}").value(), Uuid::FromString("b6b1ba7c-14aa-5c6f-9436-8c46face6053").value()}, /* FFTHDFWriterFilter */
    {Uuid::FromString("{bf35f515-294b-55ed-8c69-211b7e69cb56}").value(), Uuid::FromString("bf35f515-294b-55ed-8c69-211b7e69cb56").value()}, /* FindArrayStatistics */
    {Uuid::FromString("{d87ff37c-120d-577d-a955-571c8280fa8e}").value(), Uuid::FromString("d87ff37c-120d-577d-a955-571c8280fa8e").value()}, /* FindCSLBoundaries */
    {Uuid::FromString("{ef37f78b-bc9a-5884-b372-d882df6abe74}").value(), Uuid::FromString("ef37f78b-bc9a-5884-b372-d882df6abe74").value()}, /* FindElementCentroids */
    {Uuid::FromString("{c85277c7-9f02-5891-8141-04523658737d}").value(), Uuid::FromString("c85277c7-9f02-5891-8141-04523658737d").value()}, /* FindLayerStatistics */
    {Uuid::FromString("{6cc3148c-0bad-53b4-9568-ee1971cadb00}").value(), Uuid::FromString("6cc3148c-0bad-53b4-9568-ee1971cadb00").value()}, /* FindMinkowskiBouligandDimension */
    {Uuid::FromString("{73ee33b6-7622-5004-8b88-4d145514fb6a}").value(), Uuid::FromString("73ee33b6-7622-5004-8b88-4d145514fb6a").value()}, /* FindNeighborListStatistics */
    {Uuid::FromString("{5d0cd577-3e3e-57b8-a36d-b215b834251f}").value(), Uuid::FromString("5d0cd577-3e3e-57b8-a36d-b215b834251f").value()}, /* FindNorm */
    {Uuid::FromString("{4178c7f9-5f90-5e95-8cf1-a67ca2a98a60}").value(), Uuid::FromString("4178c7f9-5f90-5e95-8cf1-a67ca2a98a60").value()}, /* FindSurfaceRoughness */
    {Uuid::FromString("{fcdde553-36b4-5731-bc88-fc499806cb4e}").value(), Uuid::FromString("fcdde553-36b4-5731-bc88-fc499806cb4e").value()}, /* FindVertexToTriangleDistances */
    {Uuid::FromString("{8b2fa51e-3bad-58ec-9fbf-b03b065e67fc}").value(), Uuid::FromString("8b2fa51e-3bad-58ec-9fbf-b03b065e67fc").value()}, /* GenerateFeatureIDsbyBoundingBoxes */
    {Uuid::FromString("{e4d6fda0-1143-56cc-9d00-c09a94e2f501}").value(), Uuid::FromString("e4d6fda0-1143-56cc-9d00-c09a94e2f501").value()}, /* GenerateMaskFromSimpleShapes */
    {Uuid::FromString("{073798a1-1fb4-5e3c-81f6-e426f60e347a}").value(), Uuid::FromString("073798a1-1fb4-5e3c-81f6-e426f60e347a").value()}, /* IdentifyDislocationSegments */
    {Uuid::FromString("{76ec2a58-a0cd-5548-b248-5a5eb08a1581}").value(), Uuid::FromString("76ec2a58-a0cd-5548-b248-5a5eb08a1581").value()}, /* ImportCLIFile */
    {Uuid::FromString("{85654f78-04a8-50ed-9ae1-25dfbd0539b3}").value(), Uuid::FromString("85654f78-04a8-50ed-9ae1-25dfbd0539b3").value()}, /* ImportMASSIFData */
    {Uuid::FromString("{ab8f6892-2b57-5ec6-88b7-01610d80c32c}").value(), Uuid::FromString("ab8f6892-2b57-5ec6-88b7-01610d80c32c").value()}, /* ImportPrintRiteHDF5File */
    {Uuid::FromString("{38fde424-0292-5c42-b3b4-18d80c95524d}").value(), Uuid::FromString("38fde424-0292-5c42-b3b4-18d80c95524d").value()}, /* ImportPrintRiteTDMSFiles */
    {Uuid::FromString("{14f85d76-2400-57b8-9650-438563a8b8eb}").value(), Uuid::FromString("14f85d76-2400-57b8-9650-438563a8b8eb").value()}, /* ImportQMMeltpoolH5File */
    {Uuid::FromString("{60b75e1c-4c65-5d86-8cb0-8b8086193d23}").value(), Uuid::FromString("60b75e1c-4c65-5d86-8cb0-8b8086193d23").value()}, /* ImportQMMeltpoolTDMSFile */
    {Uuid::FromString("{5fa10d81-94b4-582b-833f-8eabe659069e}").value(), Uuid::FromString("5fa10d81-94b4-582b-833f-8eabe659069e").value()}, /* ImportVolumeGraphicsFile */
    {Uuid::FromString("{fd6da27d-0f2c-5c35-802f-7d6ce1ad0ca1}").value(), Uuid::FromString("fd6da27d-0f2c-5c35-802f-7d6ce1ad0ca1").value()}, /* InsertTransformationPhases */
    {Uuid::FromString("{4b551c15-418d-5081-be3f-d3aeb62408e5}").value(), Uuid::FromString("4b551c15-418d-5081-be3f-d3aeb62408e5").value()}, /* InterpolatePointCloudToRegularGrid */
    {Uuid::FromString("{6c8fb24b-5b12-551c-ba6d-ae2fa7724764}").value(), Uuid::FromString("6c8fb24b-5b12-551c-ba6d-ae2fa7724764").value()}, /* IterativeClosestPoint */
    {Uuid::FromString("{247ea39f-cb50-5dbb-bb9d-ecde12377fed}").value(), Uuid::FromString("247ea39f-cb50-5dbb-bb9d-ecde12377fed").value()}, /* KDistanceGraph */
    {Uuid::FromString("{b56a04de-0ca0-509d-809f-52219fca9c98}").value(), Uuid::FromString("b56a04de-0ca0-509d-809f-52219fca9c98").value()}, /* KMeans */
    {Uuid::FromString("{f7486aa6-3049-5be7-8511-ae772b70c90b}").value(), Uuid::FromString("f7486aa6-3049-5be7-8511-ae772b70c90b").value()}, /* KMedoids */
    {Uuid::FromString("{a250a228-3b6b-5b37-a6e4-8687484f04c4}").value(), Uuid::FromString("a250a228-3b6b-5b37-a6e4-8687484f04c4").value()}, /* LabelTriangleGeometry */
    {Uuid::FromString("{1cf52f08-a11a-5870-a38c-ea8958071bd8}").value(), Uuid::FromString("1cf52f08-a11a-5870-a38c-ea8958071bd8").value()}, /* LaplacianSmoothPointCloud */
    {Uuid::FromString("{620a3022-0f92-5d07-b725-b22604874bbf}").value(), Uuid::FromString("620a3022-0f92-5d07-b725-b22604874bbf").value()}, /* LocalDislocationDensityCalculator */
    {Uuid::FromString("{9fe34deb-99e1-5f3a-a9cc-e90c655b47ee}").value(), Uuid::FromString("9fe34deb-99e1-5f3a-a9cc-e90c655b47ee").value()}, /* MapPointCloudToRegularGrid */
    {Uuid::FromString("{8c584519-15c3-5010-b5ed-a2ac626591a1}").value(), Uuid::FromString("8c584519-15c3-5010-b5ed-a2ac626591a1").value()}, /* NormalizeArrays */
    {Uuid::FromString("{39652621-3cc4-5a72-92f3-e56c516d2b18}").value(), Uuid::FromString("39652621-3cc4-5a72-92f3-e56c516d2b18").value()}, /* ParaDisReader */
    {Uuid::FromString("{119861c5-e303-537e-b210-2e62936222e9}").value(), Uuid::FromString("119861c5-e303-537e-b210-2e62936222e9").value()}, /* PointSampleTriangleGeometry */
    {Uuid::FromString("{e15ec84b-1e02-53a6-a830-59e0813775a1}").value(), Uuid::FromString("e15ec84b-1e02-53a6-a830-59e0813775a1").value()}, /* PottsModel */
    {Uuid::FromString("{ec163736-39c8-5c69-9a56-61940a337c07}").value(), Uuid::FromString("ec163736-39c8-5c69-9a56-61940a337c07").value()}, /* PrincipalComponentAnalysis */
    {Uuid::FromString("{f2259481-5011-5f22-9fcb-c92fb6f8be10}").value(), Uuid::FromString("f2259481-5011-5f22-9fcb-c92fb6f8be10").value()}, /* ReadBinaryCTNorthStar */
    {Uuid::FromString("{5a2b714e-bae9-5213-8171-d1e190609e2d}").value(), Uuid::FromString("5a2b714e-bae9-5213-8171-d1e190609e2d").value()}, /* ReadMicData */
    {Uuid::FromString("{379ccc67-16dd-530a-984f-177db2314bac}").value(), Uuid::FromString("379ccc67-16dd-530a-984f-177db2314bac").value()}, /* RemoveFlaggedVertices */
    {Uuid::FromString("{3062fc2c-76b2-5c50-92b7-edbbb424c41d}").value(), Uuid::FromString("3062fc2c-76b2-5c50-92b7-edbbb424c41d").value()}, /* RobustAutomaticThreshold */
    {Uuid::FromString("{f84d4d69-9ea5-54b6-a71c-df76d76d50cf}").value(), Uuid::FromString("f84d4d69-9ea5-54b6-a71c-df76d76d50cf").value()}, /* Silhouette */
    {Uuid::FromString("{222307a4-67fd-5cb5-a12e-d80f9fb970ae}").value(), Uuid::FromString("222307a4-67fd-5cb5-a12e-d80f9fb970ae").value()}, /* SliceTriangleGeometry */
    {Uuid::FromString("{07b1048e-d6d4-56d0-8cc5-132ac79bdf60}").value(), Uuid::FromString("07b1048e-d6d4-56d0-8cc5-132ac79bdf60").value()}, /* SteinerCompact */
    {Uuid::FromString("{6f1abe57-ca7b-57ce-b03a-8c6f06fdc633}").value(), Uuid::FromString("6f1abe57-ca7b-57ce-b03a-8c6f06fdc633").value()}, /* TesselateFarFieldGrains */
    {Uuid::FromString("{81e94b15-efb6-5bae-9ab1-c74099136174}").value(), Uuid::FromString("81e94b15-efb6-5bae-9ab1-c74099136174").value()}, /* TiDwellFatigueCrystallographicAnalysis */
    {Uuid::FromString("{5d4c38fe-af0a-5cb3-b7fa-fb9e57b2097f}").value(), Uuid::FromString("5d4c38fe-af0a-5cb3-b7fa-fb9e57b2097f").value()}, /* WaveFrontObjectFileWriter */
};
// Plugin Uuid
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("302815ff-93a3-55c4-85b8-99b05690e2e6");
} // namespace

DREAM3DReviewPlugin::DREAM3DReviewPlugin()
: AbstractPlugin(k_ID, "DREAM3DReview", "<<--Description was not read-->>", "Open-Source")
{
  registerFilters();
}

DREAM3DReviewPlugin::~DREAM3DReviewPlugin() = default;

std::vector<complex::H5::IDataFactory*> DREAM3DReviewPlugin::getDataFactories() const
{
  return {};
}

COMPLEX_DEF_PLUGIN(DREAM3DReviewPlugin)

// The below file is generated at CMake configure time. This is done because
// the cmake system knows what filters are being compiled. This saves the
// developer from having to upkeep these lists.
#include "DREAM3DReview/plugin_filter_registration.h"
