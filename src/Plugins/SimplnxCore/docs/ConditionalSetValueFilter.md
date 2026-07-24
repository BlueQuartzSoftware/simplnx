# Replace Value in Array (Conditional)

## Group (Subgroup)

Core (Misc)

## Description

This **Filter** replaces selected values in a scalar **Attribute Array** with a user-specified value. The filter operates in one of two modes selected by the *Use Conditional Mask* parameter.

### Mode 1 -- Conditional Mask (Use Conditional Mask = ON)

A second boolean array (the *Conditional Mask*) of the same length picks which tuples to overwrite. Every tuple where the mask is *true* has its scalar value set to *Replace Value*; tuples where the mask is *false* are unchanged.

Typical use case: replace cell values flagged by an upstream threshold (e.g., set all "bad" cells to 0).

### Mode 2 -- Value Match (Use Conditional Mask = OFF)

Every occurrence of *Value to Replace* in the target array is replaced with *Replace Value*. The Conditional Mask parameter is ignored.

Typical use case: remap a specific sentinel value (e.g., turn every -1 into 0).

### Numeric Type Compatibility

The target array must be a **scalar** (single-component) array. *Replace Value* (and *Value to Replace* in Mode 2) are reinterpreted as the array's data type. The valid range for each primitive type:

| Type | Size | Range |
|------|------|-------|
| Signed Integer | 8 bit | -128 to 127 |
| Unsigned Integer | 8 bit | 0 to 255 |
| Signed Integer | 16 bit | -32,768 to 32,767 |
| Unsigned Integer | 16 bit | 0 to 65,535 |
| Signed Integer | 32 bit | -2,147,483,648 to 2,147,483,647 |
| Unsigned Integer | 32 bit | 0 to 4,294,967,295 |
| Signed Integer | 64 bit | -9.2e18 to 9.2e18 |
| Unsigned Integer | 64 bit | 0 to 1.8e19 |
| Float | 32 bit | ±1.1e-38 to ±3.4e+38 (7 digits) |
| Double | 64 bit | ±2.2e-308 to ±1.7e+308 (15 digits) |
| Boolean | 8 bit | 0 = false, non-zero = true |

### Required Input Sources

- **Target Array** -- the scalar cell-level (or other tuple-level) array whose values will be overwritten.
- **Conditional Mask** (only when *Use Conditional Mask* is enabled) -- a boolean array with the same number of tuples; typically produced by [Multi-Threshold Objects](MultiThresholdObjectsFilter.md).

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
