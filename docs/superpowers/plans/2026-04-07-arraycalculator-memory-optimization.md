# ArrayCalculator Memory Optimization Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Eliminate unnecessary memory allocations in the ArrayCalculator evaluator by introducing a CalcBuffer RAII sentinel class, making the parser data-free, and enabling direct-write to the output array.

**Architecture:** CalcBuffer wraps Float64Array references (borrowed or owned) with automatic cleanup via DataStructure::removeData(). The parser produces data-free RPN items (DataPath + metadata, no DataObject allocation). The evaluator creates a local DataStructure for temp arrays, uses a CalcBuffer stack where intermediates are freed via RAII when consumed, and the last operator writes directly into the output DataArray when the output type is float64.

**Tech Stack:** C++20, simplnx DataStructure/DataArray/DataStore, Catch2 tests

**Design spec:** `docs/superpowers/specs/2026-04-07-arraycalculator-memory-optimization-design.md`

---

### Task 1: Establish Baseline — Verify All Existing Tests Pass

**Files:**
- Read: `src/Plugins/SimplnxCore/test/ArrayCalculatorTest.cpp`

This task ensures we have a clean starting point before making changes.

- [ ] **Step 1: Build the project**

```bash
cd /Users/mjackson/Workspace2/DREAM3D-Build/simplnx-Rel && cmake --build . --target SimplnxCore
```

Expected: Build succeeds with no errors.

- [ ] **Step 2: Run existing ArrayCalculator tests**

```bash
cd /Users/mjackson/Workspace2/DREAM3D-Build/simplnx-Rel && ctest -R "SimplnxCore::ArrayCalculatorFilter" --verbose
```

Expected: All test cases pass (Filter Execution, Tokenizer, Array Resolution, Built-in Constants, Modulo Operator, Tuple Component Indexing, Sub-expression Component Access, Multi-word Array Names, Sub-expression Tuple Component Extraction).

---

### Task 2: Add CalcBuffer Class Declaration to Header

**Files:**
- Modify: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ArrayCalculator.hpp`

Add the CalcBuffer class declaration. This task adds new code only — nothing is removed or changed yet.

- [ ] **Step 1: Add CalcBuffer class declaration after the OperatorDef struct (after line 133)**

Insert the following class declaration between the `OperatorDef` struct and the `CalcValue` struct (before line 138):

```cpp
// ---------------------------------------------------------------------------
// RAII sentinel for temporary Float64Arrays in the evaluator.
// Move-only. When an Owned CalcBuffer is destroyed, it removes its
// DataArray from the scratch DataStructure via removeData().
// ---------------------------------------------------------------------------
class SIMPLNXCORE_EXPORT CalcBuffer
{
public:
  // --- Factory methods ---

  /**
   * @brief Zero-copy reference to an existing Float64Array in the real DataStructure.
   * Read-only. Destructor: no-op.
   */
  static CalcBuffer borrow(const Float64Array& source);

  /**
   * @brief Allocate a temp Float64Array in tempDS and convert source data from any numeric type.
   * Owned. Destructor: removes the temp array from tempDS.
   */
  static CalcBuffer convertFrom(DataStructure& tempDS, const IDataArray& source, const std::string& name);

  /**
   * @brief Allocate a 1-element temp Float64Array with the given scalar value.
   * Owned. Destructor: removes the temp array from tempDS.
   */
  static CalcBuffer scalar(DataStructure& tempDS, float64 value, const std::string& name);

  /**
   * @brief Allocate an empty temp Float64Array with the given shape.
   * Owned. Destructor: removes the temp array from tempDS.
   */
  static CalcBuffer allocate(DataStructure& tempDS, const std::string& name, std::vector<usize> tupleShape, std::vector<usize> compShape);

  /**
   * @brief Wrap the output DataArray<float64> for direct writing.
   * Not owned. Destructor: no-op.
   */
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
  void markAsScalar();

  // --- Access underlying array (for final copy to non-float64 output) ---
  const Float64Array& array() const;

private:
  CalcBuffer() = default;

  enum class Storage
  {
    Borrowed,
    Owned,
    OutputDirect
  };

  Storage m_Storage = Storage::Owned;

  // Borrowed: const pointer to source Float64Array in real DataStructure
  const Float64Array* m_BorrowedArray = nullptr;

  // Owned: pointer to temp Float64Array + reference to its DataStructure for cleanup
  DataStructure* m_TempDS = nullptr;
  DataObject::IdType m_ArrayId = 0;
  Float64Array* m_OwnedArray = nullptr;

  // OutputDirect: writable pointer to output DataArray<float64>
  DataArray<float64>* m_OutputArray = nullptr;

  bool m_IsScalar = false;
};
```

- [ ] **Step 2: Build to verify the header compiles**

```bash
cd /Users/mjackson/Workspace2/DREAM3D-Build/simplnx-Rel && cmake --build . --target SimplnxCore
```

Expected: Build succeeds. CalcBuffer is declared but not yet defined — the linker won't complain because nothing references it yet.

- [ ] **Step 3: Commit**

```bash
git add src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ArrayCalculator.hpp
git commit -m "ENH: Add CalcBuffer RAII sentinel class declaration to ArrayCalculator.hpp"
```

---

### Task 3: Implement CalcBuffer in ArrayCalculator.cpp

**Files:**
- Modify: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ArrayCalculator.cpp`

Add the full CalcBuffer implementation. Place it after the anonymous namespace closing brace (after line 250) and before the `getOperatorRegistry()` function (line 255). This keeps it near the top of the file with other utility code.

