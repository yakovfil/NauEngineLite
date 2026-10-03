"""Reject active EASTL dependencies in the engine and its source consumers.

Historical Markdown and third-party attribution are intentionally outside this
check. Pass --fixture FILE to verify that a proposed source dependency fails.
"""

from __future__ import annotations

import argparse
import re
import subprocess
from pathlib import Path


ENGINE = Path(__file__).resolve().parents[2]
ROOT = ENGINE.parent
REPOSITORIES = (ENGINE, ROOT / "NauEditorPublic", ROOT / "NauSamples", ROOT / "NauDemo")
SOURCE_SUFFIXES = {".c", ".cc", ".cpp", ".h", ".hpp", ".inl", ".mm", ".cmake", ".py", ".json", ".natjmc", ".natvis"}
DEPENDENCY = re.compile(
    r"\beastl::|[<\"/]EASTL[/\\]|\bEASTL\b|\beastl_(?:aliases|new)\b|EASTL_BUILD_|EASTL-config",
    re.IGNORECASE,
)
COMMENT = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|//[^\n]*|/\*[\s\S]*?\*/')


def tracked_sources(repo: Path):
    result = subprocess.run(
        ["git", "ls-files", "--cached", "--others", "--exclude-standard", "-z"],
        cwd=repo, check=True, capture_output=True,
    )
    for entry in result.stdout.split(b"\0"):
        if not entry:
            continue
        path = repo / entry.decode("utf-8", errors="surrogateescape")
        if path.resolve() == Path(__file__).resolve():
            continue
        if path.name != "CMakeLists.txt" and path.suffix.lower() not in SOURCE_SUFFIXES:
            continue
        if not path.is_file() or any(part.lower() in {"build", "dist", "output", "external", "3rd_party"} for part in path.relative_to(repo).parts):
            continue
        yield path


def active_text(path: Path) -> str:
    contents = path.read_text(encoding="utf-8", errors="replace")
    if path.suffix.lower() in {".c", ".cc", ".cpp", ".h", ".hpp", ".inl", ".mm"}:
        return COMMENT.sub(lambda match: "" if match[0].startswith(("//", "/*")) else match[0], contents)
    if path.suffix.lower() in {".py", ".cmake"} or path.name == "CMakeLists.txt":
        return "\n".join(line for line in contents.splitlines() if not line.lstrip().startswith("#"))
    return contents


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fixture", type=Path, help="Also check a source fixture; it must fail if it imports EASTL")
    args = parser.parse_args()
    paths = [path for repo in REPOSITORIES for path in tracked_sources(repo)]
    if args.fixture:
        paths.append(args.fixture)
    failures = []
    for path in paths:
        match = DEPENDENCY.search(active_text(path))
        if match:
            failures.append(f"{path}: {match.group(0)}")
    for failure in failures:
        print(failure)
    print(f"Checked {len(paths)} active source/build files; {len(failures)} EASTL dependencies")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
