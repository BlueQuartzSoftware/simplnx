#include "OrientationAnalysis/Filters/ComputeMDFFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Common/Constants.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <EbsdLib/LaueOps/LaueOps.h>
#include <EbsdLib/Orientation/AxisAngle.hpp>
#include <EbsdLib/Orientation/Euler.hpp>
#include <EbsdLib/Orientation/Quaternion.hpp>
#include <EbsdLib/Orientation/Rodrigues.hpp>

#include <cmath>

using namespace nx::core;

namespace
{
DataStructure CreateDataStructure()
{
  DataStructure dataStructure;

  auto* imageGeom = ImageGeom::Create(dataStructure, "ImageGeom");
  imageGeom->setDimensions({4, 4, 1});

  auto* cellAM = AttributeMatrix::Create(dataStructure, "Cell Data", {16}, imageGeom->getId());
  UnitTest::CreateTestDataArray<float32>(dataStructure, "EulerAngles", {16}, {3}, cellAM->getId());
  UnitTest::CreateTestDataArray<int32>(dataStructure, "Phases", {16}, {1}, cellAM->getId());
  UnitTest::CreateTestDataArray<int32>(dataStructure, "FeatureIds", {16}, {1}, cellAM->getId());

  auto* ensembleAM = AttributeMatrix::Create(dataStructure, "Cell Ensemble Data", {2}, imageGeom->getId());
  auto* crystalStructures = UnitTest::CreateTestDataArray<uint32>(dataStructure, "CrystalStructures", {2}, {1}, ensembleAM->getId());
  (*crystalStructures)[0] = 999;
  (*crystalStructures)[1] = 1;

  return dataStructure;
}

// Builds an 8x8x1 single-slice ImageGeom. When splitFeatures is true the left half (x < 4) is
// feature 1 with Euler {0,0,0} and the right half (x >= 4) is feature 2 with Euler {45deg,0,0}
// (a 45 degree rotation about the sample Z axis). When false every cell is a single feature with
// identical orientation, i.e. no feature boundaries exist.
DataStructure CreateBicrystalDataStructure(bool splitFeatures)
{
  DataStructure dataStructure;

  auto* imageGeom = ImageGeom::Create(dataStructure, "ImageGeom");
  imageGeom->setDimensions({8, 8, 1});

  const usize numCells = 64;
  auto* cellAM = AttributeMatrix::Create(dataStructure, "Cell Data", {numCells}, imageGeom->getId());
  auto* eulerAngles = UnitTest::CreateTestDataArray<float32>(dataStructure, "EulerAngles", {numCells}, {3}, cellAM->getId());
  auto* phases = UnitTest::CreateTestDataArray<int32>(dataStructure, "Phases", {numCells}, {1}, cellAM->getId());
  auto* featureIds = UnitTest::CreateTestDataArray<int32>(dataStructure, "FeatureIds", {numCells}, {1}, cellAM->getId());

  const auto fortyFiveDegrees = static_cast<float32>(45.0 * Constants::k_PiOver180D);
  for(usize y = 0; y < 8; y++)
  {
    for(usize x = 0; x < 8; x++)
    {
      const usize cellIndex = y * 8 + x;
      (*phases)[cellIndex] = 1;
      const bool secondFeature = splitFeatures && (x >= 4);
      (*featureIds)[cellIndex] = secondFeature ? 2 : 1;
      (*eulerAngles)[cellIndex * 3 + 0] = secondFeature ? fortyFiveDegrees : 0.0f;
      (*eulerAngles)[cellIndex * 3 + 1] = 0.0f;
      (*eulerAngles)[cellIndex * 3 + 2] = 0.0f;
    }
  }

  auto* ensembleAM = AttributeMatrix::Create(dataStructure, "Cell Ensemble Data", {2}, imageGeom->getId());
  auto* crystalStructures = UnitTest::CreateTestDataArray<uint32>(dataStructure, "CrystalStructures", {2}, {1}, ensembleAM->getId());
  (*crystalStructures)[0] = 999;
  (*crystalStructures)[1] = 1;

  return dataStructure;
}

// Builds an 8x8x1 single-slice bicrystal that is a Sigma3 annealing twin: the left half (x < 4)
// is feature 1 with a general orientation e1, and the right half is feature 2 whose orientation is
// chosen so the crystal-to-crystal misorientation (in EbsdLib's passive convention, q1 * conj(q2))
// is exactly 60 degrees about <111>. Feature 1 is deliberately NOT the identity: with a general e1
// the correct sample-frame misorientation q1*conj(q2) folds to 60/<111>, whereas the crystal-frame
// form conj(q1)*q2 folds to a DIFFERENT, non-60 disorientation. A test asserting a 60/<111> peak
// therefore fails for the crystal-frame form and passes only for the correct sample-frame form.
DataStructure CreateSigma3TwinDataStructure()
{
  DataStructure dataStructure;

  auto* imageGeom = ImageGeom::Create(dataStructure, "ImageGeom");
  imageGeom->setDimensions({8, 8, 1});

  const usize numCells = 64;
  auto* cellAM = AttributeMatrix::Create(dataStructure, "Cell Data", {numCells}, imageGeom->getId());
  auto* eulerAngles = UnitTest::CreateTestDataArray<float32>(dataStructure, "EulerAngles", {numCells}, {3}, cellAM->getId());
  auto* phases = UnitTest::CreateTestDataArray<int32>(dataStructure, "Phases", {numCells}, {1}, cellAM->getId());
  auto* featureIds = UnitTest::CreateTestDataArray<int32>(dataStructure, "FeatureIds", {numCells}, {1}, cellAM->getId());

  // Feature 1: a deliberately non-identity, non-symmetric orientation.
  const ebsdlib::QuatD quat1 = ebsdlib::EulerDType(0.3, 0.4, 0.5).toQuaternion();
  // Sigma3 twin misorientation: 60 degrees about [111].
  const double sixtyDegrees = 60.0 * Constants::k_PiOver180D;
  const double invSqrt3 = 1.0 / std::sqrt(3.0);
  const double s = std::sin(sixtyDegrees / 2.0);
  const ebsdlib::QuatD twinMiso(invSqrt3 * s, invSqrt3 * s, invSqrt3 * s, std::cos(sixtyDegrees / 2.0));
  // Choose feature-2 orientation q2 so that q1 * conj(q2) == twinMiso, i.e. q2 = conj(twinMiso) * q1.
  const ebsdlib::QuatD quat2 = twinMiso.conjugate() * quat1;
  const ebsdlib::EulerDType euler1 = ebsdlib::QuaternionDType(quat1).toEuler();
  const ebsdlib::EulerDType euler2 = ebsdlib::QuaternionDType(quat2).toEuler();

  for(usize y = 0; y < 8; y++)
  {
    for(usize x = 0; x < 8; x++)
    {
      const usize cellIndex = y * 8 + x;
      (*phases)[cellIndex] = 1;
      const bool secondFeature = (x >= 4);
      (*featureIds)[cellIndex] = secondFeature ? 2 : 1;
      const ebsdlib::EulerDType& e = secondFeature ? euler2 : euler1;
      (*eulerAngles)[cellIndex * 3 + 0] = static_cast<float32>(e[0]);
      (*eulerAngles)[cellIndex * 3 + 1] = static_cast<float32>(e[1]);
      (*eulerAngles)[cellIndex * 3 + 2] = static_cast<float32>(e[2]);
    }
  }

  auto* ensembleAM = AttributeMatrix::Create(dataStructure, "Cell Ensemble Data", {2}, imageGeom->getId());
  auto* crystalStructures = UnitTest::CreateTestDataArray<uint32>(dataStructure, "CrystalStructures", {2}, {1}, ensembleAM->getId());
  (*crystalStructures)[0] = 999;
  (*crystalStructures)[1] = 1;

  return dataStructure;
}

// Fills the given Arguments with the standard set of paths for the bicrystal DataStructure.
Arguments CreateBicrystalArgs(float32 halfwidth, int32 numCurvePoints)
{
  DataPath imageGeomPath({"ImageGeom"});
  DataPath cellDataPath = imageGeomPath.createChildPath("Cell Data");
  DataPath outputGroupPath({"MDF Data"});

  Arguments args;
  args.insertOrAssign(ComputeMDFFilter::k_ImageGeometryPath_Key, std::make_any<GeometrySelectionParameter::ValueType>(imageGeomPath));
  args.insertOrAssign(ComputeMDFFilter::k_CellEulerAnglesArrayPath_Key, std::make_any<ArraySelectionParameter::ValueType>(cellDataPath.createChildPath("EulerAngles")));
  args.insertOrAssign(ComputeMDFFilter::k_CellPhasesArrayPath_Key, std::make_any<ArraySelectionParameter::ValueType>(cellDataPath.createChildPath("Phases")));
  args.insertOrAssign(ComputeMDFFilter::k_FeatureIdsArrayPath_Key, std::make_any<ArraySelectionParameter::ValueType>(cellDataPath.createChildPath("FeatureIds")));
  args.insertOrAssign(ComputeMDFFilter::k_CrystalStructuresArrayPath_Key,
                      std::make_any<ArraySelectionParameter::ValueType>(imageGeomPath.createChildPath("Cell Ensemble Data").createChildPath("CrystalStructures")));
  args.insertOrAssign(ComputeMDFFilter::k_Halfwidth_Key, std::make_any<Float32Parameter::ValueType>(halfwidth));
  args.insertOrAssign(ComputeMDFFilter::k_NumCurvePoints_Key, std::make_any<Int32Parameter::ValueType>(numCurvePoints));
  args.insertOrAssign(ComputeMDFFilter::k_UseAreaWeights_Key, std::make_any<BoolParameter::ValueType>(false));
  args.insertOrAssign(ComputeMDFFilter::k_OutputGroupPath_Key, std::make_any<DataGroupCreationParameter::ValueType>(outputGroupPath));
  return args;
}
} // namespace

