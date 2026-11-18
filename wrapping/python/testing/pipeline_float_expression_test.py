import math

from simplnx_utilities import _encode_simple_value


# Evaluate the generated keyword expression in the same built-in namespace as a saved Python pipeline.
for cutoff in (math.inf, -math.inf, 137.125):
    expression = _encode_simple_value(cutoff)
    restored = eval(expression, {"__builtins__": {"float": float}})
    assert restored == cutoff, expression
