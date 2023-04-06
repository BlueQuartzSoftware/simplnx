#include "FindAvgCAxes.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/Utilities/ImageRotationUtilities.hpp"
#include "complex/Utilities/Math/MatrixMath.hpp"

#include "EbsdLib/Core/Orientation.hpp"
#include "EbsdLib/Core/OrientationTransformation.hpp"
#include "EbsdLib/Core/Quaternion.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
FindAvgCAxes::FindAvgCAxes(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindAvgCAxesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
FindAvgCAxes::~FindAvgCAxes() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& FindAvgCAxes::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> FindAvgCAxes::operator()()
{
  const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  const auto& quats = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->QuatsArrayPath);
  auto& avgCAxes = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->AvgCAxesArrayPath);

  const usize totalPoints = featureIds.getNumberOfTuples();
  const usize totalFeatures = avgCAxes.getNumberOfTuples();

  float32 g1[3][3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
  float32 g1t[3][3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
  float32 caxis[3] = {0.0f, 0.0f, 1.0f};
  float32 c1[3] = {0.0f, 0.0f, 0.0f};

  std::vector<int32> counter(totalFeatures, 0);

  float32 curCAxis[3] = {0.0f, 0.0f, 0.0f};
  usize index = 0;
  float32 w = 0.0f;

  for(usize i = 0; i < totalPoints; i++)
  {
    if(featureIds[i] > 0)
    {
      index = 3 * featureIds[i];
      const usize offsetIndex = i * 4;
      OrientationTransformation::qu2om<QuatF, OrientationF>({quats[offsetIndex], quats[offsetIndex + 1], quats[offsetIndex + 2], quats[offsetIndex + 3]}).toGMatrix(g1);

      // transpose the g matrices so when caxis is multiplied by it
      // it will give the sample direction that the caxis is along
      MatrixMath::Transpose3x3(g1, g1t);
      MatrixMath::Multiply3x3with3x1(g1t, caxis, c1);
      // normalize so that the magnitude is 1
      MatrixMath::Normalize3x1(c1);
      curCAxis[0] = avgCAxes[index] / counter[featureIds[i]];
      curCAxis[1] = avgCAxes[index + 1] / counter[featureIds[i]];
      curCAxis[2] = avgCAxes[index + 2] / counter[featureIds[i]];
      MatrixMath::Normalize3x1(curCAxis);
      w = ImageRotationUtilities::CosBetweenVectors(Eigen::Vector3f{c1[0], c1[1], c1[2]}, Eigen::Vector3f{curCAxis[0], curCAxis[1], curCAxis[2]});
      if(w < 0)
      {
        MatrixMath::Multiply3x1withConstant(c1, -1.0f);
      }
      counter[featureIds[i]]++;
      avgCAxes[index] += c1[0];
      avgCAxes[index + 1] += c1[1];
      avgCAxes[index + 2] += c1[2];
    }
  }

  for(size_t i = 1; i < totalFeatures; i++)
  {
    if(counter[i] == 0)
    {
      avgCAxes[3 * i] = 0;
      avgCAxes[3 * i + 1] = 0;
      avgCAxes[3 * i + 2] = 1;
    }
    else
    {
      avgCAxes[3 * i] /= counter[i];
      avgCAxes[3 * i + 1] /= counter[i];
      avgCAxes[3 * i + 2] /= counter[i];
    }
  }
  return {};
}
