#include "RotateEulerRefFrame.hpp"

#include "complex/Common/Numbers.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/Utilities/Math/MatrixMath.hpp"
#include "complex/Utilities/ParallelDataAlgorithm.hpp"

#include "EbsdLib/Core/Orientation.hpp"
#include "EbsdLib/Core/OrientationTransformation.hpp"

using namespace complex;

namespace
{

/**
 * @brief The RotateEulerRefFrameImpl class implements a threaded algorithm that rotates an array of Euler
 * angles about the supplied axis-angle pair.
 */
class RotateEulerRefFrameImpl
{

public:
  RotateEulerRefFrameImpl(Float32Array& data, float rotAngle, std::vector<float>& rotAxis, const std::atomic_bool& shouldCancel)
  : m_CellEulerAngles(data)
  , angle(rotAngle)
  , axis(rotAxis)
  , m_ShouldCancel(shouldCancel)
  {
    axis[0] = rotAxis[0];
    axis[1] = rotAxis[1];
    axis[2] = rotAxis[2];
  }
  virtual ~RotateEulerRefFrameImpl() = default;

  void convert(size_t start, size_t end) const
  {
    float rotMat[3][3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
    OrientationTransformation::ax2om<OrientationF, OrientationF>(OrientationF(axis[0], axis[1], axis[2], angle)).toGMatrix(rotMat);

    float ea1 = 0, ea2 = 0, ea3 = 0;
    float g[3][3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
    float gNew[3][3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
    for(size_t i = start; i < end; i++)
    {
      ea1 = m_CellEulerAngles[3 * i + 0];
      ea2 = m_CellEulerAngles[3 * i + 1];
      ea3 = m_CellEulerAngles[3 * i + 2];
      OrientationTransformation::eu2om<OrientationF, OrientationF>(OrientationF(ea1, ea2, ea3)).toGMatrix(g);

      complex::MatrixMath::Multiply3x3with3x3(g, rotMat, gNew);
      complex::MatrixMath::Normalize3x3(gNew);

      OrientationF eu = OrientationTransformation::om2eu<OrientationF, OrientationF>(OrientationF(gNew));
      m_CellEulerAngles[3 * i] = eu[0];
      m_CellEulerAngles[3 * i + 1] = eu[1];
      m_CellEulerAngles[3 * i + 2] = eu[2];
    }
  }

  void operator()(const Range& range) const
  {
    convert(range.min(), range.max());
  }

private:
  Float32Array& m_CellEulerAngles;
  float angle;
  std::vector<float> axis;
  const std::atomic_bool& m_ShouldCancel;
};
} // namespace

// -----------------------------------------------------------------------------
RotateEulerRefFrame::RotateEulerRefFrame(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel)
: m_DataStructure(dataStructure)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
RotateEulerRefFrameInputValues* RotateEulerRefFrame::inputValues()
{
  return &m_InputValues;
}

// -----------------------------------------------------------------------------
RotateEulerRefFrame::~RotateEulerRefFrame() noexcept = default;

// -----------------------------------------------------------------------------
Result<> RotateEulerRefFrame::operator()()
{
  complex::Float32Array& eulerAngles = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues.eulerAngleDataPath);

  size_t m_TotalElements = eulerAngles.getNumberOfTuples();

  float rotAngle = m_InputValues.rotationAngle * complex::numbers::pi_v<float> / 180.0F;
  MatrixMath::Normalize3x1(m_InputValues.rotationAxis.data());

  // Allow data-based parallelization
  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, m_TotalElements);
  dataAlg.execute(RotateEulerRefFrameImpl(eulerAngles, rotAngle, m_InputValues.rotationAxis, m_ShouldCancel));
  return {};
}
