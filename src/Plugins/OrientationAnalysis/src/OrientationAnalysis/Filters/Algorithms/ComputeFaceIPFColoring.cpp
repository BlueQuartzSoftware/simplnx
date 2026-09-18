#include "ComputeFaceIPFColoring.hpp"

#include "simplnx/Common/RgbColor.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <EbsdLib/Core/EbsdLibConstants.h>
#include <EbsdLib/LaueOps/CubicLowOps.h>
#include <EbsdLib/LaueOps/CubicOps.h>
#include <EbsdLib/LaueOps/HexagonalLowOps.h>
#include <EbsdLib/LaueOps/HexagonalOps.h>
#include <EbsdLib/LaueOps/LaueOps.h>
#include <EbsdLib/LaueOps/MonoclinicOps.h>
#include <EbsdLib/LaueOps/OrthoRhombicOps.h>
#include <EbsdLib/LaueOps/TetragonalLowOps.h>
#include <EbsdLib/LaueOps/TetragonalOps.h>
#include <EbsdLib/LaueOps/TriclinicOps.h>
#include <EbsdLib/LaueOps/TrigonalLowOps.h>
#include <EbsdLib/LaueOps/TrigonalOps.h>

#include <nonstd/span.hpp>

#include <algorithm>
#include <vector>

using namespace nx::core;

namespace
{
constexpr usize k_ValidationChunkTuples = 65536;

/**
 * @brief Validates positive Feature Phases referenced by either side of a face.
 * @param faceLabelsArrayRef Identifies the two Features adjacent to each face.
 * @param featurePhasesArrayRef Maps Feature indices to Phase indices.
 * @param crystalStructuresArrayRef Defines the valid Phase-index range.
 * @param inputValues Provides DataPaths for error diagnostics.
 * @return Success, or an error for invalid Phase indices or bulk I/O.
 */
Result<> ValidateReferencedFeaturePhases(const Int32Array& faceLabelsArrayRef, const Int32Array& featurePhasesArrayRef, const UInt32Array& crystalStructuresArrayRef,
                                         const ComputeFaceIPFColoringInputValues& inputValues)
{
  const usize numFeatures = featurePhasesArrayRef.getNumberOfTuples();
  std::vector<int32> featurePhasesCache(numFeatures);
  if(Result<> readResult = featurePhasesArrayRef.getDataStoreRef().copyIntoBuffer(0, nonstd::span<int32>(featurePhasesCache.data(), featurePhasesCache.size())); readResult.invalid())
  {
    return readResult;
  }

  const usize numCrystalStructures = crystalStructuresArrayRef.getNumberOfTuples();
  const auto& faceLabelsStoreRef = faceLabelsArrayRef.getDataStoreRef();
  const usize numFaces = faceLabelsArrayRef.getNumberOfTuples();
  std::vector<int32> faceLabelsBuffer(k_ValidationChunkTuples * 2);
  for(usize faceOffset = 0; faceOffset < numFaces; faceOffset += k_ValidationChunkTuples)
  {
    const usize faceCount = std::min(k_ValidationChunkTuples, numFaces - faceOffset);
    if(Result<> readResult = faceLabelsStoreRef.copyIntoBuffer(faceOffset * 2, nonstd::span<int32>(faceLabelsBuffer.data(), faceCount * 2)); readResult.invalid())
    {
      return readResult;
    }

    for(usize chunkFaceIdx = 0; chunkFaceIdx < faceCount; chunkFaceIdx++)
    {
      for(usize faceSideIdx = 0; faceSideIdx < 2; faceSideIdx++)
      {
        const int32 featureIdx = faceLabelsBuffer[chunkFaceIdx * 2 + faceSideIdx];
        if(featureIdx <= 0)
        {
          continue;
        }
        const int32 currentPhaseIdx = featurePhasesCache[featureIdx];
        if(currentPhaseIdx > 0 && static_cast<usize>(currentPhaseIdx) >= numCrystalStructures)
        {
          return MakeErrorResult(-24341, fmt::format("Feature Phases array '{}' has value {} at Feature index {}, referenced by face {} side {}, but Crystal Structures array '{}' contains {} tuples. "
                                                     "Valid positive Phase indices are in [1, {}).",
                                                     inputValues.FeaturePhasesArrayPath.toString(), currentPhaseIdx, featureIdx, faceOffset + chunkFaceIdx, faceSideIdx,
                                                     inputValues.CrystalStructuresArrayPath.toString(), numCrystalStructures, numCrystalStructures));
        }
      }
    }
  }
  return {};
}
} // namespace

