import re
import os
import sys
from operator import contains


def find_restore_and_coords(lines):
    restore_pattern = re.compile(r'#X restore (\d+) (\d+) graph;?')
    coords_pattern = re.compile(
        r'#X coords [-\d]+ [-\d]+ [-\d]+ [-\d]+ (\d+) (\d+) .+')

    restore_x, restore_y = None, None
    size_w, size_h = None, None

    for i, line in enumerate(lines):
        restore_match = restore_pattern.match(line)
        if restore_match:
            restore_x, restore_y = map(int, restore_match.groups())
            if i > 0:
                coords_match = coords_pattern.match(lines[i - 1])
                if coords_match:
                    size_w, size_h = map(int, coords_match.groups())
                    return restore_x, restore_y, size_w, size_h

    return None, None, None, None


def update_coords(lines, restore_x, restore_y, size_w, size_h):
    coords_pattern = re.compile(r'#X coords .+')
    coords_line = len(lines)-1
    if coords_pattern.match(lines[coords_line]):
        lines[coords_line] = f'#X coords 0 0 1 1 {size_w} {size_h} 1 {restore_x} {restore_y};\n'


def process_patch(file_path):
    with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
        lines = f.readlines()

    lines[-1].rstrip();

    restore_x, restore_y, size_w, size_h = find_restore_and_coords(lines)
    if restore_x is not None and size_w is not None:
        update_coords(lines, restore_x, restore_y, size_w, size_h)

    with open(file_path, 'w') as f:
        f.writelines(lines)


def process(folder_path):
    for root, _, files in os.walk(folder_path):
        for file in files:
            if file.endswith(".m~.pd") and "brane" not in file:
                process_patch(os.path.join(root, file))


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python update_pd_patch.py <folder_path>")
    else:
        process_folder(sys.argv[1])
