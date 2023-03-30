#include "AppendImageGeometryZSlice.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/StringArray.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"
#include "complex/Utilities/DataGroupUtilities.hpp"
#include "complex/Utilities/ParallelAlgorithmUtilities.hpp"
#include "complex/Utilities/ParallelTaskAlgorithm.hpp"

using namespace complex;

namespace
{
template <class K>
void AppendData(K* inputArray, K* destArray, usize offset)
{
  for(usize i = 0; i < inputArray->getNumberOfTuples(); ++i)
  {
    auto value = (*inputArray)[i]; // make sure we are getting a copy not a ref
    (*destArray)[offset + i] = value;
  }
}

template<typename T>
class AppendImageGeom
{
public:
public:
  AppendImageGeom(IArray::ArrayType arrayType, IArray* inputCellArray, IArray* destCellArray, usize tupleOffset)
  : m_ArrayType(arrayType)
  ,m_InputCellArray(dynamic_cast<DataArray<T>*>(inputCellArray))
  , m_DestCellArray(dynamic_cast<DataArray<T>*>(destCellArray))
  , m_TupleOffset(tupleOffset)
  {
  }

  ~AppendImageGeom() = default;

  AppendImageGeom(const AppendImageGeom&) = default;
  AppendImageGeom(AppendImageGeom&&) noexcept = default;
  AppendImageGeom& operator=(const AppendImageGeom&) = delete;
  AppendImageGeom& operator=(AppendImageGeom&&) noexcept = delete;

  void operator()() const
  {
    const usize offset = m_TupleOffset * m_DestCellArray->getNumberOfComponents();
    if(m_ArrayType == IArray::ArrayType::NeighborListArray)
    {
      using nListT = NeighborList<T>;
      AppendData<nListT>(dynamic_cast<nListT*>(m_InputCellArray), dynamic_cast<nListT*>(m_DestCellArray), offset);
    }
    if(m_ArrayType == IArray::ArrayType::DataArray)
    {
      using dArrayT = DataArray<T>;
      AppendData<dArrayT>(dynamic_cast<dArrayT*>(m_InputCellArray), dynamic_cast<dArrayT*>(m_DestCellArray), offset);
    }
    if(m_ArrayType == IArray::ArrayType::StringArray)
    {
      AppendData<StringArray>(dynamic_cast<StringArray*>(m_InputCellArray), dynamic_cast<StringArray*>(m_DestCellArray), offset);
    }
  }

private:
  IArray::ArrayType m_ArrayType = IArray::ArrayType::Any;
  IArray* m_InputCellArray = nullptr;
  IArray* m_DestCellArray = nullptr;
  usize m_TupleOffset;
};

template<typename T>
class CombineImageGeom
{
public:
  CombineImageGeom(IArray::ArrayType arrayType, IArray* inputCellArray1, IArray* inputCellArray2, IArray* destCellArray)
  : m_ArrayType(arrayType)
  , m_InputCellArray1(inputCellArray1)
  , m_InputCellArray2(inputCellArray2)
  , m_DestCellArray(destCellArray)
  {
  }

  ~CombineImageGeom() = default;

  CombineImageGeom(const CombineImageGeom&) = default;
  CombineImageGeom(CombineImageGeom&&) noexcept = default;
  CombineImageGeom& operator=(const CombineImageGeom&) = delete;
  CombineImageGeom& operator=(CombineImageGeom&&) noexcept = delete;

