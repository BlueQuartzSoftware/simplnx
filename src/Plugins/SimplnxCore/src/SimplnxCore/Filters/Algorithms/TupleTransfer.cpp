#include "TupleTransfer.hpp"

namespace nx::core
{

void AddTupleTransferInstance(DataStructure& dataStructure, const DataPath& selectedDataPath, const DataPath& createdDataPath,
                              std::vector<std::shared_ptr<AbstractTupleTransfer>>& tupleTransferFunctions)
{
  auto* inputDataArray = dataStructure.getDataAs<IDataArray>(selectedDataPath);

  if(TemplateHelpers::CanDynamicCast<Float32Array>()(inputDataArray))
  {
    tupleTransferFunctions.push_back(std::make_shared<TransferTuple<float>>(dataStructure, selectedDataPath, createdDataPath));
  }
  if(TemplateHelpers::CanDynamicCast<Float64Array>()(inputDataArray))
  {
    tupleTransferFunctions.push_back(std::make_shared<TransferTuple<double>>(dataStructure, selectedDataPath, createdDataPath));
  }
  if(TemplateHelpers::CanDynamicCast<Int8Array>()(inputDataArray))
  {
    tupleTransferFunctions.push_back(std::make_shared<TransferTuple<int8_t>>(dataStructure, selectedDataPath, createdDataPath));
  }
  if(TemplateHelpers::CanDynamicCast<UInt8Array>()(inputDataArray))
  {
    tupleTransferFunctions.push_back(std::make_shared<TransferTuple<uint8_t>>(dataStructure, selectedDataPath, createdDataPath));
  }
  if(TemplateHelpers::CanDynamicCast<Int16Array>()(inputDataArray))
  {
    tupleTransferFunctions.push_back(std::make_shared<TransferTuple<int16_t>>(dataStructure, selectedDataPath, createdDataPath));
  }
  if(TemplateHelpers::CanDynamicCast<UInt16Array>()(inputDataArray))
  {
    tupleTransferFunctions.push_back(std::make_shared<TransferTuple<uint16_t>>(dataStructure, selectedDataPath, createdDataPath));
  }
  if(TemplateHelpers::CanDynamicCast<Int32Array>()(inputDataArray))
  {
    tupleTransferFunctions.push_back(std::make_shared<TransferTuple<int32_t>>(dataStructure, selectedDataPath, createdDataPath));
  }
  if(TemplateHelpers::CanDynamicCast<UInt32Array>()(inputDataArray))
  {
    tupleTransferFunctions.push_back(std::make_shared<TransferTuple<uint32_t>>(dataStructure, selectedDataPath, createdDataPath));
  }
  if(TemplateHelpers::CanDynamicCast<Int64Array>()(inputDataArray))
  {
    tupleTransferFunctions.push_back(std::make_shared<TransferTuple<int64_t>>(dataStructure, selectedDataPath, createdDataPath));
  }
  if(TemplateHelpers::CanDynamicCast<UInt64Array>()(inputDataArray))
  {
    tupleTransferFunctions.push_back(std::make_shared<TransferTuple<uint64_t>>(dataStructure, selectedDataPath, createdDataPath));
  }
  if(TemplateHelpers::CanDynamicCast<BoolArray>()(inputDataArray))
  {
    tupleTransferFunctions.push_back(std::make_shared<TransferTuple<bool>>(dataStructure, selectedDataPath, createdDataPath));
  }
}

} // namespace nx::core