/**
 * @class CalculateFaceIPFColorsImpl
 * @brief Calculates IPF colors for a contiguous range of surface-mesh faces.
 */
class CalculateFaceIPFColorsImpl
{
  const Int32Array& m_FaceLabels;
  const Int32Array& m_FeaturePhases;
  const Float64Array& m_FaceNormals;
  const Float32Array& m_FeatureEulerAngles;
  const UInt32Array& m_CrystalStructures;
  UInt8Array& m_FirstColors;
  UInt8Array& m_SecondColors;
  ebsdlib::ColorKeyKind m_ColorKey;

public:
  CalculateFaceIPFColorsImpl(const Int32Array& faceLabels, const Int32Array& featurePhases, const Float64Array& faceNormals, const Float32Array& featureEulerAngles,
                             const UInt32Array& crystalStructures, UInt8Array& firstColors, UInt8Array& secondColors, ebsdlib::ColorKeyKind colorKey)
  : m_FaceLabels(faceLabels)
  , m_FeaturePhases(featurePhases)
  , m_FaceNormals(faceNormals)
  , m_FeatureEulerAngles(featureEulerAngles)
  , m_CrystalStructures(crystalStructures)
  , m_FirstColors(firstColors)
  , m_SecondColors(secondColors)
  , m_ColorKey(colorKey)
  {
  }
  virtual ~CalculateFaceIPFColorsImpl() = default;

  void generate(usize start, usize end) const
  {
    const std::vector<ebsdlib::LaueOps::Pointer> orientationOps = ebsdlib::LaueOps::GetAllOrientationOps();

    double refDir[3] = {0.0, 0.0, 0.0};
    double dEuler[3] = {0.0, 0.0, 0.0};
    Rgba argb = 0x00000000;

    int32 firstFeatureIdx = 0;
    int32 secondFeatureIdx = 0;
    int32 firstFeaturePhaseIdx = 0;
    int32 secondFeaturePhaseIdx = 0;
    for(usize faceIdx = start; faceIdx < end; faceIdx++)
    {
      firstFeatureIdx = m_FaceLabels[2 * faceIdx];
      secondFeatureIdx = m_FaceLabels[2 * faceIdx + 1];
      if(firstFeatureIdx > 0)
      {
        firstFeaturePhaseIdx = m_FeaturePhases[firstFeatureIdx];
      }
      else
      {
        firstFeaturePhaseIdx = 0;
      }

      if(secondFeatureIdx > 0)
      {
        secondFeaturePhaseIdx = m_FeaturePhases[secondFeatureIdx];
      }
      else
      {
        secondFeaturePhaseIdx = 0;
      }

      if(firstFeaturePhaseIdx > 0)
      {
        const uint32 currentLaueIndex = m_CrystalStructures[firstFeaturePhaseIdx];
        if(currentLaueIndex < orientationOps.size())
        {
          dEuler[0] = m_FeatureEulerAngles[3 * firstFeatureIdx];
          dEuler[1] = m_FeatureEulerAngles[3 * firstFeatureIdx + 1];
          dEuler[2] = m_FeatureEulerAngles[3 * firstFeatureIdx + 2];
          refDir[0] = m_FaceNormals[3 * faceIdx];
          refDir[1] = m_FaceNormals[3 * faceIdx + 1];
          refDir[2] = m_FaceNormals[3 * faceIdx + 2];

          argb = orientationOps[currentLaueIndex]->generateIPFColor(dEuler, refDir, false, m_ColorKey);
          m_FirstColors[3 * faceIdx] = RgbColor::dRed(argb);
          m_FirstColors[3 * faceIdx + 1] = RgbColor::dGreen(argb);
          m_FirstColors[3 * faceIdx + 2] = RgbColor::dBlue(argb);
        }
      }
      else // A face side without a positive Phase receives black.
      {
        m_FirstColors[3 * faceIdx] = 0;
        m_FirstColors[3 * faceIdx + 1] = 0;
        m_FirstColors[3 * faceIdx + 2] = 0;
      }

      // The second face side uses the opposite normal direction.
      if(secondFeaturePhaseIdx > 0)
      {
        const uint32 currentLaueIndex = m_CrystalStructures[secondFeaturePhaseIdx];
        if(currentLaueIndex < orientationOps.size())
        {
          dEuler[0] = m_FeatureEulerAngles[3 * secondFeatureIdx];
          dEuler[1] = m_FeatureEulerAngles[3 * secondFeatureIdx + 1];
          dEuler[2] = m_FeatureEulerAngles[3 * secondFeatureIdx + 2];
          refDir[0] = -m_FaceNormals[3 * faceIdx];
          refDir[1] = -m_FaceNormals[3 * faceIdx + 1];
          refDir[2] = -m_FaceNormals[3 * faceIdx + 2];

          argb = orientationOps[currentLaueIndex]->generateIPFColor(dEuler, refDir, false, m_ColorKey);
          m_SecondColors[3 * faceIdx] = RgbColor::dRed(argb);
          m_SecondColors[3 * faceIdx + 1] = RgbColor::dGreen(argb);
          m_SecondColors[3 * faceIdx + 2] = RgbColor::dBlue(argb);
        }
      }
      else
      {
        m_SecondColors[3 * faceIdx] = 0;
        m_SecondColors[3 * faceIdx + 1] = 0;
        m_SecondColors[3 * faceIdx + 2] = 0;
      }
    }
  }

