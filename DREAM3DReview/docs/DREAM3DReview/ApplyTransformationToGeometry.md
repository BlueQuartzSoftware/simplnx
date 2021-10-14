# ApplyTransformationToGeometry #

| Ready | Parameter Key | Human Name | Parameter Type | Parameter Class |
|-------|---------------|------------|-----------------|----------------|
| YES | TransformationMatrixType | Transformation Type | ChoicesParameter::ValueType | ChoicesParameter |
| NO | ManualTransformationMatrix | Transformation Matrix | <<<NOT_IMPLEMENTED>>> | DynamicTableFilterParameter |
| YES | RotationAngle | Rotation Angle (Degrees) | float32 | Float32Parameter |
| YES | RotationAxis | Rotation Axis (ijk) | VectorFloat32Parameter::ValueType | VectorFloat32Parameter |
| YES | Translation | Translation | VectorFloat32Parameter::ValueType | VectorFloat32Parameter |
| YES | Scale | Scale | VectorFloat32Parameter::ValueType | VectorFloat32Parameter |
| YES | GeometryToTransform | Geometry to Transform | DataPath | DataGroupSelectionParameter |
| YES | ComputedTransformationMatrix | Transformation Matrix | DataPath | ArraySelectionParameter |
