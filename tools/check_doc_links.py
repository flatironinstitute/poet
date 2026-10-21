#!/usr/bin/env python3
"""Check the internal links of a built Sphinx HTML tree.

Every relative `href`/`src` must resolve to a file in the tree, and every
`#fragment` must name an `id` (or `<a name>`) in the target page. Sphinx's
`linkcheck` builder covers the external links but skips these.

    check_doc_links.py build/docs/_build/html
    check_doc_links.py --self-test
"""

from __future__ import annotations

import argparse
import sys
import tempfile
from html.parser import HTMLParser
from pathlib import Path
from urllib.parse import unquote, urlsplit


class _Page(HTMLParser):
    def __init__(self) -> None:
        super().__init__()
        self.ids: set[str] = set()
        self.links: list[str] = []

    def handle_starttag(self, tag, attrs):
        a = dict(attrs)
        for key in ("id", "name") if tag == "a" else ("id",):
            if value := a.get(key):
                self.ids.add(value)
        for key in ("href", "src"):
            if value := a.get(key):
                self.links.append(value)


def _parse(path: Path) -> _Page:
    page = _Page()
    page.feed(path.read_text(encoding="utf-8", errors="replace"))
    return page


def check(root: Path) -> tuple[int, list[str]]:
    pages = {p: _parse(p) for p in sorted(root.rglob("*.html"))}
    errors = []
    for path, page in pages.items():
        for link in page.links:
            url = urlsplit(link)
            if url.scheme or url.netloc or link.startswith(("mailto:", "javascript:")):
                continue
            target = (path.parent / unquote(url.path)).resolve() if url.path else path
            if target.is_dir():
                target = target / "index.html"
            where = f"{path.relative_to(root)}: {link}"
            if not target.is_file():
                errors.append(f"{where}: missing file")
            elif url.fragment and target.suffix == ".html":
                ids = pages[target].ids if target in pages else _parse(target).ids
                if unquote(url.fragment) not in ids:
                    errors.append(f"{where}: missing anchor")
    return sum(len(p.links) for p in pages.values()), errors


def self_test() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        (root / "sub").mkdir()
        (root / "index.html").write_text(
            '<a href="sub/page.html#ok">1</a><a href="sub/page.html#gone">2</a>'
            '<a href="nope.html">3</a><a href="#top">4</a><a href="https://x.org/y">5</a>'
            '<img src="sub/"><h1 id="top">t</h1>'
        )
        (root / "sub" / "page.html").write_text('<a name="ok"></a>')
        (root / "sub" / "index.html").write_text("")
        _, errors = check(root.resolve())
    expected = [
        "index.html: sub/page.html#gone: missing anchor",
        "index.html: nope.html: missing file",
    ]
    assert errors == expected, errors
    print(
        "self-test: a missing file and a missing anchor are reported; good links, dirs and external URLs pass"
    )


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Check the internal links of a built Sphinx HTML tree."
    )
    parser.add_argument("html_dir", nargs="?", type=Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        self_test()
        return 0
    if args.html_dir is None:
        parser.error("html_dir is required")
    root = args.html_dir.resolve()
    count, errors = check(root)
    if count == 0:
        print(f"no links found under {root}", file=sys.stderr)
        return 1
    for error in errors:
        print(error, file=sys.stderr)
    print(
        f"{count} internal and external links in {root}: {len(errors)} broken internal links"
    )
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
