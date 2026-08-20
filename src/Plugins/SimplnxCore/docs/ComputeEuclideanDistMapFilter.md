# Compute Euclidean Distance Map

## Group (Subgroup)

Statistics (Morphological)

## Description

This **Filter** measures how far each **Cell** is from the nearest **Feature** boundary, **triple line** and/or **quadruple point**, and writes one output array per requested category.

### Step 1 — mark the boundary Cells (the *seeds*)

For every **Cell** whose *Feature Id* is greater than zero, the **Filter** collects the *distinct* **Feature Ids** of its six face-face neighbours that differ from the **Cell**'s own **Feature Id**. Call that count `n`. Neighbours outside the volume are not counted, but **Feature Id 0 is counted as a distinct neighbouring Feature** — a **Cell** on the edge of the *bad data* region is therefore a boundary **Cell**.

| Condition | The Cell is a seed for |
|---|---|
| `n >= 1` | *Boundary Distances* |
| `n >= 2` | *Triple Line Distances* |
| `n >= 3` | *Quadruple Point Distances* |

Every seed's distance is set to *0*. Because the thresholds nest, the quadruple-point seeds are a subset of the triple-line seeds, which are a subset of the boundary seeds.

### Step 2 — grow outward from the seeds

Each requested map is then grown independently, one *layer* at a time. In each pass, every **Cell** that does not yet have a distance and whose *Feature Id* is greater than zero looks at its six face neighbours; if any of them already has a distance, the **Cell** adopts that neighbour's *nearest seed* and is assigned the current pass number as its distance. Passes continue until no further **Cell** can be assigned.

Propagation only travels through **Cells** whose *Feature Id* is greater than zero, and it crosses **Feature** boundaries freely — the distance is measured to the nearest *boundary*, not within a single **Feature**.

### Step 3 — convert to distance

- If *Output arrays are Manhattan distance (int32)* is **on**, the pass numbers from Step 2 are the output. These are "city-block" (6-connected graph) distances measured in **Cells**, not in physical units, and they ignore the **Geometry**'s spacing.

- If it is **off**, each **Cell**'s pass number is replaced by the straight-line physical distance — using the **Geometry**'s spacing — from the **Cell** to the *nearest seed it was assigned in Step 2*, and the result is stored as `float32`.

> **Important:** the `float32` output is **not** a Euclidean distance transform. It is the straight-line distance to *the one seed the layer-by-layer growth happened to hand the **Cell***, and when several neighbours are available in the same pass the **Filter** takes the last one it examines, in the fixed order
>
> `+Z` beats `+Y` beats `+X` beats `-X` beats `-Y` beats `-Z`.
>
> That choice is a tie-break, not a search for the closest boundary, so the reported distance is always **greater than or equal to** the true distance to the nearest seed, and it can be strictly greater. A worked example: on a 10x6x1 **Image Geometry** with spacing `(1, 2, 1)` and two **Features** stacked in *Y*, the corner **Cell** `(0, 0, 0)` is reported as `4.0` even though a boundary seed sits `2.0` away, because the tie-break preferred the `+Y` neighbour over the `+X` one. If you need a true nearest-boundary distance, do not use the `float32` output of this **Filter**.
>
> The same tie-break is present in the *Manhattan* output, but it cannot change any value there: every neighbour available in a given pass carries the same pass number, so only the *identity* of the recorded seed differs, and that identity is not written out.

### The `-1` fill value

Both output types are pre-filled with `-1`, and an element keeps that value when the **Filter** never assigns it a distance. That happens in two cases:

1. The **Cell**'s *Feature Id* is less than or equal to zero (*bad data*). Such **Cells** are excluded from Step 1 and Step 2 entirely.
2. The requested category has no seeds at all, or the **Cell** cannot reach one by travelling through **Cells** with a positive *Feature Id*. A single-**Feature** volume has no boundary seeds, so *every* element of every map stays `-1`; a volume with no quadruple points leaves the whole *Quadruple Point Distances* array at `-1`.

`-1` therefore means "no distance was computed here", and downstream **Filters** must not treat it as a distance of one **Cell**.

### Behaviour notes

- At least one of the three category options must be enabled; the **Filter** reports an error if all three are off.
- A given category's output does not depend on which of the other two categories are also requested.
- The *nearest seed* bookkeeping is internal and is not saved as an output array.

% Auto generated parameter table will be inserted here

## Example Pipelines

- (01) SmallIN100 Morphological Statistics

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
