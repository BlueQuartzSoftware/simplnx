#include "RotateEulerRefFrame.hpp"

#include "OrientationAnalysis/utilities/OrientationUtilities.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Numbers.hpp"
#include "simplnx/DataStructure/DataArray.hpp"

#include <EbsdLib/Orientation/AxisAngle.hpp>
#include <EbsdLib/Orientation/OrientationFwd.hpp>
#include <EbsdLib/Orientation/OrientationMatrix.hpp>

using namespace nx::core;

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

  auto& eulerAngles = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->eulerAngleDataPath);
  auto& eulerStore = eulerAngles.getDataStoreRef();
  const usize totalTuples = eulerAngles.getNumberOfTuples();

  FloatVec3 axis = {m_InputValues->rotationAxis[0], m_InputValues->rotationAxis[1], m_InputValues->rotationAxis[2]};
  axis = axis.normalize();
  const float32 angle = m_InputValues->rotationAxis[3];

  ebsdlib::OrientationMatrixDType omRot = ebsdlib::AxisAngleDType(axis[0], axis[1], axis[2], angle * nx::core::numbers::pi / 180.0).toOrientationMatrix();
  OrientationUtilities::Matrix3dR rotMat = omRot.toEigenGMatrix();

  // Process in bounded chunks: read → rotate → write back
  constexpr usize k_ChunkTuples = 65536;
  std::vector<float32> buf(k_ChunkTuples * 3);

  for(usize startTup = 0; startTup < totalTuples; startTup += k_ChunkTuples)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    const usize count = std::min(k_ChunkTuples, totalTuples - startTup);
    eulerStore.copyIntoBuffer(startTup * 3, nonstd::span<float32>(buf.data(), count * 3));

    for(usize i = 0; i < count; i++)
    {
      ebsdlib::OrientationMatrixDType om = ebsdlib::EulerDType(buf[i * 3], buf[i * 3 + 1], buf[i * 3 + 2]).toOrientationMatrix();
      OrientationUtilities::Matrix3dR gNew = (om * rotMat).colwise().normalized();
      ebsdlib::EulerDType eu = ebsdlib::OrientationMatrixDType(gNew.data()).toEuler();
      buf[i * 3] = eu[0];
      buf[i * 3 + 1] = eu[1];
      buf[i * 3 + 2] = eu[2];
    }

    eulerStore.copyFromBuffer(startTup * 3, nonstd::span<const float32>(buf.data(), count * 3));
  }

  return {};
}

// -----------------------------------------------------------------------------
bool RotateEulerRefFrame::shouldCancel() const
{
  return m_ShouldCancel;
}
