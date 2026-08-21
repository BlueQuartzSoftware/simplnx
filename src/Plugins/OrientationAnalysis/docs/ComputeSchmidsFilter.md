# Compute Schmid Factors

## Group (Subgroup)

Statistics (Crystallography)

## Description

This **Filter** calculates the Schmid factor of each **Feature** given its average orientation and a user defined loading axis. The Schmid Factor is the combination of the component of the axial force *F* that lies parallel to the slip direction and the component that lies perpendicular to the slip plane.  The equation for the Schmid Factor is given as:

Schmid Factor = (cos &phi; cos &lambda;)

*The angle &phi; is the angle between the tensile axis and the slip plane normal, and &lambda; is the angle between the tensile axis and the slip direction in the slip plane.*

The **Filter** determines the Schmid factor for each **Feature** by using the above equation for all possible slip systems (given the **Feature's** crystal structure).  The largest Schmid factor from all of the slip systems is stored for the **Feature**. Only the Schmid factor is used in determining which slip system's Schmid factor to report.  The critical resolved shear stress for the different slip systems is not considered.

The user-supplied *Loading Direction* is normalized before use, so only its direction matters — `[1, 2, 3]` and `[3, 6, 9]` give identical results. It is a **sample-frame** direction; the **Filter** rotates it into each **Feature's** crystal frame using that **Feature's** average orientation before evaluating the slip systems.

**Feature 0** is the conventional "unassigned" **Feature** and is never computed; all of its output values are zero. A **Feature** whose phase maps to a crystal structure for which no slip systems are enumerated is skipped, and all of its output values are likewise zero.

### Ties Between Slip Systems

Candidate slip systems are compared with a strict greater-than, in the fixed order in which they are enumerated for the crystal structure. When two or more slip systems share the maximum Schmid factor — which happens for high-symmetry loading directions such as `[0, 0, 1]` or `[1, 1, 1]` in a cubic crystal — the **lowest-numbered** of the tied systems is reported. This is a property of the enumeration order, not a physical preference: the tied systems are equally favoured.

### Slip Systems Numbering Depends on Override Default Slip System

The meaning of the *Slip Systems* output changes with the *Override Default Slip System* toggle:

| Override Default Slip System | *Slip Systems* value |
|---|---|
| Off (default) | Index into the crystal structure's built-in slip system list — numbered from **0** (0–11) for Cubic-High and Cubic-Low, and from **1** (1–6) for Hexagonal-High and Hexagonal-Low |
| On | Index of the **symmetry operator** (0–23 for Cubic-High) that maps the user-supplied slip plane and slip direction onto the winning variant |

The two numbering schemes are not comparable, and neither is the base of the *off* numbering comparable between the cubic and hexagonal classes.

A *Slip Systems* value of `0` means **"no slip system found"** in two situations, and in neither of them is it a slip-system number:

- With *Override Default Slip System* **on** and no symmetry-operator variant producing a non-zero Schmid factor — for example a loading direction parallel to the slip plane normal, which puts the loading axis perpendicular to every slip direction.
- With the toggle **off** on a **Hexagonal-High or Hexagonal-Low** phase, where the built-in systems are numbered 1–6 and no candidate exceeds the initial Schmid factor of `0`. Because that numbering starts at 1, `0` falls outside the valid range and is unambiguous. (For the cubic classes, whose numbering starts at 0, the reported `0` in this situation is indistinguishable from a genuine win on system 0; check the reported Schmid factor, which is `0` in the degenerate case.)

In every one of these cases the reported Schmid factor is `0`.

With *Override Default Slip System* on, the reported index is **relative to the symmetry-operator table** of the orientation library, and several operators typically tie at the maximum Schmid factor — for a `(001)[100]` system under `[1,2,3]` loading, six of the twenty-four do. The reported index is simply the first of those in table order. It identifies *a* maximizing variant, not a uniquely determined one, and the same input run through DREAM3D 6.5.x reports a different index for the same physical answer because that version's table is ordered differently. Treat this value as a handle into the current library's table, not as a portable label.

### Phis and Lambdas Units Depend on Override Default Slip System

The *Phis* and *Lambdas* arrays change **units** with the same toggle, which is easy to miss because the parameter descriptions and array names do not:

| Override Default Slip System | *Phis* / *Lambdas* contents |
|---|---|
| Off (default) | The direction **cosines** cos &phi; and cos &lambda; (dimensionless, in [0, 1]) |
| On | The **angles** &phi; and &lambda; themselves, in **radians** |

With the toggle off, the reported Schmid factor is the product of the two stored values. With the toggle on, it is the product of their cosines. If you need consistent units across both modes, convert explicitly rather than assuming.

### Poles Is Not a Miller Index

Despite its name, the *Poles* array does **not** contain a crystallographic index. It is the unit loading direction expressed in the **Feature's** crystal frame, multiplied by 100 and **truncated toward zero** to an integer — a compact fixed-point encoding of that unit vector, retaining two decimal places. Components can be negative, and the sum of squares is approximately 10 000 rather than 1. Because the conversion truncates rather than rounds, a component whose scaled value falls near an integer can differ by one from the value you would get by rounding. Reducing the triplet to a Miller index requires dividing by the greatest common divisor yourself, and the truncation means the result is only approximate.

% Auto generated parameter table will be inserted here

## Example Pipelines

+ (04) Small IN100 Crystallographic Statistics

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
