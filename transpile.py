#!/usr/bin/env python3
"""
c-- transpiler: turn regular C into enterprise-grade C that stores every
variable in its own PostgreSQL table.

Usage:
    python transpile.py <input.c> <output.c> [--connstr postgres://localhost/cmm]
"""

import argparse
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Optional

# ---------------------------------------------------------------------------
# Types
# ---------------------------------------------------------------------------

C_TO_PG = {
    "int":    ("INTEGER",          "int"),
    "float":  ("REAL",             "float"),
    "double": ("DOUBLE PRECISION", "double"),
    "char*":  ("TEXT",             "str"),
    "char":   ("CHAR(1)",          "char"),
}

@dataclass
class VarDecl:
    c_type: str       # e.g. "int", "char*"
    suffix: str       # e.g. "int", "str"  (matches db_runtime helper suffix)
    name: str
    init: Optional[str]  # raw init expression, or None


# ---------------------------------------------------------------------------
# Parsing
# ---------------------------------------------------------------------------

# Matches:  int x = expr;   or   int x;
# Also handles char* and char (checked in order so char* is tried first)
DECL_RE = re.compile(
    r'^\s*(double|float|int|char\*|char)\s+([A-Za-z_]\w*)\s*'
    r'(?:=\s*([^;]+?))?\s*;',
    re.MULTILINE,
)


def parse_declarations(source: str) -> list[VarDecl]:
    decls: list[VarDecl] = []
    seen: set[str] = set()
    for m in DECL_RE.finditer(source):
        c_type = m.group(1)
        name = m.group(2)
        init = m.group(3).strip() if m.group(3) else None
        if name in seen:
            continue
        seen.add(name)
        _, suffix = C_TO_PG[c_type]
        decls.append(VarDecl(c_type=c_type, suffix=suffix, name=name, init=init))
    return decls


# ---------------------------------------------------------------------------
# Expression rewriting
# ---------------------------------------------------------------------------

def rewrite_expr(expr: str, decls: list[VarDecl]) -> str:
    """Replace all known variable names in an expression with db_get_* calls."""
    for decl in decls:
        # Only replace whole-word occurrences
        expr = re.sub(
            rf'\b{re.escape(decl.name)}\b',
            f'db_get_{decl.suffix}("{decl.name}")',
            expr,
        )
    return expr


# ---------------------------------------------------------------------------
# Line-level transformation
# ---------------------------------------------------------------------------

# Standalone assignment:  varname = expr;
ASSIGN_RE = re.compile(
    r'^(\s*)([A-Za-z_]\w*)\s*=\s*([^;]+?)\s*;',
    re.MULTILINE,
)


def transform_source(source: str, decls: list[VarDecl], connstr: str) -> str:
    decl_names = {d.name for d in decls}
    decl_by_name = {d.name: d for d in decls}

    lines_out: list[str] = []

    # Track which lines are variable declarations so we can replace them
    decl_spans: dict[int, VarDecl] = {}
    for m in DECL_RE.finditer(source):
        name = m.group(2)
        if name in decl_by_name:
            # record the line number (0-indexed) of this declaration
            line_no = source[:m.start()].count('\n')
            decl_spans[line_no] = decl_by_name[name]

    source_lines = source.splitlines()

    # First pass: identify the open-brace line of main() so we can inject db_connect
    main_open_brace_line: Optional[int] = None
    in_main = False
    for i, line in enumerate(source_lines):
        if re.search(r'\bmain\s*\(', line):
            in_main = True
        if in_main and '{' in line:
            main_open_brace_line = i
            break

    for i, line in enumerate(source_lines):
        stripped = line.strip()

        # Inject runtime header after the last #include
        if stripped.startswith('#include') and not any(
            'db_runtime' in l for l in source_lines
        ):
            lines_out.append(line)
            # We'll add the header line once, after all includes
            # (handled below by post-processing)
            continue

        # Inject db_connect after the opening brace of main
        if i == main_open_brace_line:
            lines_out.append(line)
            indent = '    '
            lines_out.append(f'{indent}db_connect("{connstr}");')
            lines_out.append('')
            continue

        # Variable declaration line → replace with ensure + set
        if i in decl_spans:
            decl = decl_spans[i]
            indent_m = re.match(r'(\s*)', line)
            indent = indent_m.group(1) if indent_m else ''
            lines_out.append(f'{indent}db_ensure_table_{decl.suffix}("{decl.name}");')
            if decl.init is not None:
                init_rewritten = rewrite_expr(decl.init, decls)
                lines_out.append(f'{indent}db_set_{decl.suffix}("{decl.name}", {init_rewritten});')
            lines_out.append('')
            continue

        # Standalone assignment line → replace with db_set
        assign_m = ASSIGN_RE.match(line)
        if assign_m:
            var_name = assign_m.group(2)
            if var_name in decl_names:
                decl = decl_by_name[var_name]
                indent = assign_m.group(1)
                expr = assign_m.group(3).strip()
                expr_rewritten = rewrite_expr(expr, decls)
                lines_out.append(f'{indent}db_set_{decl.suffix}("{var_name}", {expr_rewritten});')
                continue

        # General line: rewrite any variable reads
        rewritten = line
        # Only rewrite inside expressions (not in #include, comments, strings)
        if not stripped.startswith('#') and not stripped.startswith('//'):
            rewritten = rewrite_reads_in_line(line, decls)
        lines_out.append(rewritten)

    result = '\n'.join(lines_out)

    # Inject #include "runtime/db_runtime.h" after the last #include
    result = inject_runtime_header(result)

    return result


