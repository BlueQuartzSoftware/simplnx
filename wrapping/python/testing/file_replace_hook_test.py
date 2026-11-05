"""Checks the file-replace hook results at the Python boundary of Pipeline.to_file.

A test build of the simplnx module registers the handler that SIMPLNX_TEST_FILE_REPLACE_HANDLER selects
("<action>|<target path>") when the module is imported. Each case therefore runs Pipeline.to_file in a new Python
process with the variable set before the import. SIMPLNX_TEST_FILE_REPLACE_LOG makes the handler write one observation
line per call for the target. It records the action, result validity, warning and error codes, byte count, and FNV-1a hash.
"""

import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

HANDLER_VARIABLE = "SIMPLNX_TEST_FILE_REPLACE_HANDLER"
LOG_VARIABLE = "SIMPLNX_TEST_FILE_REPLACE_LOG"
PREVIOUS_CONTENT = b"previous pipeline file"
NON_ASCII_FOLDER = "ünïcødé_测试"

CHILD_SCRIPT = """
import sys
import simplnx as nx

try:
    nx.Pipeline().to_file(sys.argv[1])
except RuntimeError as error:
    print("RAISED:", error)
    sys.exit(3)
print("WRITTEN")
"""


def fnv1a64(data):
    """Returns the 64-bit FNV-1a hash of bytes as 16 hex digits, as the test handler writes it."""
    value = 14695981039346656037
    for byte in data:
        value ^= byte
        value = (value * 1099511628211) & 0xFFFFFFFFFFFFFFFF
    return f"{value:016x}"


class FileReplaceHookTest(unittest.TestCase):
    def run_to_file(self, action, folder_name):
        """Runs Pipeline.to_file on an existing file with the selected handler action.

        Returns the completed process, the target path, the file bytes after the call and the observation lines.
        """
        with tempfile.TemporaryDirectory() as directory:
            folder = Path(directory) / folder_name
            folder.mkdir()
            target = folder / "existing.d3dpipeline"
            target.write_bytes(PREVIOUS_CONTENT)
            observation_log = Path(directory) / "observations.txt"
            environment = dict(os.environ)
            environment[HANDLER_VARIABLE] = f"{action}|{target}"
            environment[LOG_VARIABLE] = str(observation_log)
            process = subprocess.run([sys.executable, "-c", CHILD_SCRIPT, str(target)], env=environment, capture_output=True, text=True, timeout=600)
            observations = observation_log.read_text(encoding="utf-8").splitlines() if observation_log.exists() else []
            return process, target, target.read_bytes(), observations

    def assert_one_observation(self, observations, action, target, *, valid, warnings="", errors=""):
        """Requires one hook call with the expected result and the original bytes at callback time."""
        expected = (f"action={action} valid={valid} warnings={warnings} errors={errors} "
                    f"size={len(PREVIOUS_CONTENT)} fnv1a64={fnv1a64(PREVIOUS_CONTENT)} target={target}")
        self.assertEqual(observations, [expected])

    def test_each_action_in_ascii_and_non_ascii_folders(self):
        for folder_name in ("ascii", NON_ASCII_FOLDER):
            with self.subTest(folder=folder_name, action="warn"):
                # Pipeline.to_file returns nothing and raises only on an error.
                # The observation proves that the handler returns warning 99701 before the file changes.
                process, target, content, observations = self.run_to_file("warn", folder_name)
                self.assertEqual(process.returncode, 0, process.stdout + process.stderr)
                self.assertIn("WRITTEN", process.stdout)
                self.assertNotEqual(content, PREVIOUS_CONTENT)
                self.assert_one_observation(observations, "warn", target, valid=1, warnings="99701")

            with self.subTest(folder=folder_name, action="refuse"):
                process, target, content, observations = self.run_to_file("refuse", folder_name)
                self.assertEqual(process.returncode, 3, process.stdout + process.stderr)
                self.assertIn("RAISED:", process.stdout)
                self.assertIn("-99702", process.stdout)
                self.assertEqual(content, PREVIOUS_CONTENT)
                self.assert_one_observation(observations, "refuse", target, valid=0, errors="-99702")

            with self.subTest(folder=folder_name, action="refuse-empty"):
                process, target, content, observations = self.run_to_file("refuse-empty", folder_name)
                self.assertEqual(process.returncode, 3, process.stdout + process.stderr)
                self.assertIn("RAISED:", process.stdout)
                self.assertEqual(content, PREVIOUS_CONTENT)
                self.assert_one_observation(observations, "refuse-empty", target, valid=0)


if __name__ == "__main__":
    unittest.main()
