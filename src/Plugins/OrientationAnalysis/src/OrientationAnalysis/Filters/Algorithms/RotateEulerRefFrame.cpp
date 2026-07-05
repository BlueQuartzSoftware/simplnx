#include "RotateEulerRefFrame.hpp"

#include "OrientationAnalysis/utilities/OrientationUtilities.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Numbers.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <EbsdLib/Orientation/AxisAngle.hpp>
#include <EbsdLib/Orientation/OrientationFwd.hpp>
#include <EbsdLib/Orientation/OrientationMatrix.hpp>

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
  RotateEulerRefFrameImpl(Float32Array& data, const FloatVec3& rotAxis, float angle, const std::atomic_bool& shouldCancel, ProgressMessageHelper& progressMessageHelper)
  : m_CellEulerAngles(data)
  , m_RotationAxis(rotAxis)
  , m_Angle(angle)
  , m_ShouldCancel(shouldCancel)
  , m_ProgressMessageHelper(progressMessageHelper)
  {
  }
  ~RotateEulerRefFrameImpl() = default;

  void convert(size_t start, size_t end) const
  {
    // m_Angle arrives in degrees (user-facing parameter) while the Euler angle data is in
    // radians. The axis-angle pair produces the active rotation matrix R, so gNew = g * R
    // implements a passive rotation of the sample reference frame by +angle (right-hand rule).
    ebsdlib::OrientationMatrixDType om = ebsdlib::AxisAngleDType(m_RotationAxis[0], m_RotationAxis[1], m_RotationAxis[2], m_Angle * nx::core::numbers::pi / 180.0).toOrientationMatrix();

    OrientationUtilities::Matrix3dR rotMat = om.toEigenGMatrix();

    ProgressMessenger progressMessenger = m_ProgressMessageHelper.createProgressMessenger();

    usize counter = 0;
    usize counterIncrement = (end - start) / 100;
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

      om = ebsdlib::EulerDType(m_CellEulerAngles[3 * i + 0], m_CellEulerAngles[3 * i + 1], m_CellEulerAngles[3 * i + 2]).toOrientationMatrix();
      OrientationUtilities::Matrix3dR gNew = (om * rotMat).colwise().normalized();

      ebsdlib::EulerDType eu = ebsdlib::OrientationMatrixDType(gNew.data()).toEuler();
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
  FloatVec3 m_RotationAxis;
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

  nx::core::FloatVec3 axis = {m_InputValues->rotationAxis[0], m_InputValues->rotationAxis[1], m_InputValues->rotationAxis[2]};
  axis = axis.normalize();

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
