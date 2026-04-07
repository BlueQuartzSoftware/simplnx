# ArrayCalculator Memory Optimization Design

## Problem

The ArrayCalculator evaluator allocates temporary `Float64Array` objects in a scratch `DataStructure` (`m_TempDataStructure`) for every input array, every intermediate result, and the final result. None of these temporaries are freed until the parser is destroyed. For a simple expression like `a + b` with float64 inputs and float64 output, this produces 4 array-sized allocations when 0 would suffice.

For large datasets with complex expressions, this causes memory usage to explode: an expression with N operations on an array of M elements allocates O(N * M * 8) bytes of temporaries that all stay alive simultaneously.

### Specific waste patterns

1. **Input float64 arrays are copied into temp Float64Arrays** even though the data is already float64 — a full O(M) allocation + O(M) copy per input reference.
2. **Every intermediate result allocates a new Float64Array** in `m_TempDataStructure`. None are freed until the parser destructs.
3. **The final result is copied element-by-element** from the temp Float64Array into the output DataArray, even when the output type is float64 — an unnecessary O(M) allocation + O(M) copy.

## Constraints

- **`m_TempDataStructure` must be retained.** Temporary arrays must live inside a `DataStructure` because the `DataArray`/`DataStore` abstraction is load-bearing: the project is moving to an out-of-core `DataStore` implementation where data may not be resident in memory. Raw `std::vector<float64>` or `float64*` buffers cannot replace `DataArray`.
- **No raw pointer access to DataArray data.** All element reads/writes must go through `DataArray::at()` or `operator[]`. Never use `T*` pointers obtained from `DataStore::data()` or similar.
- **Memory optimization is the first priority.** CPU computation performance is the second priority.
- **Backward compatibility.** All existing tests must continue to pass. Error codes, parameter keys, and pipeline JSON formats are unchanged by this work.

## Solution: CalcBuffer — RAII Sentinel for Temp DataArrays

### Overview

Introduce a `CalcBuffer` class: a move-only RAII handle that wraps a `Float64Array` (either a temp array in `m_TempDataStructure` or a borrowed reference to an input array in the real `DataStructure`). When an owned `CalcBuffer` is destroyed, it removes its `DataArray` from the `DataStructure` via `DataStructure::removeData(DataObject::IdType)`.

The evaluation stack changes from `std::stack<CalcValue>` (with manual ID lookups) to `std::stack<CalcBuffer>` (with automatic RAII cleanup). Intermediates are freed the instant they are consumed by an operator.

### CalcBuffer class

```cpp
class SIMPLNXCORE_EXPORT CalcBuffer
{
public:
  // --- Factory methods ---

  // Zero-copy reference to an existing Float64Array. Read-only. Destructor: no-op.
  static CalcBuffer borrow(const Float64Array& source);

  // Allocate a temp Float64Array in tempDS, convert source data from any numeric type.
  // Owned. Destructor: removes from tempDS.
  static CalcBuffer convertFrom(DataStructure& tempDS, const IDataArray& source,
                                const std::string& name);

  // Allocate a 1-element temp Float64Array with the given scalar value.
  static CalcBuffer scalar(DataStructure& tempDS, double value, const std::string& name);

  // Allocate an empty temp Float64Array with the given shape.
  static CalcBuffer allocate(DataStructure& tempDS, const std::string& name,
                             std::vector<usize> tupleShape, std::vector<usize> compShape);

  // Wrap the output DataArray<float64> for direct writing. Destructor: no-op.
  static CalcBuffer wrapOutput(DataArray<float64>& outputArray);

  // --- Move-only, non-copyable ---
  CalcBuffer(CalcBuffer&& other) noexcept;
  CalcBuffer& operator=(CalcBuffer&& other) noexcept;
  ~CalcBuffer();

  CalcBuffer(const CalcBuffer&) = delete;
  CalcBuffer& operator=(const CalcBuffer&) = delete;

  // --- Element access ---
  float64 read(usize index) const;
  void write(usize index, float64 value);
  void fill(float64 value);

  // --- Metadata ---
  usize size() const;
  usize numTuples() const;
  usize numComponents() const;
  std::vector<usize> tupleShape() const;
  std::vector<usize> compShape() const;
  bool isScalar() const;
  bool isOwned() const;
  bool isOutputDirect() const;

  // --- Access underlying array (for final copy to non-float64 output) ---
  const Float64Array& array() const;

private:
  CalcBuffer() = default;

  enum class Storage
  {
    Borrowed,      // const Float64Array* in real DataStructure, read-only
    Owned,         // Float64Array in m_TempDataStructure, read-write, cleaned up on destroy
    OutputDirect   // DataArray<float64>* output array, read-write, NOT cleaned up
  };

  Storage m_Storage = Storage::Owned;

  // Borrowed
  const Float64Array* m_BorrowedArray = nullptr;

  // Owned
  DataStructure* m_TempDS = nullptr;
  DataObject::IdType m_ArrayId = 0;
  Float64Array* m_OwnedArray = nullptr;

  // OutputDirect
  DataArray<float64>* m_OutputArray = nullptr;

  bool m_IsScalar = false;
};
```