- [ ] **Step 1: Add CalcBuffer implementation**

Insert the following after the anonymous namespace (after line 250, before `getOperatorRegistry()`):

```cpp
// ===========================================================================
// CalcBuffer implementation
// ===========================================================================

CalcBuffer::CalcBuffer(CalcBuffer&& other) noexcept
: m_Storage(other.m_Storage)
, m_BorrowedArray(other.m_BorrowedArray)
, m_TempDS(other.m_TempDS)
, m_ArrayId(other.m_ArrayId)
, m_OwnedArray(other.m_OwnedArray)
, m_OutputArray(other.m_OutputArray)
, m_IsScalar(other.m_IsScalar)
{
  other.m_TempDS = nullptr;
  other.m_BorrowedArray = nullptr;
  other.m_OwnedArray = nullptr;
  other.m_OutputArray = nullptr;
}

CalcBuffer& CalcBuffer::operator=(CalcBuffer&& other) noexcept
{
  if(this != &other)
  {
    // Clean up current state
    if(m_Storage == Storage::Owned && m_TempDS != nullptr)
    {
      m_TempDS->removeData(m_ArrayId);
    }

    m_Storage = other.m_Storage;
    m_BorrowedArray = other.m_BorrowedArray;
    m_TempDS = other.m_TempDS;
    m_ArrayId = other.m_ArrayId;
    m_OwnedArray = other.m_OwnedArray;
    m_OutputArray = other.m_OutputArray;
    m_IsScalar = other.m_IsScalar;

    other.m_TempDS = nullptr;
    other.m_BorrowedArray = nullptr;
    other.m_OwnedArray = nullptr;
    other.m_OutputArray = nullptr;
  }
  return *this;
}

CalcBuffer::~CalcBuffer()
{
  if(m_Storage == Storage::Owned && m_TempDS != nullptr)
  {
    m_TempDS->removeData(m_ArrayId);
  }
}

CalcBuffer CalcBuffer::borrow(const Float64Array& source)
{
  CalcBuffer buf;
  buf.m_Storage = Storage::Borrowed;
  buf.m_BorrowedArray = &source;
  buf.m_IsScalar = false;
  return buf;
}

CalcBuffer CalcBuffer::convertFrom(DataStructure& tempDS, const IDataArray& source, const std::string& name)
{
  std::vector<usize> tupleShape = source.getTupleShape();
  std::vector<usize> compShape = source.getComponentShape();
  Float64Array* destArr = Float64Array::CreateWithStore<Float64DataStore>(tempDS, name, tupleShape, compShape);

  usize totalElements = source.getSize();
  ExecuteDataFunction(CopyToFloat64Functor{}, source.getDataType(), source, *destArr);

  CalcBuffer buf;
  buf.m_Storage = Storage::Owned;
  buf.m_TempDS = &tempDS;
  buf.m_ArrayId = destArr->getId();
  buf.m_OwnedArray = destArr;
  buf.m_IsScalar = false;
  return buf;
}

CalcBuffer CalcBuffer::scalar(DataStructure& tempDS, float64 value, const std::string& name)
{
  Float64Array* arr = Float64Array::CreateWithStore<Float64DataStore>(tempDS, name, std::vector<usize>{1}, std::vector<usize>{1});
  (*arr)[0] = value;

  CalcBuffer buf;
  buf.m_Storage = Storage::Owned;
  buf.m_TempDS = &tempDS;
  buf.m_ArrayId = arr->getId();
  buf.m_OwnedArray = arr;
  buf.m_IsScalar = true;
  return buf;
}

CalcBuffer CalcBuffer::allocate(DataStructure& tempDS, const std::string& name, std::vector<usize> tupleShape, std::vector<usize> compShape)
{
  Float64Array* arr = Float64Array::CreateWithStore<Float64DataStore>(tempDS, name, tupleShape, compShape);

  CalcBuffer buf;
  buf.m_Storage = Storage::Owned;
  buf.m_TempDS = &tempDS;
  buf.m_ArrayId = arr->getId();
  buf.m_OwnedArray = arr;
  buf.m_IsScalar = false;
  return buf;
}

CalcBuffer CalcBuffer::wrapOutput(DataArray<float64>& outputArray)
{
  CalcBuffer buf;
  buf.m_Storage = Storage::OutputDirect;
  buf.m_OutputArray = &outputArray;
  buf.m_IsScalar = false;
  return buf;
}

float64 CalcBuffer::read(usize index) const
{
  switch(m_Storage)
  {
  case Storage::Borrowed:
    return m_BorrowedArray->at(index);
  case Storage::Owned:
    return m_OwnedArray->at(index);
  case Storage::OutputDirect:
    return m_OutputArray->at(index);
  }
  return 0.0;
}

void CalcBuffer::write(usize index, float64 value)
{
  switch(m_Storage)
  {
  case Storage::Owned:
    (*m_OwnedArray)[index] = value;
    return;
  case Storage::OutputDirect:
    (*m_OutputArray)[index] = value;
    return;
  case Storage::Borrowed:
    return; // read-only — should not be called
  }
}

void CalcBuffer::fill(float64 value)
{
  switch(m_Storage)
  {
  case Storage::Owned:
    m_OwnedArray->fill(value);
    return;
  case Storage::OutputDirect:
    m_OutputArray->fill(value);
    return;
  case Storage::Borrowed:
    return; // read-only
  }
}

usize CalcBuffer::size() const
{
  switch(m_Storage)
  {
  case Storage::Borrowed:
    return m_BorrowedArray->getSize();
  case Storage::Owned:
    return m_OwnedArray->getSize();
  case Storage::OutputDirect:
    return m_OutputArray->getSize();
  }
  return 0;
}

usize CalcBuffer::numTuples() const
{
  switch(m_Storage)
  {
  case Storage::Borrowed:
    return m_BorrowedArray->getNumberOfTuples();
  case Storage::Owned:
    return m_OwnedArray->getNumberOfTuples();
  case Storage::OutputDirect:
    return m_OutputArray->getNumberOfTuples();
  }
  return 0;
}

usize CalcBuffer::numComponents() const
{
  switch(m_Storage)
  {
  case Storage::Borrowed:
    return m_BorrowedArray->getNumberOfComponents();
  case Storage::Owned:
    return m_OwnedArray->getNumberOfComponents();
  case Storage::OutputDirect:
    return m_OutputArray->getNumberOfComponents();
  }
  return 0;
}

std::vector<usize> CalcBuffer::tupleShape() const
{
  switch(m_Storage)
  {
  case Storage::Borrowed:
    return m_BorrowedArray->getTupleShape();
  case Storage::Owned:
    return m_OwnedArray->getTupleShape();
  case Storage::OutputDirect:
    return m_OutputArray->getTupleShape();
  }
  return {};
}

std::vector<usize> CalcBuffer::compShape() const
{
  switch(m_Storage)
  {
  case Storage::Borrowed:
    return m_BorrowedArray->getComponentShape();
  case Storage::Owned:
    return m_OwnedArray->getComponentShape();
  case Storage::OutputDirect:
    return m_OutputArray->getComponentShape();
  }
  return {};
}

bool CalcBuffer::isScalar() const
{
  return m_IsScalar;
}

bool CalcBuffer::isOwned() const
{
  return m_Storage == Storage::Owned;
}

bool CalcBuffer::isOutputDirect() const
{
  return m_Storage == Storage::OutputDirect;
}

void CalcBuffer::markAsScalar()
{
  m_IsScalar = true;
}

const Float64Array& CalcBuffer::array() const
{
  switch(m_Storage)
  {
  case Storage::Borrowed:
    return *m_BorrowedArray;
  case Storage::Owned:
    return *m_OwnedArray;
  case Storage::OutputDirect:
    return *m_OutputArray;
  }
  // Should never reach here; return owned as fallback
  return *m_OwnedArray;
}
```

