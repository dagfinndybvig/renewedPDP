"""Token-stream command reader — mirrors the C ``get_command()`` / ``in_stream`` model.

The legacy C programs read a single whitespace-separated token at a time from
``in_stream`` (which is either stdin or a script file).  A command like::

    get network JETS.NET

could appear on one line **or** as three separate lines::

    get
    network
    JETS.NET

Both forms are handled identically because the C I/O layer just calls
``fscanf(..., "%s", token)`` each time it needs the next word.

``CommandTokenStream`` replicates that behaviour: callers call ``next()`` to
consume the next whitespace-delimited token.  ``run_tokens()`` drives the
top-level command dispatch table, building each command by consuming exactly
as many tokens as needed for each recognised command verb.
"""
from __future__ import annotations

import sys
from collections import deque
from pathlib import Path
from typing import Iterator


class CommandTokenStream:
    """A flat stream of whitespace-split tokens from one or more sources."""

    def __init__(self) -> None:
        self._queue: deque[str] = deque()
        self._eof = False

    # ------------------------------------------------------------------
    # Loading tokens
    # ------------------------------------------------------------------

    def feed_file(self, path: Path) -> None:
        """Append all tokens from *path* to the queue."""
        text = path.read_text(encoding="utf-8", errors="ignore")
        for token in text.split():
            self._queue.append(token)

    def feed_string(self, text: str) -> None:
        """Append all tokens from a string literal."""
        for token in text.split():
            self._queue.append(token)

    def feed_interactive(self, prompt: str) -> Iterator[str]:
        """Yield tokens interactively, one prompt-line at a time.

        Intended use:  ``stream.feed_interactive("iac: ")`` returns an
        iterator that keeps pulling lines from the user and feeding them
        into the queue; callers should drive this via :meth:`next_interactive`.
        """
        # Not used directly by callers; interactive mode is handled
        # in the CLI loops.
        raise NotImplementedError

    # ------------------------------------------------------------------
    # Consuming tokens
    # ------------------------------------------------------------------

    def peek(self) -> str | None:
        """Return the next token without consuming it, or None if empty."""
        return self._queue[0] if self._queue else None

    def next(self) -> str | None:  # noqa: A003
        """Consume and return the next token, or None if the stream is empty."""
        return self._queue.popleft() if self._queue else None

    def next_required(self, context: str = "") -> str:
        """Consume the next token; raise ValueError if the stream is exhausted."""
        tok = self.next()
        if tok is None:
            raise ValueError(
                f"Unexpected end of input{(' in ' + context) if context else ''}"
            )
        return tok

    def consume_until(self, sentinel: str) -> list[str]:
        """Consume tokens until *sentinel* is found; return them (excluding sentinel)."""
        result: list[str] = []
        while True:
            tok = self.next()
            if tok is None:
                break
            if tok.lower() == sentinel.lower():
                break
            result.append(tok)
        return result

    def is_empty(self) -> bool:
        return len(self._queue) == 0