  /**
   * @brief Processes a face range through the parallel-algorithm interface.
   * @param range Identifies the half-open face range.
   */
  void operator()(const Range& range) const
  {
    generate(range.min(), range.max());
  }
};

// -----------------------------------------------------------------------------
ComputeFaceIPFColoring::ComputeFaceIPFColoring(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                               ComputeFaceIPFColoringInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeFaceIPFColoring::~ComputeFaceIPFColoring() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ComputeFaceIPFColoring::operator()()
{
  auto& faceLabelsArrayRef = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->SurfaceMeshFaceLabelsArrayPath);
  auto& faceNormalsArrayRef = m_DataStructure.getDataRefAs<Float64Array>(m_InputValues->SurfaceMeshFaceNormalsArrayPath);
  auto& featureEulerAnglesArrayRef = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->FeatureEulerAnglesArrayPath);
  auto& featurePhasesArrayRef = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeaturePhasesArrayPath);
  auto& crystalStructuresArrayRef = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);
  DataPath firstIpfColorsArrayPath = m_InputValues->SurfaceMeshFaceLabelsArrayPath.replaceName(m_InputValues->FirstFaceIPFColorsArrayName);
  auto& firstIpfColorsArrayRef = m_DataStructure.getDataRefAs<UInt8Array>(firstIpfColorsArrayPath);
  DataPath secondIpfColorsArrayPath = m_InputValues->SurfaceMeshFaceLabelsArrayPath.replaceName(m_InputValues->SecondFaceIPFColorsArrayName);
  auto& secondIpfColorsArrayRef = m_DataStructure.getDataRefAs<UInt8Array>(secondIpfColorsArrayPath);
  const int64 numTriangles = faceLabelsArrayRef.getNumberOfTuples();

  if(Result<> validationResult = ValidateReferencedFeaturePhases(faceLabelsArrayRef, featurePhasesArrayRef, crystalStructuresArrayRef, *m_InputValues); validationResult.invalid())
  {
    return validationResult;
  }

  typename IParallelAlgorithm::AlgorithmArrays algArrays;
  algArrays.push_back(&faceLabelsArrayRef);
  algArrays.push_back(&faceNormalsArrayRef);
  algArrays.push_back(&featureEulerAnglesArrayRef);
  algArrays.push_back(&featurePhasesArrayRef);
  algArrays.push_back(&crystalStructuresArrayRef);
  algArrays.push_back(&firstIpfColorsArrayRef);
  algArrays.push_back(&secondIpfColorsArrayRef);

  ParallelDataAlgorithm parallelTask;
  parallelTask.setRange(0, numTriangles);
  parallelTask.requireArraysInMemory(algArrays);
  // DataArray and DataStore access is not thread-safe, including distinct-index writes.
  // Keep parallelization disabled to match ComputeFeatureFaceMisorientation.
  parallelTask.setParallelizationEnabled(false);
  parallelTask.execute(CalculateFaceIPFColorsImpl(faceLabelsArrayRef, featurePhasesArrayRef, faceNormalsArrayRef, featureEulerAnglesArrayRef, crystalStructuresArrayRef, firstIpfColorsArrayRef,
                                                  secondIpfColorsArrayRef, m_InputValues->ColorKey));

  return {};
}
