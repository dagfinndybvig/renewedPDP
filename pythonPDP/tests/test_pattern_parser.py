from __future__ import annotations

import tempfile
import unittest
from pathlib import Path

from pdp.io.pattern_parser import parse_pattern_file


class TestPatternParser(unittest.TestCase):
    def test_parses_symbolic_and_numeric_tokens(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "mini.pat"
            path.write_text("p0 + - . 0.5\np1 - + . -1\n", encoding="utf-8")

            parsed = parse_pattern_file(path, expected_inputs=4)

        self.assertEqual(parsed.names, ["p0", "p1"])
        self.assertEqual(parsed.inputs[0], [1.0, -1.0, 0.0, 0.5])
        self.assertEqual(parsed.inputs[1], [-1.0, 1.0, 0.0, -1.0])


if __name__ == "__main__":
    unittest.main()
