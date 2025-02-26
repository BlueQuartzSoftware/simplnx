#include "MatrixCalculator.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/ImageRotationUtilities.hpp"

#include <Eigen/Dense>

using namespace nx::core;

namespace
{
template <typename T>
Eigen::Matrix<T, 4, 4, Eigen::RowMajor> CreateEigenMatrix(const AbstractDataStore<T>& dataStore)
{
  Eigen::Matrix<T, 4, 4, Eigen::RowMajor> matrix;
  matrix.fill(0);
  matrix << dataStore[0], dataStore[1], dataStore[2], dataStore[3], dataStore[4], dataStore[5], dataStore[6], dataStore[7], dataStore[8], dataStore[9], dataStore[10], dataStore[11], dataStore[12],
      dataStore[13], dataStore[14], dataStore[15];

  return matrix;
}

struct MatrixOperationFunctor
{
  template <typename ScalarType>
  Result<> operator()(const IDataArray& array1, const IDataArray& array2, IDataArray& outputArray, ChoicesParameter::ValueType opIdx)
  {
    using MatrixType = Eigen::Matrix<ScalarType, 4, 4, Eigen::RowMajor>;
    using StoreType = AbstractDataStore<ScalarType>;
    const auto& array1StoreRef = array1.getIDataStoreRefAs<StoreType>();
    const auto& array2StoreRef = array2.getIDataStoreRefAs<StoreType>();

    auto eigenMatrix1 = CreateEigenMatrix<ScalarType>(array1StoreRef);
    auto eigenMatrix2 = CreateEigenMatrix<ScalarType>(array2StoreRef);

    MatrixType output;

    if(opIdx == matrix_calculator::constants::k_MultiplicationIdx)
    {
      output = eigenMatrix1 * eigenMatrix2;
    }
    else if(opIdx == matrix_calculator::constants::k_AdditionIdx)
    {
      output = eigenMatrix1 + eigenMatrix2;
    }
    else if(opIdx == matrix_calculator::constants::k_SubtractionIdx)
    {
      output = eigenMatrix1 - eigenMatrix2;
    }

    auto& dataStore = outputArray.getIDataStoreRefAs<StoreType>();

    dataStore[0] = output(0, 0);
    dataStore[1] = output(0, 1);
    dataStore[2] = output(0, 2);
    dataStore[3] = output(0, 3);
    dataStore[4] = output(1, 0);
    dataStore[5] = output(1, 1);
    dataStore[6] = output(1, 2);
    dataStore[7] = output(1, 3);
    dataStore[8] = output(2, 0);
    dataStore[9] = output(2, 1);
    dataStore[10] = output(2, 2);
    dataStore[11] = output(2, 3);
    dataStore[12] = output(3, 0);
    dataStore[13] = output(3, 1);
    dataStore[14] = output(3, 2);
    dataStore[15] = output(3, 3);

    return {};
  }
};

} // namespace

// -----------------------------------------------------------------------------
MatrixCalculator::MatrixCalculator(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, MatrixCalculatorInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
MatrixCalculator::~MatrixCalculator() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& MatrixCalculator::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> MatrixCalculator::operator()()
{

  auto& outputArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->OutputPath);
  const auto& array1Ref = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->SelectedPaths[0]);
  const auto& array2Ref = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->SelectedPaths[1]);

  if(array1Ref.getDataType() != array2Ref.getDataType())
  {
    return MakeErrorResult(-89750, "DataType mismatch");
  }

  ExecuteDataFunction(MatrixOperationFunctor{}, array1Ref.getDataType(), array1Ref, array2Ref, outputArray, m_InputValues->Operation);

  for(usize selectedArrayIdx = 2; selectedArrayIdx < m_InputValues->SelectedPaths.size(); selectedArrayIdx++)
  {

    const auto& arrayRef = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->SelectedPaths[selectedArrayIdx]);

    ExecuteDataFunction(MatrixOperationFunctor{}, outputArray.getDataType(), outputArray, arrayRef, outputArray, m_InputValues->Operation);
  }

  return {};
}
