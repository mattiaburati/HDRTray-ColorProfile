#!/usr/bin/env python3

import sys


def main() -> int:
    if len(sys.argv) != 3:
        sys.stderr.write("Usage: md2html.py <input_markdown> <output_html>\n")
        return 1

    try:
        import marko
    except ImportError:
        sys.stderr.write("md2html.py: missing dependency 'marko'. Install it with `pip install marko`\n")
        return 1

    input_path = sys.argv[1]
    output_path = sys.argv[2]

    try:
        with open(input_path, "r", encoding="utf-8") as infile:
            markdown_text = infile.read()
        html_text = marko.convert(markdown_text)
        with open(output_path, "w", encoding="utf-8") as outfile:
            outfile.write(html_text)
    except Exception as exc:
        sys.stderr.write(f"md2html.py: {exc}\n")
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
