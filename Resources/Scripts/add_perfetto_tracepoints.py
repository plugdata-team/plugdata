#!/usr/bin/env python

# This script insert Perfetto TRACE_COMPONENT() calls at top of matched function bodies in .cpp files
# Requires latest libclang bindings (>= 19.1.3)
# Either install from LLVM source or use unofficial `libclang` package on pip
# Please put path to libclang .dll/.so at command line !!!

import clang.cindex
import re
import sys
import argparse
import os

parser = argparse.ArgumentParser(description="Insert perfetto tracepoints in given .cpp files. NOTE: Due to how the libclang works it won't work on .h files!")
parser.add_argument('filenames', nargs='+', help='.cpp file(s) or directories to add to')
parser.add_argument('--libclang-path', '-l', help='Path to libclang .dll/.so', default="C:/Program Files/LLVM/bin/libclang.dll")
parser.add_argument('--regex', '-r', help='The regular expression of function name to match', default=".*")
parser.add_argument('--macro', '-m', help='Macro/code to add at top of functions', default="TRACE_COMPONENT();")
parser.add_argument('--dry-run', '-d', help='Do not modify code', action="store_true")
args = parser.parse_args()

PATTERN = re.compile(args.regex)

def insert_tracepoint_in_functions(filename):
    global PATTERN, args

    print(f"===== {filename} =====")

    clang.cindex.Config.set_library_file(args.libclang_path)
    index = clang.cindex.Index.create()
    translation_unit = index.parse(filename)
    rewriter = clang.cindex.Rewriter.create(translation_unit)

    insertions = []
    for node in translation_unit.cursor.walk_preorder():
        if node.kind in [clang.cindex.CursorKind.CXX_METHOD, clang.cindex.CursorKind.CONSTRUCTOR, clang.cindex.CursorKind.DESTRUCTOR]:
            if node.is_definition() and PATTERN.search(node.spelling):
                body = next(iter(x for x in node.get_children() if x.kind == clang.cindex.CursorKind.COMPOUND_STMT))
                if body:
                    # Check if already trace this method body
                    already_traced = False
                    for n in body.walk_preorder():
                        # (vvv) What libclang reports for macro insertion (vvv)
                        if n.kind == clang.cindex.CursorKind.OVERLOADED_DECL_REF and args.macro.startswith(n.spelling):
                            already_traced = True
                            break

                    if already_traced:
                        print(f"* Already traced: {node.spelling}")
                        continue

                    # Search for opening '{' token, insert after
                    tokens = list(body.get_tokens())
                    for i, token in enumerate(tokens):
                        if token.kind == clang.cindex.TokenKind.PUNCTUATION and token.spelling == "{" and i < len(tokens) - 1:
                            pos = tokens[i+1].extent.start
                            print(f"+ Tracing {node.spelling} at line {pos.line}")
                            insertions.append(pos)
                            break

    for pos in insertions:
        # Maintain whitespace at beginning of next node pushed to next line
        ws = " " * max(pos.column - 1, 0)
        rewriter.insert_text_before(pos, f"{args.macro}\n\n{ws}")

    if args.dry_run:
        print("!!! No files changed for dry run !!!")
    else:
        rewriter.overwrite_changed_files()


if __name__ == "__main__":
    if not os.path.isfile(args.libclang_path):
        raise RuntimeError(f"Can't find libclang dll/so library at {args.libclang_path}, please pass with -l <path/to/libclang.dll> on commandline !")

    # If its file, read the file, else if directory read the .cpp files in it
    for filename in args.filenames:
        if os.path.isfile(filename):
            insert_tracepoint_in_functions(filename)
        elif os.path.isdir(filename):
            for root, dirs, files in os.walk(filename):
                for fn in files:
                    if fn.endswith('.cpp'):
                        insert_tracepoint_in_functions(os.path.join(root, fn))
        else:
            print(f"!!! Invalid file: {filename} !!!")
