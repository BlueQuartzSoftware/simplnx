#include "ComputeFeatureFaceMisorientation.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <EbsdLib/Core/EbsdLibConstants.h>
#include <EbsdLib/LaueOps/LaueOps.h>

using LaueOpsShPtrType = std::shared_ptr<ebsdlib::LaueOps>;
using LaueOpsContainer = std::vector<LaueOpsShPtrType>;

using namespace nx::core;

/**
 * @brief The CalculateFaceMisorientationColorsImpl class implements a threaded algorithm that computes the misorientation
 * colors for the given list of surface mesh labels
 */

class ComputeFeatureMisorientationPerTriangleImpl
{
  const Int32Array& m_FaceLabels;
  const Int32Array& m_FeaturePhases;
  const Float32Array& m_FeatureAvgQuats;
  const UInt32Array& m_CrystalStructures;
  const std::atomic_bool& m_ShouldCancel;
  Float32Array& m_Misorientations;
  LaueOpsContainer m_LaueOrientationOps;

public:
  ComputeFeatureMisorientationPerTriangleImpl(const Int32Array& labels, const Int32Array& phases, const Float32Array& quats, const UInt32Array& crystalStructures, const std::atomic_bool& shouldCancel,
                                              Float32Array& output)
  : m_FaceLabels(labels)
  , m_FeaturePhases(phases)
  , m_FeatureAvgQuats(quats)
  , m_CrystalStructures(crystalStructures)
  , m_ShouldCancel(shouldCancel)
  , m_Misorientations(output)
  {
    m_LaueOrientationOps = ebsdlib::LaueOps::GetAllOrientationOps();
  }
  virtual ~ComputeFeatureMisorientationPerTriangleImpl() = default;

  void generate(const usize start, const usize end) const
  {
    // Since our meshes use unified triangles, there are two triangles
    // per entry. These are distinguished via the face labels array,
    // which contains the feature id of each respective face. Here, the
    // first entry in face labels is denoted as "front" and the second "back"
    int32 frontFeature = 0, backFeature = 0, frontPhase = 0, backPhase = 0;

    for(usize i = start; i < end; i++)
    {
      if(m_ShouldCancel)
      {
        return;
      }

      frontFeature = m_FaceLabels[2 * i];
      backFeature = m_FaceLabels[2 * i + 1];
      if(frontFeature > 0)
      {
        frontPhase = m_FeaturePhases[frontFeature];
      }
      else
      {
        frontPhase = 0;
      }
      if(backFeature > 0)
      {
        backPhase = m_FeaturePhases[backFeature];
      }
      else
      {
        backPhase = 0;
      }
      if(frontPhase > 0 && frontPhase == backPhase)
      {
        uint32_t laueIndex = m_CrystalStructures[frontPhase];
        // Make sure the crystal structure is a valid laue class
        if(laueIndex < m_LaueOrientationOps.size())
        {
          float32 quat0 = m_FeatureAvgQuats[frontFeature * 4];
          float32 quat1 = m_FeatureAvgQuats[frontFeature * 4 + 1];
          float32 quat2 = m_FeatureAvgQuats[frontFeature * 4 + 2];
          float32 quat3 = m_FeatureAvgQuats[frontFeature * 4 + 3];
          ebsdlib::QuatD q1(quat0, quat1, quat2, quat3);
          quat0 = m_FeatureAvgQuats[backFeature * 4];
          quat1 = m_FeatureAvgQuats[backFeature * 4 + 1];
          quat2 = m_FeatureAvgQuats[backFeature * 4 + 2];
          quat3 = m_FeatureAvgQuats[backFeature * 4 + 3];
          ebsdlib::QuatD q2(quat0, quat1, quat2, quat3);
          ebsdlib::AxisAngleDType axisAngle = m_LaueOrientationOps[laueIndex]->calculateMisorientation(q1, q2);
          m_Misorientations.setValue(i, static_cast<float32>(axisAngle[3] * Constants::k_180OverPiD));
        }
      }
      else
      {
        m_Misorientations.setValue(i, static_cast<float>(std::nan("0")));
      }
    }
  }

  /**
   * @brief operator () This is called from the TBB stye of code
   * @param r The range to compute the values
   */
  void operator()(const Range& r) const
  {
    generate(r.min(), r.max());
  }
};

// -----------------------------------------------------------------------------
ComputeFeatureFaceMisorientation::ComputeFeatureFaceMisorientation(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                   ComputeFeatureFaceMisorientationInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeFeatureFaceMisorientation::~ComputeFeatureFaceMisorientation() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeFeatureFaceMisorientation::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ComputeFeatureFaceMisorientation::operator()()
{
  const auto& faceLabels = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->surfaceMeshFaceLabelsArrayPath);
  const auto& avgQuats = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->avgQuatsArrayPath);
  const auto& phases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->featurePhasesArrayPath);
  const auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->crystalStructuresArrayPath);
  auto& misorientations = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->misorientationArrayPath);
  const usize numTriangles = faceLabels.getNumberOfTuples();

  ParallelDataAlgorithm parallelTask;
  parallelTask.setRange(0, numTriangles);
  parallelTask.setParallelizationEnabled(false);
  parallelTask.execute(ComputeFeatureMisorientationPerTriangleImpl(faceLabels, phases, avgQuats, crystalStructures, m_ShouldCancel, misorientations));
  return {};
}