- [ ] **Step 2: Build to verify CalcBuffer compiles**

```bash
cd /Users/mjackson/Workspace2/DREAM3D-Build/simplnx-Rel && cmake --build . --target SimplnxCore
```

Expected: Build succeeds. CalcBuffer is implemented but not yet used.

- [ ] **Step 3: Run existing tests to verify no regressions**

```bash
cd /Users/mjackson/Workspace2/DREAM3D-Build/simplnx-Rel && ctest -R "SimplnxCore::ArrayCalculatorFilter" --verbose
```

Expected: All tests pass (no change in behavior).

- [ ] **Step 4: Commit**

```bash
git add src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ArrayCalculator.cpp
git commit -m "ENH: Implement CalcBuffer RAII sentinel for ArrayCalculator temp arrays"
```

---

### Task 4: Make ParsedItem and RpnItem Data-Free

**Files:**
- Modify: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ArrayCalculator.hpp` (RpnItem)
- Modify: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ArrayCalculator.cpp` (ParsedItem)

Replace CalcValue-based data in ParsedItem and RpnItem with metadata-only fields. This task changes the data structures but does not yet change the parser or evaluator logic — those come next.

- [ ] **Step 1: Update RpnItem in ArrayCalculator.hpp**

Replace the current `RpnItem` struct (lines 152-166) with the data-free version. Also delete the `CalcValue` struct (lines 138-147):

Delete `CalcValue`:
```cpp
// DELETE the entire CalcValue struct (lines 138-147):
// struct SIMPLNXCORE_EXPORT CalcValue { ... };
```

Replace `RpnItem`:
```cpp
// ---------------------------------------------------------------------------
// A single item in the RPN (reverse-polish notation) evaluation sequence.
// Data-free: stores DataPath references and scalar values, not DataObject IDs.
// ---------------------------------------------------------------------------
struct SIMPLNXCORE_EXPORT RpnItem
{
  enum class Type
  {
    Scalar,
    ArrayRef,
    Operator,
    ComponentExtract,
    TupleComponentExtract
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

- [ ] **Step 2: Update ParsedItem in the anonymous namespace of ArrayCalculator.cpp**

Replace the current `ParsedItem` struct (lines 26-44) with the data-free version:

```cpp
struct ParsedItem
{
  enum class Kind
  {
    Scalar,
    ArrayRef,
    Operator,
    LParen,
    RParen,
    Comma,
    ComponentExtract,
    TupleComponentExtract
  } kind;

  // Scalar
  float64 scalarValue = 0.0;

  // ArrayRef: metadata for validation (no data allocated)
  DataPath arrayPath;
  DataType sourceDataType = DataType::float64;
  std::vector<usize> arrayTupleShape;
  std::vector<usize> arrayCompShape;

  // Operator
  const OperatorDef* op = nullptr;
  bool isNegativePrefix = false;

