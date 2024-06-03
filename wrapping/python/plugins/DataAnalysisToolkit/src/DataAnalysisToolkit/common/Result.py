from typing import Generic, TypeVar, List
import simplnx as nx

T = TypeVar('T')

class Result(Generic[T]):
    """A class to represent a result that might be a value or an error."""
    def __init__(self, value: T = None, errors: List[nx.Error] = None, warnings: List[nx.Warning] = None):
        if errors and value is not None:
            raise ValueError("Cannot create a Result with both errors and a value.")

        self.value = value
        self.errors = errors if errors else []
        self.warnings = warnings if warnings else []

    def valid(self) -> bool:
        """Check if the Result is valid (i.e., has no errors)."""
        return not self.errors

    def invalid(self) -> bool:
        """Check if the Result is invalid (i.e., has errors)."""
        return self.errors

def make_error_result(code: int, message: str) -> Result:
    return Result(errors=[nx.Error(code, message)])

def make_warning_result(code: int, message: str) -> Result:
    return Result(warnings=[nx.Warning(code, message)])

def convert_result_to_void(result: Result) -> Result[None]:
    """Convert a Result of any type to a Result of type None, preserving errors and warnings."""
    return Result(None, result.errors, result.warnings)


def merge_results(first: Result, second: Result) -> Result:
    """Merge two Results into one, combining their errors and warnings."""
    merged_errors = first.errors + second.errors
    merged_warnings = first.warnings + second.warnings
    # Assuming we're merging results without values; adjust as needed for your use case
    return Result(None, merged_errors, merged_warnings)


# Example usage
if __name__ == "__main__":
    # Create an error and a warning
    error = nx.Error(1, "An error occurred")
    warning = nx.Warning(1, "This is a warning")

    # Create a valid result and an invalid one
    valid_result = Result(value="Success")
    invalid_result = Result(errors=[error])

    # Check if results are valid
    print("Valid result is valid:", valid_result.valid())
    print("Invalid result is valid:", invalid_result.valid())

    # Merge results
    merged_result = merge_results(valid_result, invalid_result)
    print("Merged result has", len(merged_result.errors), "errors and", len(merged_result.warnings), "warnings.")
