# GenerateMisorientationColors

| Ready | Parameter Key | Human Name | Parameter Type | Parameter Class |
|-------|---------------|------------|-----------------|----------------|
| YES | ReferenceAxis | Reference Orientation Axis | VectorFloat32Parameter::ValueType | VectorFloat32Parameter |
| YES | ReferenceAngle | Reference Orientation Angle (Degrees) | float32 | Float32Parameter |
| YES | UseGoodVoxels | Apply to Good Elements Only (Bad Elements Will Be Black) | bool | BoolParameter |
| YES | QuatsArrayPath | Quaternions | DataPath | ArraySelectionParameter |
| YES | CellPhasesArrayPath | Phases | DataPath | ArraySelectionParameter |
| YES | GoodVoxelsArrayPath | Mask | DataPath | ArraySelectionParameter |
| YES | CrystalStructuresArrayPath | Crystal Structures | DataPath | ArraySelectionParameter |
| YES | MisorientationColorArrayName | Misorientation Colors | DataPath | ArrayCreationParameter |