  void operator()() const
  {
    if(m_ArrayType == IArray::ArrayType::NeighborListArray)
    {
      using nListT = NeighborList<T>;
      AppendData<nListT>(dynamic_cast<nListT*>(m_InputCellArray1), dynamic_cast<nListT*>(m_DestCellArray), 0);
      AppendData<NeighborList<T>>(dynamic_cast<nListT*>(m_InputCellArray2), dynamic_cast<nListT*>(m_DestCellArray), m_InputCellArray1->getSize());
    }
    if(m_ArrayType == IArray::ArrayType::DataArray)
    {
      using dArrayT = DataArray<T>;
      AppendData<dArrayT>(dynamic_cast<dArrayT*>(m_InputCellArray1), dynamic_cast<dArrayT*>(m_DestCellArray), 0);
      AppendData<dArrayT>(dynamic_cast<dArrayT*>(m_InputCellArray2), dynamic_cast<dArrayT*>(m_DestCellArray), m_InputCellArray1->getSize());
    }
    if(m_ArrayType == IArray::ArrayType::StringArray)
    {
      AppendData<StringArray>(dynamic_cast<StringArray*>(m_InputCellArray1), dynamic_cast<StringArray*>(m_DestCellArray), 0);
      AppendData<StringArray>(dynamic_cast<StringArray*>(m_InputCellArray2), dynamic_cast<StringArray*>(m_DestCellArray), m_InputCellArray1->getSize());
    }
  }

private:
  IArray::ArrayType m_ArrayType = IArray::ArrayType::Any;
  IArray* m_InputCellArray1 = nullptr;
  IArray* m_InputCellArray2 = nullptr;
  IArray* m_DestCellArray = nullptr;
};

template <bool UseCombine, bool UseAppend>
struct TemplateTypeOptions
{
  static inline constexpr bool UsingCombine = UseCombine;
  static inline constexpr bool UsingAppend = UseAppend;
};

using Combine = TemplateTypeOptions<true,false>;
using Append = TemplateTypeOptions<false,true>;

void RunAppendBoolAppend(IArray* inputCellArray, IArray* destCellArray, usize tupleOffset)
{
  using dArrayT = DataArray<bool>;
  const usize offset = tupleOffset * destCellArray->getNumberOfComponents();
  AppendData<dArrayT>(dynamic_cast<dArrayT*>(inputCellArray), dynamic_cast<dArrayT*>(destCellArray), offset);
}

void RunCombineBoolAppend(IArray* inputCellArray1, IArray* inputCellArray2, IArray* destCellArray)
{
  using dArrayT = DataArray<bool>;
  AppendData<dArrayT>(dynamic_cast<dArrayT*>(inputCellArray1), dynamic_cast<dArrayT*>(destCellArray), 0);
  AppendData<dArrayT>(dynamic_cast<dArrayT*>(inputCellArray2), dynamic_cast<dArrayT*>(destCellArray), inputCellArray1->getSize());
}

template<class TemplateTypeOptions = Combine, class ParallelRunnerT, class... ArgsT>
void RunParallelFunction(IArray::ArrayType arrayType, IArray* destArray, ParallelRunnerT&& runner, ArgsT&&... args)
{
  static_assert(!(TemplateTypeOptions::UsingCombine && TemplateTypeOptions::UsingAppend), "Cannot use both Append and Combine");
  static_assert(!(!TemplateTypeOptions::UsingCombine && !TemplateTypeOptions::UsingAppend), "Cannot have Append and Combine false");

  DataType dataType = DataType::int32;
  if(arrayType == IArray::ArrayType::NeighborListArray)
  {
    dataType = dynamic_cast<INeighborList*>(destArray)->getDataType();
  }
  if(arrayType == IArray::ArrayType::DataArray)
  {
    dataType = dynamic_cast<IDataArray*>(destArray)->getDataType();
    if(dataType == DataType::boolean)
    {
      if constexpr(TemplateTypeOptions::UsingCombine)
      {
        RunCombineBoolAppend(destArray, std::forward<ArgsT>(args)...);
      }
      if constexpr(TemplateTypeOptions::UsingAppend)
      {
        RunAppendBoolAppend(destArray, std::forward<ArgsT>(args)...);
      }
      return;
    }
  }

  if constexpr(TemplateTypeOptions::UsingCombine)
  {
    ExecuteParallelFunction<CombineImageGeom, NoBooleanType>(dataType, std::forward<ParallelRunnerT>(runner),arrayType, destArray, std::forward<ArgsT>(args)...);
  }
  if constexpr(TemplateTypeOptions::UsingAppend)
  {
    ExecuteParallelFunction<AppendImageGeom, NoBooleanType>(dataType, std::forward<ParallelRunnerT>(runner),arrayType, destArray, std::forward<ArgsT>(args)...);
  }
}
} // namespace

