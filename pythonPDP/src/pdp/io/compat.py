from __future__ import annotations

from pathlib import Path


def resolve_compat_path(path: str | Path, base_dir: str | Path | None = None) -> Path:
    candidate = Path(path)
    if not candidate.is_absolute() and base_dir is not None:
        candidate = Path(base_dir) / candidate

    variants = [candidate]
    variants.append(candidate.with_name(candidate.name.upper()))
    variants.append(candidate.with_name(candidate.name.lower()))

    for variant in variants:
        if variant.exists():
            return variant

    raise FileNotFoundError(f"Could not resolve path: {path}")