#### Storage mode behavior

| Mode | `read(i)` | `write(i, v)` | Destructor |
|------|-----------|---------------|------------|
| Borrowed | `m_BorrowedArray->at(i)` | assert/error | no-op |
| Owned | `m_OwnedArray->at(i)` | `(*m_OwnedArray)[i] = v` | `m_TempDS->removeData(m_ArrayId)` |
| OutputDirect | `m_OutputArray->at(i)` | `(*m_OutputArray)[i] = v` | no-op |

#### Move semantics

The move constructor transfers all fields and nulls out the source so the moved-from object's destructor is a no-op:

```cpp
CalcBuffer::CalcBuffer(CalcBuffer&& other) noexcept
: m_Storage(other.m_Storage)
, m_BorrowedArray(other.m_BorrowedArray)
, m_TempDS(other.m_TempDS)
, m_ArrayId(other.m_ArrayId)
, m_OwnedArray(other.m_OwnedArray)
, m_OutputArray(other.m_OutputArray)
, m_IsScalar(other.m_IsScalar)
{
  other.m_TempDS = nullptr;  // prevents other's destructor from removing the array
  other.m_BorrowedArray = nullptr;
  other.m_OwnedArray = nullptr;
  other.m_OutputArray = nullptr;
}
```

### RPN item changes (data-free parser)

`CalcValue` is deleted. `RpnItem` stores metadata only — no `DataObject::IdType`, no data allocation during parsing.

```cpp
struct SIMPLNXCORE_EXPORT RpnItem
{
  enum class Type
  {
    Scalar,                // Numeric literal or constant (pi, e)
    ArrayRef,              // Reference to a source array in the real DataStructure
    Operator,              // Math operator or function
    ComponentExtract,      // [C] on a sub-expression result
    TupleComponentExtract  // [T, C] on a sub-expression result
  } type;

  // Scalar
  float64 scalarValue = 0.0;

  // ArrayRef
  DataPath arrayPath;
  DataType sourceDataType = DataType::float64;

  // Operator
  const OperatorDef* op = nullptr;

  // ComponentExtract / TupleComponentExtract
  usize componentIndex = std::numeric_limits<usize>::max();
  usize tupleIndex = std::numeric_limits<usize>::max();
};
```

### Parser changes

The parser becomes a pure validation + RPN construction pass with no data allocation:

1. **`createScalarInTemp()` eliminated** — parser stores `float64 scalarValue` in RpnItem.
2. **`copyArrayToTemp()` eliminated** — parser stores `DataPath` + `DataType` from the source.
3. **`Array[C]` bracket indexing unified with `(expr)[C]`** — parser emits `ArrayRef` + `ComponentExtract` RPN items instead of extracting component data during parsing.
4. **`Array[T,C]` bracket indexing unified with `(expr)[T,C]`** — parser emits `ArrayRef` + `TupleComponentExtract`.
5. **Validation step 7b** queries source array shapes directly via `dataStructure.getDataRefAs<IDataArray>(path)` instead of looking at temp arrays.
6. **`m_IsPreflight` flag eliminated** — parser is identical for preflight and execution.
7. **`m_TempDataStructure` removed from the parser** — it is created in the evaluator only.

### Evaluator changes

`m_TempDataStructure` becomes a local variable inside `evaluateInto()` (currently it is a member of `ArrayCalculatorParser`). Since the parser no longer needs it, the evaluator creates it on the stack and it is destroyed — along with any remaining temp arrays — when `evaluateInto()` returns. The eval stack is `std::stack<CalcBuffer>`.

#### Buffer creation by RPN type

| RPN Type | CalcBuffer creation |
|----------|-------------------|
| `Scalar` | `CalcBuffer::scalar(tempDS, value, name)` |
| `ArrayRef` where `sourceDataType == float64` | `CalcBuffer::borrow(sourceFloat64Array)` |
| `ArrayRef` where `sourceDataType != float64` | `CalcBuffer::convertFrom(tempDS, source, name)` |

#### RAII cleanup during operator evaluation

When processing a binary operator, operands are moved out of the stack into locals. After the result is computed and pushed, the locals are destroyed at the closing brace, triggering `removeData()` for any owned temps:

```cpp
{
  CalcBuffer right = std::move(evalStack.top());
  evalStack.pop();
  CalcBuffer left = std::move(evalStack.top());
  evalStack.pop();

  CalcBuffer result = CalcBuffer::allocate(tempDS, name, outTupleShape, outCompShape);

  bool leftIsScalar = left.isScalar();
  bool rightIsScalar = right.isScalar();

  for(usize i = 0; i < totalSize; i++)
  {
    float64 lv = left.read(leftIsScalar ? 0 : i);
    float64 rv = right.read(rightIsScalar ? 0 : i);
    result.write(i, op->binaryOp(lv, rv));
  }

  evalStack.push(std::move(result));
}
// left and right destroyed here -> owned temp arrays removed from tempDS
```

#### Last-operator direct-write optimization

For the last RPN operator, when the output type is `float64`:

1. Pre-scan `m_RpnItems` to find the index of the last Operator/ComponentExtract/TupleComponentExtract.
2. When processing that item, use `CalcBuffer::wrapOutput(outputFloat64Array)` instead of `CalcBuffer::allocate()`.
3. Writes go directly into the output array via `operator[]`.
4. At the end, detect `isOutputDirect()` on the final CalcBuffer and skip the copy step.

This eliminates the last temp array allocation entirely.

#### Final result copy to output

After the RPN loop, the stack has exactly one CalcBuffer:

Checked in order (first match wins):

| Final CalcBuffer | Output type | Action |
|-----------------|-------------|--------|
| isScalar() | any | Fill the output array with the scalar value via `DataArray::fill()` or equivalent loop |
| OutputDirect | float64 | No copy needed — data is already in the output |
| Owned or Borrowed | float64 | Element-by-element copy into output `DataArray<float64>` via `operator[]` (no type cast) |
| Owned or Borrowed | non-float64 | `CopyResultFunctor` cast-copy via `ExecuteDataFunction` |

### CPU performance considerations

- **`CalcBuffer::read()` branch on storage mode**: predictable per-CalcBuffer (same branch every call). Negligible cost compared to `std::function` dispatch through `op->binaryOp`/`op->unaryOp`.
- **Scalar broadcast check**: hoisted outside inner loops (`leftIsScalar`/`rightIsScalar` evaluated once before the loop).
- **OutputDirect adds a third branch to `write()`**: only applies to the single final-result CalcBuffer, so the branch predictor handles it trivially.
- **Borrowed reads via `Float64Array::at()` vs current temp array reads via `Float64Array::at()`**: identical per-element cost. Zero CPU regression for reads.

### Memory impact analysis

For expression `a + b + c + d + e + f` with float64 inputs and float64 output (M elements each):

| Metric | Current | New |
|--------|---------|-----|
| Input copies | 6 arrays (6 * M * 8 bytes) | 0 (all borrowed) |
| Peak intermediate arrays | 5 (all alive simultaneously) | 1 (RAII frees consumed) |
| Final result temp | 1 array (M * 8 bytes) | 0 (OutputDirect writes to output) |
| **Total temp memory** | **12 * M * 8 bytes** | **1 * M * 8 bytes** (one intermediate) |

For the same expression with non-float64 inputs (e.g., int32):

| Metric | Current | New |
|--------|---------|-----|
| Input copies | 6 arrays | 6 arrays (conversion required) |
| Peak intermediate arrays | 5 | 1 |
| Final result temp | 1 | 0 (if output is float64) or 1 (if not) |
| **Total temp memory** | **12 * M * 8 bytes** | **7 * M * 8 bytes** (6 conversions + 1 intermediate) |

### Files modified

- `ArrayCalculator.hpp` — add `CalcBuffer` class, update `RpnItem`, remove `CalcValue`, remove `m_TempDataStructure`/`m_IsPreflight`/`createScalarInTemp`/`copyArrayToTemp` from parser, add `m_TempDataStructure` to evaluator or create locally
- `ArrayCalculator.cpp` — rewrite `parse()` to be data-free, rewrite `evaluateInto()` with CalcBuffer stack + RAII + OutputDirect, remove `CopyToFloat64Functor` (moved into `CalcBuffer::convertFrom`), update `CopyResultFunctor` for the non-float64 output path
- `ArrayCalculatorFilter.cpp` — no changes expected (parser/evaluator API stays the same)
- `ArrayCalculatorTest.cpp` — no changes expected (tests exercise the filter, not internal classes)

### What is NOT changing

- `OperatorDef` struct and operator registry — unchanged
- `Token` struct and `tokenize()` — unchanged
- `CalculatorErrorCode` / `CalculatorWarningCode` enums — unchanged
- `ArrayCalculatorInputValues` struct — unchanged
- `ArrayCalculator` algorithm class public API — unchanged
- `ArrayCalculatorParser::parseAndValidate()` signature — unchanged
- `ArrayCalculatorParser::evaluateInto()` signature — unchanged
- Filter parameter keys, UUID, pipeline JSON format — unchanged
