from __future__ import annotations

import unittest
from pathlib import Path

from pdp.io.template_parser import parse_template_file


class TestTemplateParser(unittest.TestCase):
    def test_parses_iac_jets_template(self) -> None:
        repo_root = Path(__file__).resolve().parents[2]
        spec = parse_template_file("JETS.TEM", base_dir=repo_root / "iac")

        self.assertEqual(len(spec.entries), 5)

        names = [entry.name for entry in spec.entries]
        self.assertIn("extinput", names)
        self.assertIn("activation", names)

        ext_entry = next(entry for entry in spec.entries if entry.name == "extinput")
        self.assertEqual(ext_entry.template_type, "look")
        self.assertIsNotNone(ext_entry.look_table)
        self.assertEqual(ext_entry.look_table.rows, 18)
        self.assertEqual(ext_entry.look_table.cols, 5)


if __name__ == "__main__":
    unittest.main()
