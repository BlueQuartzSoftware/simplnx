# Initialize Data

## Group (Subgroup)

Processing (Cleanup)

## Description

This **Filter** overwrites every tuple in a **Data Array** with a value chosen by one of four initialization modes. Multi-component arrays can be initialized either uniformly (one value applied to every component) or per-component (a semicolon-separated list).

All initialization modes generate values in bounded contiguous blocks and write them through bulk storage transfers. The resident working set therefore stays fixed for both in-memory and disk-backed arrays, including Boolean and multi-component arrays. Incremental and seeded random modes retain tuple-major, component-minor generation order, so chunk boundaries do not change their results.

### Initialization Type

The *Initialization Type* parameter provides four modes:

- **Fill Value [0]** -- every tuple is set to a user-supplied constant value (or per-component values).
- **Incremental [1]** -- the array is filled starting from a user-supplied starting value, applying a fixed step (increment or decrement) to each successive tuple.
- **Random [2]** -- every tuple is set to a uniformly-random value drawn from the **full range** of the array's data type. An optional fixed seed can be supplied for reproducibility.
- **Random With Range [3]** -- every tuple is set to a uniformly-random value drawn from a user-specified [min, max] interval per component.

#### Fill Value

Provide a single value or, for multi-component arrays, a semicolon-separated list. The same value is copied into every tuple.

#### Incremental

Provide a starting value, a *Step Operation* (Addition or Subtraction), and a step value. Each successive tuple's value is the previous tuple's value plus (or minus) the step. A step value of 0 leaves that component unchanged across all tuples.

Example -- a 3-component array filled with 3-D rotations in radians, stepping X and Y but not Z:

- Starting value: `0`
- Step Operation: Addition
- Step Values: `3.141592;6.283185;0`

#### Random and Random With Range

Both modes draw uniformly. *Random* uses the full data-type range; *Random With Range* honors a per-component lower and upper bound. Both modes use the same random seed plumbing:

- *Use Seed for Random Generation* + *Seed Value* -- supply a fixed integer seed for reproducibility.
- With the option disabled, a time-based seed is generated; the actual seed used is saved to the *Stored Seed Value Array* so the run can be reproduced later.

*Standardize Seed* (Random mode only) controls whether all components in a tuple share the same random draw:

- ON: a single value is drawn per tuple and broadcast to all components: `| 3;3;3 | 9;9;9 | 4;4;4 | ...`
- OFF: each component is drawn independently: `| 3;9;4 | 7;2;8 | 5;9;6 | ...`

For *Random With Range* on a multi-component array, the lower and upper bounds are semicolon-separated per component. A trick for fixing one component while randomizing the others: set the lower and upper bound to the same value for the fixed component. Example -- 3-component array where the middle component is always 6 and the others vary:

- Lower bound: `0;6;0`
- Upper bound: `90;6;252`

### Step Operation

Used only with *Incremental*:

- **Addition [0]** -- adds the step value to the previous tuple's value.
- **Subtraction [1]** -- subtracts the step value from the previous tuple's value.

### Boolean Array Notes

For boolean arrays, the only values that produce `false` are:

- The strings `False`, `FALSE`, `false`.
- Any well-formed numeric `0` (integer or floating-point).

**Any other** string or number is interpreted as `true`. Conventional `true` values are the strings `True`, `TRUE`, `true`, or the numeric `1`.

For boolean arrays under *Incremental* mode, the *Step Operation* behaves as follows:

- **Addition** with step value > 0: starting `false` produces `false, true, true, ...`; starting `true` produces all `true`.
- **Subtraction** with step value > 0: starting `true` produces `true, false, false, ...`; starting `false` produces all `false`.

### Multi-Component Value Format

Single value: applied to all components. Example: `2.5` initializes every component of every tuple to 2.5.

Per-component values: semicolons separate components. Example for a 2-component array: `0;1` sets component 0 to 0 and component 1 to 1.

Semicolons (rather than commas) are used to avoid international locale ambiguity (commas are decimal points in some locales).

### Required Input Sources

- **Input Data Array** -- the array to overwrite. Typically a previously created or imported array.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