  // ComponentExtract / TupleComponentExtract
  usize componentIndex = std::numeric_limits<usize>::max();
  usize tupleIndex = std::numeric_limits<usize>::max();
};
```

- [ ] **Step 3: Update isBinaryOp helper function**

The `isBinaryOp` function (around line 139-142) references `ParsedItem::Kind::Operator` which stays the same. No change needed to this function.

- [ ] **Step 4: Note — do NOT build yet**

The parser and evaluator still reference the old CalcValue and old ParsedItem/RpnItem fields. They must be updated in Tasks 5 and 6 before the code compiles. Proceed directly to Task 5.

---

### Task 5: Rewrite Parser to Be Data-Free

**Files:**
- Modify: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ArrayCalculator.hpp` (remove parser members)
- Modify: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ArrayCalculator.cpp` (rewrite parse())

This is the largest task. The parser's `parse()` method is rewritten so it allocates zero data — it only produces RPN items with DataPath/scalar metadata. The helper methods `createScalarInTemp()`, `copyArrayToTemp()`, and `nextScratchName()` are removed from the parser class. The `m_TempDataStructure`, `m_IsPreflight`, and `m_ScratchCounter` members are removed. The `getTempDataStructure()` method is removed.

- [ ] **Step 1: Remove parser-only members from ArrayCalculatorParser in the header**

In `ArrayCalculator.hpp`, remove the following from the `ArrayCalculatorParser` class:

Remove these private method declarations:
- `std::string nextScratchName();` (line 234)
- `DataObject::IdType copyArrayToTemp(const IDataArray& sourceArray);` (line 242)
- `DataObject::IdType createScalarInTemp(double value);` (line 249)

Remove these private member variables:
- `DataStructure m_TempDataStructure;` (line 252)
- `bool m_IsPreflight;` (line 255)
- `usize m_ScratchCounter = 0;` (line 256)

Remove this public method:
- `DataStructure& getTempDataStructure() { ... }` (lines 217-221)

Update the constructor signature — remove `bool isPreflight` parameter:
```cpp
ArrayCalculatorParser(const DataStructure& dataStructure, const DataPath& selectedGroupPath, const std::string& infixEquation, const std::atomic_bool& shouldCancel);
```

- [ ] **Step 2: Update ArrayCalculatorParser constructor implementation in .cpp**

Replace the constructor (lines 305-312) with:

```cpp
ArrayCalculatorParser::ArrayCalculatorParser(const DataStructure& dataStructure, const DataPath& selectedGroupPath, const std::string& infixEquation, const std::atomic_bool& shouldCancel)
: m_DataStructure(dataStructure)
, m_SelectedGroupPath(selectedGroupPath)
, m_InfixEquation(infixEquation)
, m_ShouldCancel(shouldCancel)
{
}
```

- [ ] **Step 3: Delete the old helper method implementations from .cpp**

Delete the following function bodies from ArrayCalculator.cpp:
- `ArrayCalculatorParser::nextScratchName()` (lines 443-446)
- `ArrayCalculatorParser::createScalarInTemp()` (lines 449-454)
- `ArrayCalculatorParser::copyArrayToTemp()` (lines 457-476)

- [ ] **Step 4: Rewrite parse() — token resolution (steps 3+4)**

This is the core change. In the token resolution loop (starting around line 595), every place that previously called `createScalarInTemp()` or `copyArrayToTemp()` must instead store metadata in the ParsedItem.

**For numeric literals** (the `TokenType::Number` case, around line 773):
Replace:
```cpp
DataObject::IdType id = createScalarInTemp(numValue);
ParsedItem pi;
pi.kind = ParsedItem::Kind::Value;
pi.value = CalcValue{CalcValue::Kind::Number, id};
```
With:
```cpp
ParsedItem pi;
pi.kind = ParsedItem::Kind::Scalar;
pi.scalarValue = numValue;
```

**For constants `pi` and `e`** (around line 841):
Replace:
```cpp
double constValue = (tok.text == "pi") ? std::numbers::pi : std::numbers::e;
DataObject::IdType id = createScalarInTemp(constValue);
ParsedItem pi;
pi.kind = ParsedItem::Kind::Value;
pi.value = CalcValue{CalcValue::Kind::Number, id};
```
With:
```cpp
float64 constValue = (tok.text == "pi") ? std::numbers::pi : std::numbers::e;
ParsedItem pi;
pi.kind = ParsedItem::Kind::Scalar;
pi.scalarValue = constValue;
```

**For array references found via selected group** (around line 876):
Replace:
```cpp
DataObject::IdType id = copyArrayToTemp(*dataArray);
ParsedItem pi;
pi.kind = ParsedItem::Kind::Value;
pi.value = CalcValue{CalcValue::Kind::Array, id};
```
With:
```cpp
ParsedItem pi;
pi.kind = ParsedItem::Kind::ArrayRef;
pi.arrayPath = arrayPath;
pi.sourceDataType = dataArray->getDataType();
pi.arrayTupleShape = dataArray->getTupleShape();
pi.arrayCompShape = dataArray->getComponentShape();
```

Apply the same pattern to **all other array resolution sites**:
- Array found via `findArraysByName()` (around line 916): same change — store DataPath, DataType, shapes
- Quoted string path resolution (around line 969): same change

**For bracket indexing `Array[C]`** (the block starting around line 635):

Currently this block accesses `m_TempDataStructure` to get the temp array and extract component data. Replace the entire bracket handling block. When `prevItem.kind == ParsedItem::Kind::ArrayRef`:

For `[C]` (single bracket number):
```cpp
usize numComponents = 1;
for(usize d : prevItem.arrayCompShape)
{
  numComponents *= d;
}

