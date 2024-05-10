#include "ComputeFeatureFaceMisorientation.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include "EbsdLib/Core/EbsdLibConstants.h"
#include "EbsdLib/LaueOps/LaueOps.h"

using LaueOpsShPtrType = std::shared_ptr<LaueOps>;
using LaueOpsContainer = std::vector<LaueOpsShPtrType>;

using namespace nx::core;

/**
 * @brief The CalculateFaceMisorientationColorsImpl class implements a threaded algorithm that computes the misorientation
 * colors for the given list of surface mesh labels
 */
class CalculateFaceMisorientationColorsImpl
{
  const Int32Array& m_Labels;
  const Int32Array& m_Phases;
  const Float32Array& m_Quats;
  const UInt32Array& m_CrystalStructures;
  Float32Array& m_Colors;
  LaueOpsContainer m_OrientationOps;

public:
  CalculateFaceMisorientationColorsImpl(const Int32Array& labels, const Int32Array& phases, const Float32Array& quats, const UInt32Array& crystalStructures, Float32Array& colors)
  : m_Labels(labels)
  , m_Phases(phases)
  , m_Quats(quats)
  , m_Colors(colors)
  , m_CrystalStructures(crystalStructures)
  {
    m_OrientationOps = LaueOps::GetAllOrientationOps();
  }
  virtual ~CalculateFaceMisorientationColorsImpl() = default;

  void generate(usize start, usize end) const
  {
    int32 feature1 = 0, feature2 = 0, phase1 = 0, phase2 = 0;

    for(usize i = start; i < end; i++)
    {
      feature1 = m_Labels[2 * i];
      feature2 = m_Labels[2 * i + 1];
      if(feature1 > 0)
      {
        phase1 = m_Phases[feature1];
      }
      else
      {
        phase1 = 0;
      }
      if(feature2 > 0)
      {
        phase2 = m_Phases[feature2];
      }
      else
      {
        phase2 = 0;
      }
      if(phase1 > 0 && phase1 == phase2)
      {
        if((m_CrystalStructures[phase1] == EbsdLib::CrystalStructure::Hexagonal_High) || (m_CrystalStructures[phase1] == EbsdLib::CrystalStructure::Cubic_High))
        {
          float32 quat0 = m_Quats[feature1 * 4];
          float32 quat1 = m_Quats[feature1 * 4 + 1];
          float32 quat2 = m_Quats[feature1 * 4 + 2];
          float32 quat3 = m_Quats[feature1 * 4 + 3];
          QuatD q1(quat0, quat1, quat2, quat3);
          quat0 = m_Quats[feature2 * 4];
          quat1 = m_Quats[feature2 * 4 + 1];
          quat2 = m_Quats[feature2 * 4 + 2];
          quat3 = m_Quats[feature2 * 4 + 3];
          QuatD q2(quat0, quat1, quat2, quat3);
          OrientationD axisAngle = m_OrientationOps[phase1]->calculateMisorientation(q1, q2);

          m_Colors[3 * i + 0] = axisAngle[0] * (axisAngle[3] * Constants::k_180OverPiD);
          m_Colors[3 * i + 1] = axisAngle[1] * (axisAngle[3] * Constants::k_180OverPiD);
          m_Colors[3 * i + 2] = axisAngle[2] * (axisAngle[3] * Constants::k_180OverPiD);
        }
      }
      else
      {
        m_Colors[3 * i + 0] = 0;
        m_Colors[3 * i + 1] = 0;
        m_Colors[3 * i + 2] = 0;
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
  auto& faceLabels = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->SurfaceMeshFaceLabelsArrayPath);
  auto& avgQuats = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->AvgQuatsArrayPath);
  auto& phases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeaturePhasesArrayPath);
  auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);
  DataPath faceMisorientationColorsArrayPath = m_InputValues->SurfaceMeshFaceLabelsArrayPath.replaceName(m_InputValues->SurfaceMeshFaceMisorientationColorsArrayName);
  auto& faceMisorientationColors = m_DataStructure.getDataRefAs<Float32Array>(faceMisorientationColorsArrayPath);
  int64 numTriangles = faceLabels.getNumberOfTuples();

  ParallelDataAlgorithm parallelTask;
  parallelTask.setRange(0, numTriangles);
  parallelTask.execute(CalculateFaceMisorientationColorsImpl(faceLabels, phases, avgQuats, crystalStructures, faceMisorientationColors));

  return {};
}