// -----------------------------------------------------------------------------
AppendImageGeometryZSlice::AppendImageGeometryZSlice(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                     AppendImageGeometryZSliceInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
AppendImageGeometryZSlice::~AppendImageGeometryZSlice() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& AppendImageGeometryZSlice::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> AppendImageGeometryZSlice::operator()()
{
  Result<> results = {};

  const auto& inputGeometry = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->InputGeometryPath);
  auto& destGeometry = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->DestinationGeometryPath);
  AttributeMatrix* destCellData = destGeometry.getCellData();
  const DataPath destCellDataPath = m_InputValues->DestinationGeometryPath.createChildPath(destCellData->getName());
  const DataPath inputCellDataPath = m_InputValues->InputGeometryPath.createChildPath(inputGeometry.getCellData()->getName());
  DataPath newCellDataPath = destCellDataPath;
  AttributeMatrix* newCellData = destCellData;
  SizeVec3 inputGeomDims = inputGeometry.getDimensions();
  SizeVec3 destGeomDims = destGeometry.getDimensions();

  if(m_InputValues->SaveAsNewGeometry)
  {
    newCellData = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->NewGeometryPath).getCellData();
    newCellDataPath = m_InputValues->NewGeometryPath.createChildPath(newCellData->getName());
  }
  else
  {
    // We are only appending in the Z direction
    destGeomDims[2] = destGeomDims[2] + inputGeomDims[2];
    destGeometry.setDimensions(destGeomDims);
    const std::vector<size_t> newDims = {destGeomDims[2], destGeomDims[1], destGeomDims[0]};
    ResizeAttributeMatrix(*destCellData, newDims);
  }
  const usize tupleOffset = destGeometry.getNumberOfCells();

  ParallelTaskAlgorithm taskRunner;
  for(const auto& [dataId, dataObject] : *newCellData)
  {
    if(getCancel())
    {
      return {};
    }

    const std::string name = dataObject->getName();
    if(m_DataStructure.getData(inputCellDataPath.createChildPath(name)) == nullptr)
    {
      results = MergeResults(
          results,
          MakeWarningVoidResult(
              -8213,
              fmt::format("Data object {} does not exist in the input geometry cell data attribute matrix. Cannot append data so the resulting data object will likely contain invalid data!", name)));
      continue;
    }

    auto* inputDataArray = m_DataStructure.getDataAs<IArray>(inputCellDataPath.createChildPath(name));
    auto* destDataArray = m_DataStructure.getDataAs<IArray>(destCellDataPath.createChildPath(name));
    auto* newDataArray = m_DataStructure.getDataAs<IArray>(newCellDataPath.createChildPath(name));
    if(destDataArray == nullptr || newDataArray == nullptr || inputDataArray == nullptr)
    {
      continue;
    }
    const IArray::ArrayType arrayType = destDataArray->getArrayType();

    if(m_InputValues->SaveAsNewGeometry)
    {
      m_MessageHandler(fmt::format("Combining data into array {}", newCellDataPath.createChildPath(name).toString()));
      RunParallelFunction<Combine>(arrayType, destDataArray, taskRunner, inputDataArray, newDataArray);
    }
    else
    {
      m_MessageHandler(fmt::format("Appending Data Array {}", inputCellDataPath.createChildPath(name).toString()));
      RunParallelFunction<Append>(arrayType, destDataArray, taskRunner, inputDataArray, newDataArray);
    }
  }
  taskRunner.wait(); // This will spill over if the number of DataArrays to process does not divide evenly by the number of threads.

  return results;
}