TEST_CASE("OrientationAnalysis::ComputeMDF: Preflight", "[OrientationAnalysis][ComputeMDF]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = CreateDataStructure();

  DataPath imageGeomPath({"ImageGeom"});
  DataPath eulerAnglesPath = imageGeomPath.createChildPath("Cell Data").createChildPath("EulerAngles");
  DataPath phasesPath = imageGeomPath.createChildPath("Cell Data").createChildPath("Phases");
  DataPath featureIdsPath = imageGeomPath.createChildPath("Cell Data").createChildPath("FeatureIds");
  DataPath crystalStructuresPath = imageGeomPath.createChildPath("Cell Ensemble Data").createChildPath("CrystalStructures");
  DataPath outputGroupPath({"MDF Data"});

  ComputeMDFFilter filter;
  Arguments args;

  args.insertOrAssign(ComputeMDFFilter::k_ImageGeometryPath_Key, std::make_any<GeometrySelectionParameter::ValueType>(imageGeomPath));
  args.insertOrAssign(ComputeMDFFilter::k_CellEulerAnglesArrayPath_Key, std::make_any<ArraySelectionParameter::ValueType>(eulerAnglesPath));
  args.insertOrAssign(ComputeMDFFilter::k_CellPhasesArrayPath_Key, std::make_any<ArraySelectionParameter::ValueType>(phasesPath));
  args.insertOrAssign(ComputeMDFFilter::k_FeatureIdsArrayPath_Key, std::make_any<ArraySelectionParameter::ValueType>(featureIdsPath));
  args.insertOrAssign(ComputeMDFFilter::k_CrystalStructuresArrayPath_Key, std::make_any<ArraySelectionParameter::ValueType>(crystalStructuresPath));
  args.insertOrAssign(ComputeMDFFilter::k_Halfwidth_Key, std::make_any<Float32Parameter::ValueType>(5.0f));
  args.insertOrAssign(ComputeMDFFilter::k_NumCurvePoints_Key, std::make_any<Int32Parameter::ValueType>(13));
  args.insertOrAssign(ComputeMDFFilter::k_UseAreaWeights_Key, std::make_any<BoolParameter::ValueType>(false));
  args.insertOrAssign(ComputeMDFFilter::k_OutputGroupPath_Key, std::make_any<DataGroupCreationParameter::ValueType>(outputGroupPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(outputGroupPath.createChildPath("Phase-1").createChildPath("MDF")));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(outputGroupPath.createChildPath("Phase-1").createChildPath("Angle Distribution").createChildPath("Angles")));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(outputGroupPath.createChildPath("Phase-1").createChildPath("Angle Distribution").createChildPath("MDF Density")));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(outputGroupPath.createChildPath("Phase-1").createChildPath("Angle Distribution").createChildPath("Random Density")));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeMDF: Cubic bicrystal", "[OrientationAnalysis][ComputeMDF]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = CreateBicrystalDataStructure(true);

  ComputeMDFFilter filter;
  Arguments args = CreateBicrystalArgs(10.0f, 200);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  DataPath outputGroupPath({"MDF Data"});
  DataPath phaseGroupPath = outputGroupPath.createChildPath("Phase-1");
  DataPath angleDistPath = phaseGroupPath.createChildPath("Angle Distribution");

  // Independently compute the MDF bin that EbsdLib assigns the 45deg@[001] misorientation using
  // the SAME quaternion convention as the algorithm (miso = q1 * q2.conjugate()).
  auto orientationOps = ebsdlib::LaueOps::GetAllOrientationOps();
  ebsdlib::LaueOps::Pointer cubicOps = orientationOps[1];
  const double fortyFiveDegrees = 45.0 * Constants::k_PiOver180D;
  ebsdlib::QuatD quat1 = ebsdlib::EulerDType(0.0, 0.0, 0.0).toQuaternion();
  ebsdlib::QuatD quat2 = ebsdlib::EulerDType(fortyFiveDegrees, 0.0, 0.0).toQuaternion();
  ebsdlib::QuatD misoQuat = quat1 * quat2.conjugate();
  const int expectedBin = cubicOps->getMisoBin(cubicOps->getMDFFZRod(misoQuat.toRodrigues()));
  const usize expectedMdfSize = cubicOps->getMDFSize();
  REQUIRE(expectedMdfSize == 5832);

  // 1. MDF array resized to the CubicOps MDF size.
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float64Array>(phaseGroupPath.createChildPath("MDF")));
  const auto& mdfArray = dataStructure.getDataRefAs<Float64Array>(phaseGroupPath.createChildPath("MDF"));
  REQUIRE(mdfArray.getNumberOfTuples() == expectedMdfSize);

  // 2. argmax(MDF) equals the independently computed bin.
  usize mdfArgMax = 0;
  float64 mdfMax = mdfArray[0];
  for(usize i = 1; i < mdfArray.getNumberOfTuples(); i++)
  {
    if(mdfArray[i] > mdfMax)
    {
      mdfMax = mdfArray[i];
      mdfArgMax = i;
    }
  }
  // The 18^3 = 5832 cell MDF grid covers the full Rodrigues cube, but many cells lie outside the
  // misorientation fundamental zone and fold (via getMDFFZRod, applied inside binCenter /
  // determineRodriguesVector) onto the SAME physical misorientation as an in-FZ cell. The KDE peak
  // therefore lands on whichever cell center folds closest to the input misorientation, which is
  // not necessarily the raw accumulation index. The physically meaningful check is that the peak
  // bin, folded to the FZ and re-binned, equals the canonical bin EbsdLib assigns the input
  // misorientation.
  double binCenterSeed[3] = {0.5, 0.5, 0.5};
  ebsdlib::RodriguesDType peakRod = cubicOps->determineRodriguesVector(binCenterSeed, static_cast<int>(mdfArgMax));
  const int canonicalPeakBin = cubicOps->getMisoBin(peakRod);
  REQUIRE(canonicalPeakBin == expectedBin);

  // 3. The measured "MDF Density" curve peaks near 45 degrees (Angles are in DEGREES).
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float64Array>(angleDistPath.createChildPath("Angles")));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float64Array>(angleDistPath.createChildPath("MDF Density")));
  const auto& anglesArray = dataStructure.getDataRefAs<Float64Array>(angleDistPath.createChildPath("Angles"));
  const auto& mdfDensityArray = dataStructure.getDataRefAs<Float64Array>(angleDistPath.createChildPath("MDF Density"));

  usize densityArgMax = 0;
  float64 densityMax = mdfDensityArray[0];
  for(usize i = 1; i < mdfDensityArray.getNumberOfTuples(); i++)
  {
    if(mdfDensityArray[i] > densityMax)
    {
      densityMax = mdfDensityArray[i];
      densityArgMax = i;
    }
  }
  const float64 curveStep = anglesArray[1] - anglesArray[0];
  REQUIRE(std::abs(anglesArray[densityArgMax] - 45.0) <= 2.0 * curveStep);

  // 4. The random (Mackenzie) reference distribution has unit mean.
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float64Array>(angleDistPath.createChildPath("Random Density")));
  const auto& randomDensityArray = dataStructure.getDataRefAs<Float64Array>(angleDistPath.createChildPath("Random Density"));
  float64 randomSum = 0.0;
  for(usize i = 0; i < randomDensityArray.getNumberOfTuples(); i++)
  {
    randomSum += randomDensityArray[i];
  }
  const float64 randomMean = randomSum / static_cast<float64>(randomDensityArray.getNumberOfTuples());
  REQUIRE(std::abs(randomMean - 1.0) < 1.0e-6);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Regression test for the misorientation-frame bug (topic/compute-mdf): ComputeMDF originally