// Validate component index
if(compIdx >= numComponents)
{
  return MakeErrorResult(static_cast<int>(CalculatorErrorCode::ComponentOutOfRange),
    fmt::format("Component index {} is out of range for array with {} components.", compIdx, numComponents));
}

// Emit a ComponentExtract after the ArrayRef
ParsedItem ce;
ce.kind = ParsedItem::Kind::ComponentExtract;
ce.componentIndex = compIdx;
items.push_back(ce);
```

For `[T, C]` (two bracket numbers):
```cpp
usize numTuples = 1;
for(usize d : prevItem.arrayTupleShape)
{
  numTuples *= d;
}
usize numComponents = 1;
for(usize d : prevItem.arrayCompShape)
{
  numComponents *= d;
}

if(tupleIdx >= numTuples)
{
  return MakeErrorResult(static_cast<int>(CalculatorErrorCode::TupleOutOfRange),
    fmt::format("Tuple index {} is out of range for array with {} tuples.", tupleIdx, numTuples));
}
if(compIdx >= numComponents)
{
  return MakeErrorResult(static_cast<int>(CalculatorErrorCode::ComponentOutOfRange),
    fmt::format("Component index {} is out of range for array with {} components.", compIdx, numComponents));
}

ParsedItem tce;
tce.kind = ParsedItem::Kind::TupleComponentExtract;
tce.tupleIndex = tupleIdx;
tce.componentIndex = compIdx;
items.push_back(tce);
```

The `(expr)[C]` and `(expr)[T,C]` paths (when `prevItem.kind == ParsedItem::Kind::RParen`) remain unchanged — they already emit ComponentExtract/TupleComponentExtract items.

- [ ] **Step 5: Rewrite parse() — validation step 7b**

The validation step 7b (starting around line 1316) currently queries `m_TempDataStructure.getDataAs<Float64Array>()` for each array value. Replace it to query `ParsedItem::arrayTupleShape` and `ParsedItem::arrayCompShape` directly:

```cpp
// 7b: Collect array-type values and verify consistent tuple/component info
std::vector<usize> arrayTupleShape;
std::vector<usize> arrayCompShape;
usize arrayNumTuples = 0;
bool hasArray = false;
bool hasNumericValue = false;
bool tupleShapesMatch = true;

for(const auto& item : items)
{
  if(item.kind == ParsedItem::Kind::Scalar || item.kind == ParsedItem::Kind::ArrayRef)
  {
    hasNumericValue = true;
  }
  if(item.kind == ParsedItem::Kind::ArrayRef)
  {
    std::vector<usize> ts = item.arrayTupleShape;
    std::vector<usize> cs = item.arrayCompShape;
    usize nt = 1;
    for(usize d : ts)
    {
      nt *= d;
    }

    if(hasArray)
    {
      if(!arrayCompShape.empty() && arrayCompShape != cs)
      {
        return MakeErrorResult(static_cast<int>(CalculatorErrorCode::InconsistentCompDims),
          "Attribute Array symbols in the infix expression have mismatching component dimensions.");
      }
      if(arrayNumTuples != 0 && nt != arrayNumTuples)
      {
        return MakeErrorResult(static_cast<int>(CalculatorErrorCode::InconsistentTuples),
          "Attribute Array symbols in the infix expression have mismatching number of tuples.");
      }
      if(!arrayTupleShape.empty() && arrayTupleShape != ts)
      {
        tupleShapesMatch = false;
      }
    }

    hasArray = true;
    arrayTupleShape = ts;
    arrayCompShape = cs;
    arrayNumTuples = nt;
  }
}
```

- [ ] **Step 6: Rewrite parse() — shunting-yard conversion to RPN**

The shunting-yard loop (starting around line 1416) currently converts ParsedItems to RpnItems. Update the `Value` case to handle the new `Scalar` and `ArrayRef` kinds:

Replace the `ParsedItem::Kind::Value` case with two cases:

```cpp
case ParsedItem::Kind::Scalar: {
  RpnItem rpn;
  rpn.type = RpnItem::Type::Scalar;
  rpn.scalarValue = item.scalarValue;
  m_RpnItems.push_back(rpn);
  break;
}

case ParsedItem::Kind::ArrayRef: {
  RpnItem rpn;
  rpn.type = RpnItem::Type::ArrayRef;
  rpn.arrayPath = item.arrayPath;
  rpn.sourceDataType = item.sourceDataType;
  m_RpnItems.push_back(rpn);
  break;
}
```

Update the `ComponentExtract` and `TupleComponentExtract` cases similarly — they already match the new RpnItem fields. Just ensure the RpnItem type assignment uses `RpnItem::Type::ComponentExtract` / `RpnItem::Type::TupleComponentExtract` and sets `componentIndex`/`tupleIndex`.

- [ ] **Step 7: Update constructor calls in ArrayCalculatorFilter.cpp**

In `ArrayCalculatorFilter.cpp`, update the two places where `ArrayCalculatorParser` is constructed:

In `preflightImpl()` (around line 88), remove the `true` (isPreflight) argument:
```cpp
ArrayCalculatorParser parser(dataStructure, pInfixEquationValue.m_SelectedGroup, pInfixEquationValue.m_Equation, m_ShouldCancel);
```

In `ArrayCalculator::operator()()` in ArrayCalculator.cpp (around line 1789), remove the `false` (isPreflight) argument:
```cpp
ArrayCalculatorParser parser(m_DataStructure, m_InputValues->SelectedGroup, m_InputValues->InfixEquation, m_ShouldCancel);
```

- [ ] **Step 8: Note — do NOT build yet**

The evaluator (`evaluateInto()`) still references the old CalcValue-based eval stack. It must be updated in Task 6 before the code compiles.

---

### Task 6: Rewrite Evaluator to Use CalcBuffer Stack

**Files:**
- Modify: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ArrayCalculator.cpp`