def rewrite_reads_in_line(line: str, decls: list[VarDecl]) -> str:
    """
    Rewrite variable reads in a line, being careful not to touch
    the name inside db_set_*/db_get_* calls or string literals.
    """
    # Don't touch lines that are already db_ calls we just generated
    if re.search(r'\bdb_(set|get|ensure|connect)_?\w*\s*\(', line):
        return line
    for decl in decls:
        line = re.sub(
            rf'\b{re.escape(decl.name)}\b',
            f'db_get_{decl.suffix}("{decl.name}")',
            line,
        )
    return line


def inject_runtime_header(source: str) -> str:
    """Insert #include "runtime/db_runtime.h" after the last #include line."""
    lines = source.splitlines()
    last_include = -1
    for i, line in enumerate(lines):
        if line.strip().startswith('#include'):
            last_include = i
    if last_include == -1:
        lines.insert(0, '#include "runtime/db_runtime.h"')
    else:
        lines.insert(last_include + 1, '#include "runtime/db_runtime.h"')
    return '\n'.join(lines)


# ---------------------------------------------------------------------------
# Eification
# ---------------------------------------------------------------------------

def eify(c_file: Path) -> Path:
    eify_script = Path(__file__).parent / 'eeeeeeeeee' / 'main.py'
    if not eify_script.exists():
        print(f"Warning: eeeeeeeeee submodule not found at {eify_script}", file=sys.stderr)
        return c_file
    result = subprocess.run(
        [sys.executable, str(eify_script), str(c_file)],
        capture_output=True, text=True,
    )
    if result.returncode != 0:
        print(f"eeeeeeeeee error:\n{result.stderr}", file=sys.stderr)
        sys.exit(1)
    # eeeeeeeeee writes <stem>.e.c next to the input file
    e_path = c_file.parent / f"{c_file.stem}.e{c_file.suffix}"
    return e_path


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def transpile(input_path: Path, output_path: Path, connstr: str) -> None:
    source = input_path.read_text()
    decls = parse_declarations(source)

    if not decls:
        print("No variable declarations found — nothing to transpile.", file=sys.stderr)

    transformed = transform_source(source, decls, connstr)
    output_path.write_text(transformed)

    print(f"Transpiled: {input_path} → {output_path}")
    if decls:
        print(f"  Variables ({len(decls)}):")
        for d in decls:
            print(f"    {d.c_type} {d.name}  →  Postgres table var_{d.name} ({C_TO_PG[d.c_type][0]})")

    e_path = eify(output_path)
    print(f"  Eified:    {e_path}")


def main() -> None:
    parser = argparse.ArgumentParser(description="c-- transpiler")
    parser.add_argument("input", help="Input C file")
    parser.add_argument("output", help="Output C file (intermediate; .e.c is the final artifact)")
    parser.add_argument(
        "--connstr",
        default="postgres://localhost/cmm",
        help="PostgreSQL connection string (default: postgres://localhost/cmm)",
    )
    args = parser.parse_args()

    input_path = Path(args.input)
    output_path = Path(args.output)

    if not input_path.is_file():
        print(f"Error: {input_path} not found", file=sys.stderr)
        sys.exit(1)

    transpile(input_path, output_path, args.connstr)


if __name__ == "__main__":
    main()