// computed the crystal-frame misorientation conj(q1)*q2 instead of the sample-frame q1*conj(q2)
// that EbsdLib's passive convention (and LaueOps::calculateMisorientation) require. On a real
// Sigma3-twin-rich dataset (Ni Small_IN100) that bug produced an approximately-random MDF whose
// angle curve peaked at the ~45 degree Mackenzie maximum instead of the ~60 degree twin peak.
// This test builds an exact 60/<111> twin from a non-identity base orientation (so the two
// misorientation frames are NOT symmetry-equivalent) and asserts the MDF peaks at 60/<111> and
// the angle curve peaks near 60, clearly distinct from the ~45 degree random-reference peak.
TEST_CASE("OrientationAnalysis::ComputeMDF: Sigma3 twin peaks at 60/<111>", "[OrientationAnalysis][ComputeMDF]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = CreateSigma3TwinDataStructure();

  ComputeMDFFilter filter;
  Arguments args = CreateBicrystalArgs(10.0f, 200);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  DataPath outputGroupPath({"MDF Data"});
  DataPath phaseGroupPath = outputGroupPath.createChildPath("Phase-1");
  DataPath angleDistPath = phaseGroupPath.createChildPath("Angle Distribution");

  auto orientationOps = ebsdlib::LaueOps::GetAllOrientationOps();
  ebsdlib::LaueOps::Pointer cubicOps = orientationOps[1];

  // 1. The MDF array peak bin, folded to the fundamental zone, is a 60 degree / <111> misorientation.
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float64Array>(phaseGroupPath.createChildPath("MDF")));
  const auto& mdfArray = dataStructure.getDataRefAs<Float64Array>(phaseGroupPath.createChildPath("MDF"));
  usize mdfArgMax = 0;
  float64 mdfMax = mdfArray[0];
  for(usize i = 1; i < mdfArray.getNumberOfTuples(); i++)
  {
    if(mdfArray[i] > mdfMax)
    {
      mdfMax = mdfArray[i];
      mdfArgMax = i;
    }
  }
  double binCenterSeed[3] = {0.5, 0.5, 0.5};
  ebsdlib::RodriguesDType peakRod = cubicOps->determineRodriguesVector(binCenterSeed, static_cast<int>(mdfArgMax));
  ebsdlib::AxisAngleDType peakAxisAngle = peakRod.toAxisAngle();
  const double peakAngleDeg = peakAxisAngle[3] * Constants::k_180OverPiD;
  INFO("MDF peak angle (deg): " << peakAngleDeg << " axis (" << peakAxisAngle[0] << ", " << peakAxisAngle[1] << ", " << peakAxisAngle[2] << ")");
  // The ~5 degree MDF-bin gridify shifts the peak by a couple degrees; allow a modest band.
  REQUIRE(peakAngleDeg > 56.0);
  REQUIRE(peakAngleDeg < 63.0);
  // Axis is <111>: all three |components| ~ 1/sqrt(3) = 0.577.
  const double invSqrt3 = 1.0 / std::sqrt(3.0);
  REQUIRE(std::abs(std::abs(peakAxisAngle[0]) - invSqrt3) < 0.1);
  REQUIRE(std::abs(std::abs(peakAxisAngle[1]) - invSqrt3) < 0.1);
  REQUIRE(std::abs(std::abs(peakAxisAngle[2]) - invSqrt3) < 0.1);

  // 2. The measured angle-distribution curve peaks near 60 degrees, NOT at the ~45 degree
  //    Mackenzie (random-reference) maximum. This is the assertion that fails for the buggy
  //    crystal-frame misorientation (which yields an approximately-random, ~45 degree, curve).
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float64Array>(angleDistPath.createChildPath("Angles")));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float64Array>(angleDistPath.createChildPath("MDF Density")));
  const auto& anglesArray = dataStructure.getDataRefAs<Float64Array>(angleDistPath.createChildPath("Angles"));
  const auto& mdfDensityArray = dataStructure.getDataRefAs<Float64Array>(angleDistPath.createChildPath("MDF Density"));
  usize densityArgMax = 0;
  float64 densityMax = mdfDensityArray[0];
  for(usize i = 1; i < mdfDensityArray.getNumberOfTuples(); i++)
  {
    if(mdfDensityArray[i] > densityMax)
    {
      densityMax = mdfDensityArray[i];
      densityArgMax = i;
    }
  }
  INFO("angle-curve peak (deg): " << anglesArray[densityArgMax]);
  REQUIRE(anglesArray[densityArgMax] > 53.0);
  REQUIRE(anglesArray[densityArgMax] < 63.0);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Regression test for the crystal-structure guard seam: LaueOps::GetAllOrientationOps() returns