Rewrite `evaluateInto()` to use a `std::stack<CalcBuffer>` with a local `DataStructure` for temps. Add the last-operator OutputDirect optimization and the final result copy logic.

- [ ] **Step 1: Rewrite evaluateInto()**

Replace the entire `evaluateInto()` method (starting at line 1534) with the following implementation. This is the complete new evaluator:

```cpp
Result<> ArrayCalculatorParser::evaluateInto(DataStructure& dataStructure, const DataPath& outputPath, NumericType scalarType, CalculatorParameter::AngleUnits units)
{
  // 1. Parse (populates m_RpnItems via shunting-yard)
  Result<> parseResult = parse();
  if(parseResult.invalid())
  {
    return parseResult;
  }

  // 2. Create local temp DataStructure for intermediate arrays
  DataStructure tempDS;
  usize scratchCounter = 0;
  auto nextScratchName = [&scratchCounter]() -> std::string {
    return "_calc_" + std::to_string(scratchCounter++);
  };

  // 3. Pre-scan RPN to find the index of the last operator/extract item
  //    for the OutputDirect optimization
  DataType outputDataType = ConvertNumericTypeToDataType(scalarType);
  bool outputIsFloat64 = (outputDataType == DataType::float64);
  int64 lastOpIndex = -1;
  for(int64 idx = static_cast<int64>(m_RpnItems.size()) - 1; idx >= 0; --idx)
  {
    RpnItem::Type t = m_RpnItems[static_cast<usize>(idx)].type;
    if(t == RpnItem::Type::Operator || t == RpnItem::Type::ComponentExtract || t == RpnItem::Type::TupleComponentExtract)
    {
      lastOpIndex = idx;
      break;
    }
  }

  // 4. Walk the RPN items using a CalcBuffer evaluation stack
  std::stack<CalcBuffer> evalStack;

  for(usize rpnIdx = 0; rpnIdx < m_RpnItems.size(); ++rpnIdx)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    const RpnItem& rpnItem = m_RpnItems[rpnIdx];
    bool isLastOp = (static_cast<int64>(rpnIdx) == lastOpIndex);

    switch(rpnItem.type)
    {
    case RpnItem::Type::Scalar: {
      evalStack.push(CalcBuffer::scalar(tempDS, rpnItem.scalarValue, nextScratchName()));
      break;
    }

    case RpnItem::Type::ArrayRef: {
      if(rpnItem.sourceDataType == DataType::float64)
      {
        const auto& sourceArray = m_DataStructure.getDataRefAs<Float64Array>(rpnItem.arrayPath);
        evalStack.push(CalcBuffer::borrow(sourceArray));
      }
      else
      {
        const auto& sourceArray = m_DataStructure.getDataRefAs<IDataArray>(rpnItem.arrayPath);
        evalStack.push(CalcBuffer::convertFrom(tempDS, sourceArray, nextScratchName()));
      }
      break;
    }

    case RpnItem::Type::Operator: {
      const OperatorDef* op = rpnItem.op;
      if(op == nullptr)
      {
        return MakeErrorResult(static_cast<int>(CalculatorErrorCode::InvalidEquation), "Internal error: null operator in RPN evaluation.");
      }

      if(op->numArgs == 1)
      {
        if(evalStack.empty())
        {
          return MakeErrorResult(static_cast<int>(CalculatorErrorCode::NotEnoughArguments), "Not enough arguments for unary operator.");
        }
        CalcBuffer operand = std::move(evalStack.top());
        evalStack.pop();

        std::vector<usize> resultTupleShape = operand.tupleShape();
        std::vector<usize> resultCompShape = operand.compShape();
        usize totalSize = operand.size();

        CalcBuffer result = (isLastOp && outputIsFloat64)
          ? CalcBuffer::wrapOutput(dataStructure.getDataRefAs<DataArray<float64>>(outputPath))
          : CalcBuffer::allocate(tempDS, nextScratchName(), resultTupleShape, resultCompShape);

        for(usize i = 0; i < totalSize; i++)
        {
          float64 val = operand.read(i);

          if(op->trigMode == OperatorDef::ForwardTrig && units == CalculatorParameter::AngleUnits::Degrees)
          {
            val = val * (std::numbers::pi / 180.0);
          }

          float64 res = op->unaryOp(val);

          if(op->trigMode == OperatorDef::InverseTrig && units == CalculatorParameter::AngleUnits::Degrees)
          {
            res = res * (180.0 / std::numbers::pi);
          }

          result.write(i, res);
        }

        bool wasScalar = operand.isScalar();
        if(wasScalar)
        {
          result.markAsScalar();
        }
        // operand destroyed here, RAII cleans up
        evalStack.push(std::move(result));
      }
      else if(op->numArgs == 2)
      {
        if(evalStack.size() < 2)
        {
          return MakeErrorResult(static_cast<int>(CalculatorErrorCode::NotEnoughArguments), "Not enough arguments for binary operator.");
        }
        CalcBuffer right = std::move(evalStack.top());
        evalStack.pop();
        CalcBuffer left = std::move(evalStack.top());
        evalStack.pop();

        // Determine output shape: use the array operand's shape (broadcast scalars)
        std::vector<usize> outTupleShape;
        std::vector<usize> outCompShape;
        if(!left.isScalar())
        {
          outTupleShape = left.tupleShape();
          outCompShape = left.compShape();
        }
        else
        {
          outTupleShape = right.tupleShape();
          outCompShape = right.compShape();
        }

        usize totalSize = 1;
        for(usize d : outTupleShape)
        {
          totalSize *= d;
        }
        for(usize d : outCompShape)
        {
          totalSize *= d;
        }

        CalcBuffer result = (isLastOp && outputIsFloat64)
          ? CalcBuffer::wrapOutput(dataStructure.getDataRefAs<DataArray<float64>>(outputPath))
          : CalcBuffer::allocate(tempDS, nextScratchName(), outTupleShape, outCompShape);

        bool leftIsScalar = left.isScalar();
        bool rightIsScalar = right.isScalar();

        for(usize i = 0; i < totalSize; i++)
        {
          float64 lv = left.read(leftIsScalar ? 0 : i);
          float64 rv = right.read(rightIsScalar ? 0 : i);
          result.write(i, op->binaryOp(lv, rv));
        }

        if(leftIsScalar && rightIsScalar)
        {
          result.markAsScalar();
        }
        // left and right destroyed here, RAII cleans up owned temps
        evalStack.push(std::move(result));
      }
      else
      {
        return MakeErrorResult(static_cast<int>(CalculatorErrorCode::InvalidEquation),
          fmt::format("Internal error: operator '{}' has unsupported numArgs={}.", op->token, op->numArgs));
      }
      break;
    }

    case RpnItem::Type::ComponentExtract: {
      if(evalStack.empty())
      {
        return MakeErrorResult(static_cast<int>(CalculatorErrorCode::NotEnoughArguments), "Not enough arguments for component extraction.");
      }
      CalcBuffer operand = std::move(evalStack.top());
      evalStack.pop();

      usize numComps = operand.numComponents();
      usize numTuples = operand.numTuples();
      usize compIdx = rpnItem.componentIndex;

      if(compIdx >= numComps)
      {
        return MakeErrorResult(static_cast<int>(CalculatorErrorCode::ComponentOutOfRange),
          fmt::format("Component index {} is out of range for array with {} components.", compIdx, numComps));
      }

      CalcBuffer result = (isLastOp && outputIsFloat64)
        ? CalcBuffer::wrapOutput(dataStructure.getDataRefAs<DataArray<float64>>(outputPath))
        : CalcBuffer::allocate(tempDS, nextScratchName(), operand.tupleShape(), std::vector<usize>{1});

      for(usize t = 0; t < numTuples; ++t)
      {
        result.write(t, operand.read(t * numComps + compIdx));
      }

      evalStack.push(std::move(result));
      break;
    }

    case RpnItem::Type::TupleComponentExtract: {
      if(evalStack.empty())
      {
        return MakeErrorResult(static_cast<int>(CalculatorErrorCode::NotEnoughArguments), "Not enough arguments for tuple+component extraction.");
      }
      CalcBuffer operand = std::move(evalStack.top());
      evalStack.pop();

      usize numComps = operand.numComponents();
      usize numTuples = operand.numTuples();
      usize tupleIdx = rpnItem.tupleIndex;
      usize compIdx = rpnItem.componentIndex;

      if(tupleIdx >= numTuples)
      {
        return MakeErrorResult(static_cast<int>(CalculatorErrorCode::TupleOutOfRange),
          fmt::format("Tuple index {} is out of range for array with {} tuples.", tupleIdx, numTuples));
      }
      if(compIdx >= numComps)
      {
        return MakeErrorResult(static_cast<int>(CalculatorErrorCode::ComponentOutOfRange),
          fmt::format("Component index {} is out of range for array with {} components.", compIdx, numComps));
      }

      float64 value = operand.read(tupleIdx * numComps + compIdx);
      // operand destroyed, RAII cleans up
      evalStack.push(CalcBuffer::scalar(tempDS, value, nextScratchName()));
      break;
    }

    } // end switch
  }

  // 5. Final result
  if(evalStack.size() != 1)
  {
    return MakeErrorResult(static_cast<int>(CalculatorErrorCode::InvalidEquation),
      fmt::format("Internal error: evaluation stack has {} items remaining; expected exactly 1.", evalStack.size()));
  }

  CalcBuffer finalResult = std::move(evalStack.top());
  evalStack.pop();

  // 6. Copy/cast result into the output array (checked in order, first match wins)
  if(finalResult.isScalar())
  {
    // Fill entire output with the scalar value
    float64 scalarVal = finalResult.read(0);
    ExecuteDataFunction(CopyResultFunctor{}, outputDataType, dataStructure, outputPath, scalarVal);
  }
  else if(finalResult.isOutputDirect())
  {
    // Data is already in the output array — nothing to do
  }
  else if(outputIsFloat64)
  {
    // Direct float64-to-float64 copy via operator[] (no type cast)
    auto& outputArray = dataStructure.getDataRefAs<DataArray<float64>>(outputPath);
    usize totalSize = finalResult.size();
    for(usize i = 0; i < totalSize; i++)
    {
      outputArray[i] = finalResult.read(i);
    }
  }
  else
  {
    // Type-casting copy via CopyResultFunctor
    const Float64Array& resultArray = finalResult.array();
    ExecuteDataFunction(CopyResultFunctor{}, outputDataType, dataStructure, outputPath, &resultArray, false);
  }

  return parseResult;
}
```

