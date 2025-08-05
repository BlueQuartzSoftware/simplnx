#include "TupleTransfer.hpp"

namespace nx::core
{
void AddTupleTransferInstance(DataStructure& dataStructure, const DataPath& selectedDataPath, const DataPath& createdDataPath,
                              std::vector<std::shared_ptr<AbstractTupleTransfer>>& tupleTransferFunctions)
{
  auto* inputDataArray = dataStructure.getDataAs<IDataArray>(selectedDataPath);

  switch(inputDataArray->getDataType())
  {
  case DataType::int8: {
    tupleTransferFunctions.push_back(std::make_shared<TransferTuple<int8>>(dataStructure, selectedDataPath, createdDataPath));
    break;
  }
  case DataType::uint8: {
    tupleTransferFunctions.push_back(std::make_shared<TransferTuple<uint8>>(dataStructure, selectedDataPath, createdDataPath));
    break;
  }
  case DataType::int16: {
    tupleTransferFunctions.push_back(std::make_shared<TransferTuple<int16>>(dataStructure, selectedDataPath, createdDataPath));
    break;
  }
  case DataType::uint16: {
    tupleTransferFunctions.push_back(std::make_shared<TransferTuple<uint16>>(dataStructure, selectedDataPath, createdDataPath));
    break;
  }
  case DataType::int32: {
    tupleTransferFunctions.push_back(std::make_shared<TransferTuple<int32>>(dataStructure, selectedDataPath, createdDataPath));
    break;
  }
  case DataType::uint32: {
    tupleTransferFunctions.push_back(std::make_shared<TransferTuple<uint32>>(dataStructure, selectedDataPath, createdDataPath));
    break;
  }
  case DataType::int64: {
    tupleTransferFunctions.push_back(std::make_shared<TransferTuple<int64>>(dataStructure, selectedDataPath, createdDataPath));
    break;
  }
  case DataType::uint64: {
    tupleTransferFunctions.push_back(std::make_shared<TransferTuple<uint64>>(dataStructure, selectedDataPath, createdDataPath));
    break;
  }
  case DataType::float32: {
    tupleTransferFunctions.push_back(std::make_shared<TransferTuple<float32>>(dataStructure, selectedDataPath, createdDataPath));
    break;
  }
  case DataType::float64: {
    tupleTransferFunctions.push_back(std::make_shared<TransferTuple<float64>>(dataStructure, selectedDataPath, createdDataPath));
    break;
  }
  case DataType::boolean: {
    tupleTransferFunctions.push_back(std::make_shared<TransferTuple<bool>>(dataStructure, selectedDataPath, createdDataPath));
    break;
  }
  }
}

void AddFeatureTupleTransferInstance(DataStructure& dataStructure, const DataPath& selectedDataPath, const DataPath& createdDataPath, const DataPath& featureIdsArrayPath,
                                     std::vector<std::shared_ptr<AbstractTupleTransfer>>& tupleTransferFunctions)
{
  auto* inputDataArray = dataStructure.getDataAs<IDataArray>(selectedDataPath);

  switch(inputDataArray->getDataType())
  {
  case DataType::int8: {
    tupleTransferFunctions.push_back(std::make_shared<TransferFeatureTuple<int8, int32>>(dataStructure, selectedDataPath, createdDataPath, featureIdsArrayPath));
    break;
  }
  case DataType::uint8: {
    tupleTransferFunctions.push_back(std::make_shared<TransferFeatureTuple<uint8, int32>>(dataStructure, selectedDataPath, createdDataPath, featureIdsArrayPath));
    break;
  }
  case DataType::int16: {
    tupleTransferFunctions.push_back(std::make_shared<TransferFeatureTuple<int16, int32>>(dataStructure, selectedDataPath, createdDataPath, featureIdsArrayPath));
    break;
  }
  case DataType::uint16: {
    tupleTransferFunctions.push_back(std::make_shared<TransferFeatureTuple<uint16, int32>>(dataStructure, selectedDataPath, createdDataPath, featureIdsArrayPath));
    break;
  }
  case DataType::int32: {
    tupleTransferFunctions.push_back(std::make_shared<TransferFeatureTuple<int32, int32>>(dataStructure, selectedDataPath, createdDataPath, featureIdsArrayPath));
    break;
  }
  case DataType::uint32: {
    tupleTransferFunctions.push_back(std::make_shared<TransferFeatureTuple<uint32, int32>>(dataStructure, selectedDataPath, createdDataPath, featureIdsArrayPath));
    break;
  }
  case DataType::int64: {
    tupleTransferFunctions.push_back(std::make_shared<TransferFeatureTuple<int64, int32>>(dataStructure, selectedDataPath, createdDataPath, featureIdsArrayPath));
    break;
  }
  case DataType::uint64: {
    tupleTransferFunctions.push_back(std::make_shared<TransferFeatureTuple<uint64, int32>>(dataStructure, selectedDataPath, createdDataPath, featureIdsArrayPath));
    break;
  }
  case DataType::float32: {
    tupleTransferFunctions.push_back(std::make_shared<TransferFeatureTuple<float32, int32>>(dataStructure, selectedDataPath, createdDataPath, featureIdsArrayPath));
    break;
  }
  case DataType::float64: {
    tupleTransferFunctions.push_back(std::make_shared<TransferFeatureTuple<float64, int32>>(dataStructure, selectedDataPath, createdDataPath, featureIdsArrayPath));
    break;
  }
  case DataType::boolean: {
    tupleTransferFunctions.push_back(std::make_shared<TransferFeatureTuple<bool, int32>>(dataStructure, selectedDataPath, createdDataPath, featureIdsArrayPath));
    break;
  }
  }
}

} // namespace nx::core
