#include "RotateEulerRefFrame.hpp"

#include "OrientationAnalysis/utilities/OrientationUtilities.hpp"

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/Math/MatrixMath.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include "EbsdLib/Core/Orientation.hpp"
#include "EbsdLib/Core/OrientationTransformation.hpp"

using namespace nx::core;

namespace
{

/**
 * @brief The RotateEulerRefFrameImpl class implements a threaded algorithm that rotates an array of Euler
 * angles about the supplied axis-angle pair.
 */
class RotateEulerRefFrameImpl
{

public:
  RotateEulerRefFrameImpl(Float32Array& data, std::vector<float>& rotAxis, float angle, const std::atomic_bool& shouldCancel, ProgressMessageHelper& progressMessageHelper)
  : m_CellEulerAngles(data)
  , m_AxisAngle(rotAxis)
  , m_Angle(angle)
  , m_ShouldCancel(shouldCancel)
  , m_ProgressMessageHelper(progressMessageHelper)
  {
  }
  virtual ~RotateEulerRefFrameImpl() = default;

  void convert(size_t start, size_t end) const
  {
    auto om = OrientationTransformation::ax2om<OrientationF, OrientationF>(OrientationF(m_AxisAngle[0], m_AxisAngle[1], m_AxisAngle[2], m_Angle * nx::core::numbers::pi / 180.0F));

    OrientationUtilities::Matrix3fR rotMat = OrientationUtilities::OrientationMatrixToGMatrix(om);

    ProgressMessenger progressMessenger = m_ProgressMessageHelper.createProgressMessenger();

    usize counter = 0;
    usize counterIncrement = (end - start) / 100;
    float ea1 = 0, ea2 = 0, ea3 = 0;
    for(size_t i = start; i < end; i++)
    {
      if(m_ShouldCancel)
      {
        return;
      }
      if(counter >= counterIncrement)
      {
        progressMessenger.sendProgressMessage(counter);
        counter = 0;
      }
      ea1 = m_CellEulerAngles[3 * i + 0];
      ea2 = m_CellEulerAngles[3 * i + 1];
      ea3 = m_CellEulerAngles[3 * i + 2];
      om = OrientationTransformation::eu2om<OrientationF, OrientationF>(OrientationF(ea1, ea2, ea3));
      OrientationUtilities::Matrix3fR g = OrientationUtilities::OrientationMatrixToGMatrix(om);
      OrientationUtilities::Matrix3fR gNew = (g * rotMat).colwise().normalized();

      auto eu = OrientationTransformation::om2eu<OrientationF, OrientationF>(OrientationF(gNew.data(), 9));
      m_CellEulerAngles[3 * i] = eu[0];
      m_CellEulerAngles[3 * i + 1] = eu[1];
      m_CellEulerAngles[3 * i + 2] = eu[2];
      counter++;
    }
    progressMessenger.sendProgressMessage(counter);
  }

  void operator()(const Range& range) const
  {
    convert(range.min(), range.max());
  }

private:
  Float32Array& m_CellEulerAngles;
  std::vector<float> m_AxisAngle;
  float m_Angle = 0.0F;
  const std::atomic_bool& m_ShouldCancel;
  ProgressMessageHelper& m_ProgressMessageHelper;
};
} // namespace

// -----------------------------------------------------------------------------
RotateEulerRefFrame::RotateEulerRefFrame(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RotateEulerRefFrameInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_MessageHandler(mesgHandler)
, m_ShouldCancel(shouldCancel)
, m_InputValues(inputValues)
{
}

// -----------------------------------------------------------------------------
RotateEulerRefFrame::~RotateEulerRefFrame() noexcept = default;

// -----------------------------------------------------------------------------
Result<> RotateEulerRefFrame::operator()()
{
  if(m_ShouldCancel)
  {
    return {};
  }

  nx::core::Float32Array& eulerAngles = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->eulerAngleDataPath);

  size_t totalElements = eulerAngles.getNumberOfTuples();

  std::vector<float> axis = {m_InputValues->rotationAxis[0], m_InputValues->rotationAxis[1], m_InputValues->rotationAxis[2]};
  MatrixMath::Normalize3x1(axis.data());

  MessageHelper messageHelper(m_MessageHandler);
  ProgressMessageHelper progressMessageHelper = messageHelper.createProgressMessageHelper();
  progressMessageHelper.setMaxProgresss(totalElements);
  progressMessageHelper.setProgressMessageTemplate("RotateEulerRefFrame: {:.2f}% complete");

  // Allow data-based parallelization
  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, totalElements);
  dataAlg.execute(RotateEulerRefFrameImpl(eulerAngles, axis, m_InputValues->rotationAxis[3], m_ShouldCancel, progressMessageHelper));
  return {};
}

bool RotateEulerRefFrame::shouldCancel() const
{
  return m_ShouldCancel;
}