- [ ] **Step 2: Update CopyResultFunctor to support scalar fill**

The scalar fill path now passes a `float64` value directly. Add an overload or update `CopyResultFunctor` in the anonymous namespace. Replace the existing `CopyResultFunctor` (lines 230-248) with:

```cpp
struct CopyResultFunctor
{
  // Full array copy (non-float64 output)
  template <typename T>
  void operator()(DataStructure& ds, const DataPath& outputPath, const Float64Array* resultArray, bool /*unused*/)
  {
    auto& output = ds.getDataRefAs<DataArray<T>>(outputPath).getDataStoreRef();
    for(usize i = 0; i < output.getSize(); i++)
    {
      output[i] = static_cast<T>(resultArray->at(i));
    }
  }

  // Scalar fill
  template <typename T>
  void operator()(DataStructure& ds, const DataPath& outputPath, float64 scalarValue)
  {
    auto& output = ds.getDataRefAs<DataArray<T>>(outputPath);
    output.fill(static_cast<T>(scalarValue));
  }
};
```

- [ ] **Step 3: Remove old CopyToFloat64Functor**

The `CopyToFloat64Functor` (lines 122-134) is no longer needed at the top level since its logic is now inside `CalcBuffer::convertFrom()`. However, `CalcBuffer::convertFrom()` still calls it via `ExecuteDataFunction`. Keep `CopyToFloat64Functor` in the anonymous namespace — it is still referenced by `CalcBuffer::convertFrom()`.