// 12 entries, but only indices 0..10 correspond to named CrystalStructure Laue classes
// (ebsdlib::CrystalStructure::Trigonal_High == 10); index 11 is a duplicate OrthoRhombicOps entry
// with no corresponding CrystalStructure enumerator. The original guard `structure >=
// orientationOps.size()` let a CrystalStructures value of 11 through, so ComputeMDF::operator()
// would build a MisorientationKDE for it and MisorientationKDE::computeAngleCurve would call
// random_angle_distribution::MaxMisorientationAngle(11), which hits `default: throw
// std::invalid_argument` and escapes executeImpl uncaught. This test asserts that both an
// out-of-range sentinel (999) and the index-11 seam value are rejected with a clean Result error
// instead of a thrown exception.
TEST_CASE("OrientationAnalysis::ComputeMDF: unsupported crystal structure returns clean error, not a throw", "[OrientationAnalysis][ComputeMDF]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = CreateDataStructure();

  DataPath crystalStructuresPath = DataPath({"ImageGeom"}).createChildPath("Cell Ensemble Data").createChildPath("CrystalStructures");
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt32Array>(crystalStructuresPath));
  auto& crystalStructuresRef = dataStructure.getDataRefAs<UInt32Array>(crystalStructuresPath);
  // Ensemble 0 stays 999 (the unused "invalid" phase, never dereferenced). Ensemble 1 is set to
  // 11, the duplicate-OrthoRhombicOps seam value described above.
  REQUIRE(crystalStructuresRef[0] == 999);
  crystalStructuresRef[1] = 11;

  ComputeMDFFilter filter;
  Arguments args = CreateBicrystalArgs(10.0f, 200);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors().size() == 1);
  REQUIRE(executeResult.result.errors()[0].code == -96510);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeMDF: no boundaries warns and zero-fills", "[OrientationAnalysis][ComputeMDF]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = CreateBicrystalDataStructure(false);

  ComputeMDFFilter filter;
  Arguments args = CreateBicrystalArgs(10.0f, 200);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execution must SUCCEED even though there are no feature boundaries.
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  DataPath outputGroupPath({"MDF Data"});
  DataPath phaseGroupPath = outputGroupPath.createChildPath("Phase-1");
  DataPath angleDistPath = phaseGroupPath.createChildPath("Angle Distribution");

  // MDF array is all zeros.
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float64Array>(phaseGroupPath.createChildPath("MDF")));
  const auto& mdfArray = dataStructure.getDataRefAs<Float64Array>(phaseGroupPath.createChildPath("MDF"));
  for(usize i = 0; i < mdfArray.getNumberOfTuples(); i++)
  {
    REQUIRE(mdfArray[i] == 0.0);
  }

  // Measured "MDF Density" is zero everywhere.
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float64Array>(angleDistPath.createChildPath("MDF Density")));
  const auto& mdfDensityArray = dataStructure.getDataRefAs<Float64Array>(angleDistPath.createChildPath("MDF Density"));
  for(usize i = 0; i < mdfDensityArray.getNumberOfTuples(); i++)
  {
    REQUIRE(mdfDensityArray[i] == 0.0);
  }

  // The random (Mackenzie) reference distribution is still populated with unit mean.
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float64Array>(angleDistPath.createChildPath("Random Density")));
  const auto& randomDensityArray = dataStructure.getDataRefAs<Float64Array>(angleDistPath.createChildPath("Random Density"));
  float64 randomSum = 0.0;
  for(usize i = 0; i < randomDensityArray.getNumberOfTuples(); i++)
  {
    randomSum += randomDensityArray[i];
  }
  const float64 randomMean = randomSum / static_cast<float64>(randomDensityArray.getNumberOfTuples());
  REQUIRE(std::abs(randomMean - 1.0) < 1.0e-6);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
