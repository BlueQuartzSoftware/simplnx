#include "ComputeFeatureRect.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

namespace
{
// -----------------------------------------------------------------------------
usize IndexFromCoord(const std::vector<usize>& tDims, usize x, usize y, usize z)
{
  return (tDims[1] * tDims[0] * z) + (tDims[0] * y) + x;
}
} // namespace

// -----------------------------------------------------------------------------
ComputeFeatureRect::ComputeFeatureRect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeFeatureRectInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeFeatureRect::~ComputeFeatureRect() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeFeatureRect::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ComputeFeatureRect::operator()()
{
  const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  auto& corners = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->FeatureRectArrayPath);

  if(corners.getNumberOfTuples() == 0)
  {
    usize maxElement = static_cast<usize>(*std::max_element(featureIds.begin(), featureIds.end()));
    corners.reshapeTuples(std::vector<usize>{maxElement + 1});
  }

  auto& cornersDataStore = corners.getIDataStoreRefAs<UInt32DataStore>();

  // Create corners array, which stores pixel coordinates for the top-left and bottom-right coordinates of each feature object
  for(usize i = 0; i < corners.getNumberOfTuples(); i++)
  {
    cornersDataStore.setComponent(i, 0, std::numeric_limits<uint32>::max());
    cornersDataStore.setComponent(i, 1, std::numeric_limits<uint32>::max());
    cornersDataStore.setComponent(i, 2, std::numeric_limits<uint32>::max());
    cornersDataStore.setComponent(i, 3, std::numeric_limits<uint32>::min());
    cornersDataStore.setComponent(i, 4, std::numeric_limits<uint32>::min());
    cornersDataStore.setComponent(i, 5, std::numeric_limits<uint32>::min());
  }
  std::vector<usize> imageDims = featureIds.getTupleShape();
  usize xDim = imageDims[0], yDim = imageDims[1], zDim = imageDims[2];

  usize index = 0;
  // Store the coordinates in the corners array
  for(uint32 z = 0; z < zDim; z++)
  {
    if(getCancel())
    {
      return {};
    }

    for(uint32 y = 0; y < yDim; y++)
    {
      for(uint32 x = 0; x < xDim; x++)
      {
        index = IndexFromCoord(imageDims, x, y, z); // Index into featureIds array

        int32 featureId = featureIds[index];
        if(featureId == 0)
        {
          continue;
        }

        if(featureId >= corners.getNumberOfTuples())
        {
          DataPath parentPath = m_InputValues->FeatureRectArrayPath.getParent();
          return MakeErrorResult(-31000, fmt::format("The parent data object '{}' of output array '{}' has a smaller tuple count than the maximum feature id in '{}'", parentPath.getTargetName(),
                                                     corners.getName(), featureIds.getName()));
        }

        usize numComps = 6;
        usize featureIdStartIdx = featureId * numComps;
        uint32 val = corners[featureIdStartIdx + 0];
        if(x < corners[featureIdStartIdx + 0])
        {
          corners[featureIdStartIdx + 0] = x;
        }
        val = corners[featureIdStartIdx + 1];
        if(y < corners[featureIdStartIdx + 1])
        {
          corners[featureIdStartIdx + 1] = y;
        }
        val = corners[featureIdStartIdx + 2];
        if(z < corners[featureIdStartIdx + 2])
        {
          corners[featureIdStartIdx + 2] = z;
        }

        val = corners[featureIdStartIdx + 3];
        if(x > corners[featureIdStartIdx + 3])
        {
          corners[featureIdStartIdx + 3] = x;
        }
        val = corners[featureIdStartIdx + 4];
        if(y > corners[featureIdStartIdx + 4])
        {
          corners[featureIdStartIdx + 4] = y;
        }
        val = corners[featureIdStartIdx + 5];
        if(z > corners[featureIdStartIdx + 5])
        {
          corners[featureIdStartIdx + 5] = z;
        }
      }
    }
  }

  return {};
}