- [ ] **Step 4: Build the project**

```bash
cd /Users/mjackson/Workspace2/DREAM3D-Build/simplnx-Rel && cmake --build . --target SimplnxCore
```

Expected: Build succeeds. Fix any compilation errors arising from ParsedItem/RpnItem field name mismatches — common issues:
- `item.kind == ParsedItem::Kind::Value` → split into `ParsedItem::Kind::Scalar` and `ParsedItem::Kind::ArrayRef`
- `item.value.kind == CalcValue::Kind::Array` → `item.kind == ParsedItem::Kind::ArrayRef`
- References to `item.value` → `item.scalarValue` or `item.arrayPath`

- [ ] **Step 5: Run all ArrayCalculator tests**

```bash
cd /Users/mjackson/Workspace2/DREAM3D-Build/simplnx-Rel && ctest -R "SimplnxCore::ArrayCalculatorFilter" --verbose
```

Expected: All test cases pass — identical behavior to baseline. If any fail, debug by comparing the error code or output value against the expected. Common issues:
- Bracket indexing `Array[C]` now emits ArrayRef + ComponentExtract, so the evaluator must handle ComponentExtract on borrowed arrays correctly
- Scalar detection: CalcBuffer created via `CalcBuffer::scalar()` has `m_IsScalar = true`, but CalcBuffers from binary operations where both operands are scalar should also be scalar. Check that the scalar fill path triggers correctly for all-scalar expressions.

- [ ] **Step 6: Commit**

```bash
git add src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ArrayCalculator.hpp \
        src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ArrayCalculator.cpp \
        src/Plugins/SimplnxCore/src/SimplnxCore/Filters/ArrayCalculatorFilter.cpp
git commit -m "MEM: Rewrite ArrayCalculator parser and evaluator with CalcBuffer RAII

Parser is now data-free: produces RPN items with DataPath/scalar metadata
instead of allocating temporary Float64Arrays. Evaluator uses a CalcBuffer
stack with RAII cleanup — intermediates are freed when consumed. Float64
input arrays are borrowed (zero-copy). The last RPN operator writes
directly into the output DataArray when output type is float64."
```

---

### Task 7: Final Verification and Cleanup

**Files:**
- Modify: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ArrayCalculator.hpp` (cleanup)
- Modify: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ArrayCalculator.cpp` (cleanup)

- [ ] **Step 1: Run clang-format on modified files**

```bash
cd /Users/mjackson/Workspace7/simplnx && clang-format -i \
  src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ArrayCalculator.hpp \
  src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ArrayCalculator.cpp \
  src/Plugins/SimplnxCore/src/SimplnxCore/Filters/ArrayCalculatorFilter.cpp
```

- [ ] **Step 2: Build the full project (not just SimplnxCore)**

```bash
cd /Users/mjackson/Workspace2/DREAM3D-Build/simplnx-Rel && cmake --build . --target all
```

Expected: Full build succeeds — no other files reference CalcValue or the removed parser members.

- [ ] **Step 3: Run the full SimplnxCore test suite**

```bash
cd /Users/mjackson/Workspace2/DREAM3D-Build/simplnx-Rel && ctest -R "SimplnxCore::" --verbose
```

Expected: All SimplnxCore tests pass. This catches any accidental regressions in other filters.

- [ ] **Step 4: Run ArrayCalculator tests specifically and verify all assertions pass**

```bash
cd /Users/mjackson/Workspace2/DREAM3D-Build/simplnx-Rel && ctest -R "SimplnxCore::ArrayCalculatorFilter" --verbose
```

Expected: All 9 test cases pass with all assertions (the test output should show the same assertion count as the baseline from Task 1).

- [ ] **Step 5: Commit formatting changes (if any)**

```bash
git add src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ArrayCalculator.hpp \
        src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ArrayCalculator.cpp \
        src/Plugins/SimplnxCore/src/SimplnxCore/Filters/ArrayCalculatorFilter.cpp
git commit -m "STY: Run clang-format on ArrayCalculator files after memory optimization"
```

