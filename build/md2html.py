#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""Simple Markdown to HTML converter for build process"""

import sys
import os

def convert_md_to_html(md_file, html_file):
    """Convert markdown file to basic HTML"""
    try:
        with open(md_file, 'r', encoding='utf-8') as f:
            content = f.read()

        # Basic HTML template
        html = f"""<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>{os.path.basename(md_file)}</title>
    <style>
        body {{ font-family: Arial, sans-serif; margin: 40px; }}
        pre {{ background: #f4f4f4; padding: 10px; }}
        code {{ background: #f4f4f4; padding: 2px 4px; }}
    </style>
</head>
<body>
<pre>{content}</pre>
</body>
</html>"""

        with open(html_file, 'w', encoding='utf-8') as f:
            f.write(html)

        return 0
    except Exception as e:
        print(f"Error converting {md_file}: {e}", file=sys.stderr)
        return 1

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Usage: md2html.py <input.md> <output.html>", file=sys.stderr)
        sys.exit(1)

    sys.exit(convert_md_to_html(sys.argv[1], sys.argv[2]))
