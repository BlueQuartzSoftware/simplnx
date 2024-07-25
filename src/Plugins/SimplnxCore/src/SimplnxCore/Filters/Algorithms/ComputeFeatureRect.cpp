#include "ComputeFeatureRect.hpp"

#include "simplnx/DataStructure/DataArray.hpp"

using namespace nx::core;

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
  const auto* featureIds = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  const auto& featureIdsStore = featureIds->getDataStoreRef();
  auto* corners = m_DataStructure.getDataAs<UInt32Array>(m_InputValues->FeatureRectArrayPath);
  auto& cornersStore = corners->getDataStoreRef();

  // Create corners array, which stores pixel coordinates for the top-left and bottom-right coordinates of each feature object
  for(usize i = 0; i < cornersStore.getNumberOfTuples(); i++)
  {
    cornersStore.setComponent(i, 0, std::numeric_limits<uint32>::max());
    cornersStore.setComponent(i, 1, std::numeric_limits<uint32>::max());
    cornersStore.setComponent(i, 2, std::numeric_limits<uint32>::max());
    cornersStore.setComponent(i, 3, std::numeric_limits<uint32>::min());
    cornersStore.setComponent(i, 4, std::numeric_limits<uint32>::min());
    cornersStore.setComponent(i, 5, std::numeric_limits<uint32>::min());
  }
  std::vector<usize> imageDims = featureIdsStore.getTupleShape();

  /*
   * Array dimension ordering is flipped compared to geometry dimension ordering.
   * We want the geometry dimension ordering, so we are flipping the indices below.
   */
  std::reverse(imageDims.rbegin(), imageDims.rend());

  const usize xDim = imageDims[0];
  const usize yDim = imageDims[1];
  const usize zDim = imageDims[2];

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

        const int32 featureId = featureIdsStore[index];
        if(featureId == 0)
        {
          continue;
        }

        if(featureId >= cornersStore.getNumberOfTuples())
        {
          const DataPath parentPath = m_InputValues->FeatureRectArrayPath.getParent();
          return MakeErrorResult(-31000, fmt::format("The parent data object '{}' of output array '{}' has a smaller tuple count than the maximum feature id in '{}'", parentPath.getTargetName(),
                                                     corners->getName(), featureIds->getName()));
        }

        const uint32 indices[3] = {x, y, z}; // Sequence dependent DO NOT REORDER
        const int32 featureShift = featureId * 6;
        for(uint8 l = 0; l < 6; l++) // unsigned is faster with modulo
        {
          if(l > 2)
          {
            cornersStore[featureShift + l] = std::max(cornersStore[featureShift + l], indices[l - 3]);
          }
          else
          {
            cornersStore[featureShift + l] = std::min(cornersStore[featureShift + l], indices[l]);
          }
        }
      }
    }
  }

  return {};
}
