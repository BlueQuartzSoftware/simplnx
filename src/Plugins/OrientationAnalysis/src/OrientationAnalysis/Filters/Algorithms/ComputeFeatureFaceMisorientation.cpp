#include "ComputeFeatureFaceMisorientation.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <EbsdLib/Core/EbsdLibConstants.h>
#include <EbsdLib/LaueOps/LaueOps.h>

using LaueOpsShPtrType = std::shared_ptr<ebsdlib::LaueOps>;
using LaueOpsContainer = std::vector<LaueOpsShPtrType>;

using namespace nx::core;

struct Output
{
};

struct PartialOutput : Output
{
  Float32Array& colorsArray;
};

struct FullOutput : Output
{
  Float32Array& colorsArray;
  Float32Array& axisAngleArray;
};

template <class T>
concept OutT = std::is_base_of_v<Output, T> && !
std::is_same_v<Output, T>;

/**
 * @brief The CalculateFaceMisorientationColorsImpl class implements a threaded algorithm that computes the misorientation
 * colors for the given list of surface mesh labels
 */
template <OutT OutputT>
class CalculateFaceMisorientationColorsImpl
{
  const Int32Array& m_Labels;
  const Int32Array& m_Phases;
  const Float32Array& m_Quats;
  const UInt32Array& m_CrystalStructures;
  const std::atomic_bool& m_ShouldCancel;
  const OutputT& m_Output;
  LaueOpsContainer m_OrientationOps;

public:
  CalculateFaceMisorientationColorsImpl(const Int32Array& labels, const Int32Array& phases, const Float32Array& quats, const UInt32Array& crystalStructures, const std::atomic_bool& shouldCancel,
                                        const OutputT& output)
  : m_Labels(labels)
  , m_Phases(phases)
  , m_Quats(quats)
  , m_CrystalStructures(crystalStructures)
  , m_ShouldCancel(shouldCancel)
  , m_Output(output)
  {
    m_OrientationOps = ebsdlib::LaueOps::GetAllOrientationOps();
  }
  virtual ~CalculateFaceMisorientationColorsImpl() = default;

  void generate(const usize start, const usize end) const
  {
    int32 feature1 = 0, feature2 = 0, phase1 = 0, phase2 = 0;

    for(usize i = start; i < end; i++)
    {
      if(m_ShouldCancel)
      {
        return;
      }

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
        if((m_CrystalStructures[phase1] == ebsdlib::CrystalStructure::Hexagonal_High) || (m_CrystalStructures[phase1] == ebsdlib::CrystalStructure::Cubic_High))
        {
          float32 quat0 = m_Quats[feature1 * 4];
          float32 quat1 = m_Quats[feature1 * 4 + 1];
          float32 quat2 = m_Quats[feature1 * 4 + 2];
          float32 quat3 = m_Quats[feature1 * 4 + 3];
          ebsdlib::QuatD q1(quat0, quat1, quat2, quat3);
          quat0 = m_Quats[feature2 * 4];
          quat1 = m_Quats[feature2 * 4 + 1];
          quat2 = m_Quats[feature2 * 4 + 2];
          quat3 = m_Quats[feature2 * 4 + 3];
          ebsdlib::QuatD q2(quat0, quat1, quat2, quat3);
          ebsdlib::AxisAngleDType axisAngle = m_OrientationOps[m_CrystalStructures[phase1]]->calculateMisorientation(q1, q2);

          if constexpr(std::is_same_v<OutputT, FullOutput>)
          {
            m_Output.axisAngleArray[4 * i + 0] = axisAngle[0];
            m_Output.axisAngleArray[4 * i + 1] = axisAngle[1];
            m_Output.axisAngleArray[4 * i + 2] = axisAngle[2];
            m_Output.axisAngleArray[4 * i + 3] = axisAngle[3];
          }

          m_Output.colorsArray[3 * i + 0] = axisAngle[0] * (axisAngle[3] * nx::core::Constants::k_180OverPiD);
          m_Output.colorsArray[3 * i + 1] = axisAngle[1] * (axisAngle[3] * nx::core::Constants::k_180OverPiD);
          m_Output.colorsArray[3 * i + 2] = axisAngle[2] * (axisAngle[3] * nx::core::Constants::k_180OverPiD);
        }
      }
      else
      {
        if constexpr(std::is_same_v<OutputT, FullOutput>)
        {
          m_Output.axisAngleArray[4 * i + 0] = 0.0;
          m_Output.axisAngleArray[4 * i + 1] = 0.0;
          m_Output.axisAngleArray[4 * i + 2] = 0.0;
          m_Output.axisAngleArray[4 * i + 3] = 0.0;
        }

        m_Output.colorsArray[3 * i + 0] = 0;
        m_Output.colorsArray[3 * i + 1] = 0;
        m_Output.colorsArray[3 * i + 2] = 0;
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
  auto& faceMisorientationColors = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->surfaceMeshFaceMisorientationColorsArrayPath);
  const usize numTriangles = faceLabels.getNumberOfTuples();

  ParallelDataAlgorithm parallelTask;
  parallelTask.setRange(0, numTriangles);
  if(m_InputValues->storeAxisAngle)
  {
    auto& axisArray = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->axisAngleArrayPath);
    const FullOutput fullOutput{.colorsArray = faceMisorientationColors, .axisAngleArray = axisArray};
    parallelTask.execute(CalculateFaceMisorientationColorsImpl(faceLabels, phases, avgQuats, crystalStructures, m_ShouldCancel, fullOutput));
  }
  else
  {
    const PartialOutput partialOutput{.colorsArray = faceMisorientationColors};
    parallelTask.execute(CalculateFaceMisorientationColorsImpl(faceLabels, phases, avgQuats, crystalStructures, m_ShouldCancel, partialOutput));
  }

  return {};
}
