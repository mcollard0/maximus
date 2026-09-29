#!/usr/bin/env python3
"""Storage bridge for the Maximus MEX oneliner screen.

The MEX VM has no transaction or exclusive-create primitive. SQLite keeps
the daily limit atomic across independently forked Maximus nodes.
"""

from __future__ import annotations

import os
import sqlite3
import sys
from datetime import date
from pathlib import Path


SCHEMA = """
CREATE TABLE IF NOT EXISTS oneliners (
    day TEXT NOT NULL,
    user_key TEXT NOT NULL,
    display_name TEXT NOT NULL,
    message TEXT NOT NULL,
    PRIMARY KEY (day, user_key)
)
"""


def clean(value: str, limit: int) -> str:
    """Keep the result as one safe terminal line in the BBS code page."""
    value = " ".join(value.split())
    value = "".join(c for c in value if 32 <= ord(c) < 127)
    return value[:limit].strip()


def submit(db: sqlite3.Connection, day: str, user: str, message: str) -> str:
    user = clean(user, 35)
    message = clean(message, 72)
    if not user or not message:
        return "INVALID"
    db.execute("BEGIN IMMEDIATE")
    try:
        row = db.execute(
            "INSERT OR IGNORE INTO oneliners VALUES (?, ?, ?, ?)",
            (day, user.casefold(), user, message),
        )
        db.commit()
        return "POSTED" if row.rowcount == 1 else "LIMIT"
    except BaseException:
        db.rollback()
        raise


def feed(db: sqlite3.Connection, day: str, user: str) -> tuple[str, list[str]]:
    user = clean(user, 35)
    if not user:
        return "INVALID", []
    already = db.execute(
        "SELECT 1 FROM oneliners WHERE day = ? AND user_key = ?",
        (day, user.casefold()),
    ).fetchone()
    rows = db.execute(
        "SELECT day, display_name, message FROM oneliners "
        "ORDER BY day DESC, rowid DESC LIMIT 20"
    ).fetchall()
    lines = [f"{day}  {name}: {message}" for day, name, message in rows]
    return ("LIMIT" if already else "CAN_POST"), lines


def main() -> int:
    if len(sys.argv) != 3 or sys.argv[1] not in {"list", "post"}:
        return 2
    node = sys.argv[2]
    if not node.isdecimal() or len(node) > 5:
        return 2
    root = Path(os.environ.get("PREFIX", "/var/max")).resolve()
    request = root / f"oneliner.{node}.req"
    result = root / f"oneliner.{node}.result"
    temporary = root / f"oneliner.{node}.result.tmp"
    status = "ERROR"
    lines: list[str] = []
    try:
        raw = request.read_bytes().decode("cp437").splitlines()
        user = raw[0] if raw else ""
        if sys.argv[1] == "post" and len(raw) != 2:
            status = "INVALID"
        elif user:
            with sqlite3.connect(root / "etc" / "oneliner.sqlite3", timeout=10) as db:
                db.execute(SCHEMA)
                today = date.today().isoformat()
                if sys.argv[1] == "post":
                    status = submit(db, today, user, raw[1])
                else:
                    status, lines = feed(db, today, user)
    except (OSError, sqlite3.Error, UnicodeError) as error:
        print(f"oneliner storage error: {error}", file=sys.stderr)
    try:
        temporary.write_bytes(("\n".join([status, *lines]) + "\n").encode("cp437", "replace"))
        os.replace(temporary, result)
    except OSError as error:
        print(f"oneliner result error: {error}", file=sys.stderr)
        return 1
    return 0 if status != "ERROR" else 1


if __name__ == "__main__":
    raise SystemExit(main())
