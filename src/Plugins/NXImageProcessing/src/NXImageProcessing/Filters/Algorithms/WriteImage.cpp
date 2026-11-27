#include "WriteImage.hpp"

#include "WriteImageDirect.hpp"
#include "WriteImageScanline.hpp"

#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

using namespace nx::core;

WriteImage::WriteImage(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel, const WriteImageInputValues& inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(messageHandler)
{
}

Result<> WriteImage::operator()()
{
  const auto& imageArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues.image_data_array_path);
  const IDataArray* maskArrayPtr = m_InputValues.use_mask ? m_DataStructure.getDataAs<IDataArray>(m_InputValues.mask_array_path) : nullptr;
  return DispatchAlgorithm<WriteImageDirect, WriteImageScanline>({&imageArray, maskArrayPtr}, m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
}
