#include "MergeColonies.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
MergeColonies::MergeColonies(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, MergeColoniesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
MergeColonies::~MergeColonies() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& MergeColonies::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> MergeColonies::operator()()
{
  /**
  * This section of the code should contain the actual algorithmic codes that
  * will accomplish the goal of the file.
  *
  * If you can parallelize the code there are a number of examples on how to do that.
  *    GenerateIPFColors is one example
  *
  * If you need to determine what kind of array you have (Int32Array, Float32Array, etc)
  * look to the ExecuteDataFunction() in complex/Utilities/FilterUtilities.hpp template 
  * function to help with that code.
  *   An Example algorithm class is `CombineAttributeArrays` and `RemoveFlaggedVertices`
  * 
  * There are other utility classes that can help alleviate the amount of code that needs
  * to be written.
  *
  * REMOVE THIS COMMENT BLOCK WHEN YOU ARE FINISHED WITH THE FILTER_HUMAN_NAME
  */

  return {};
}
