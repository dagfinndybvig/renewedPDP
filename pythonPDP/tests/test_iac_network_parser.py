from __future__ import annotations

import unittest
from pathlib import Path

from pdp.io.network_parser import parse_network_file


class TestIACNetworkParser(unittest.TestCase):
    def test_parses_jets_network(self) -> None:
        repo_root = Path(__file__).resolve().parents[2]
        spec = parse_network_file("JETS.NET", base_dir=repo_root / "iac")

        self.assertEqual(spec.nunits, 68)
        self.assertEqual(len(spec.weights), 68)
        self.assertEqual(len(spec.weights[0]), 68)

        self.assertAlmostEqual(spec.constraints["u"], 1.0)
        self.assertAlmostEqual(spec.constraints["v"], -1.0)

        self.assertAlmostEqual(spec.weights[0][0], 0.0)
        self.assertAlmostEqual(spec.weights[0][1], -1.0)


if __name__ == "__main__":
    unittest.main()
