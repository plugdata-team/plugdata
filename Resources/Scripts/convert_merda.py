import re
import os
import sys
from operator import contains


def find_restore_and_coords(lines):
    restore_pattern = re.compile(r'#X restore (\d+) (\d+)?')
    coords_pattern = re.compile(r'#X coords 0 0 1 1 (\d+) (\d+) .+')
    coords_pattern2 = re.compile(r'#X coords 0 -1 1 1 (\d+) (\d+) .+')
    coords_pattern3 = re.compile(r'#X coords [-\d]+ [-\d]+ [-\d]+ [-\d]+ (\d+) (\d+) .+')

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
                coords_match2 = coords_pattern2.match(lines[i - 1])
                if coords_match2:
                    size_w, size_h = map(int, coords_match2.groups())
                    return restore_x, restore_y, size_w, size_h
    for i, line in enumerate(lines):
        restore_match = restore_pattern.match(line)
        if restore_match:
            restore_x, restore_y = map(int, restore_match.groups())
            if i > 0:
                coords_match3 = coords_pattern3.match(lines[i - 1])
                if coords_match3:
                    size_w, size_h = map(int, coords_match3.groups())
                    return restore_x, restore_y, size_w, size_h

    return None, None, None, None


def update_coords(lines, restore_x, restore_y, size_w, size_h):
    coords_pattern = re.compile(r'#X coords .+')
    coords_line = len(lines)-1
    if coords_pattern.match(lines[coords_line]):
        lines[coords_line] = f'#X coords 0 0 1 1 {size_w} {size_h} 1 {restore_x} {restore_y};\n'

    goprect_pattern = re.compile(
        r'(#X obj \d+ \d+ else/message \\; \\\$0-\S+ readonly 0 .+? goprect )\d+ \d+ \d+ \d+(.*)',
            re.DOTALL
    )
    goprect_pattern2 = re.compile(
        r'(#X obj \d+ \d+ else/message \\; \\\$0-\S+ readonly 1 .+? goprect )(\d+) (\d+) (\d+) (\d+)(.*)',
            re.DOTALL
    )
    for i, line in enumerate(lines):
        m = goprect_pattern.match(line)
        if m:
            lines[i] = f'{m.group(1)}{restore_x} {restore_y} {size_w} {size_h}{m.group(2)}\n'
        n = goprect_pattern2.match(line)
        if n:
            lines[i] = f'{n.group(1)}{int(n.group(2)) + 8} {int(n.group(3)) + 7} {int(n.group(4)) - 16} {int(n.group(5)) - 14}{n.group(6)}\n'

def process_patch(file_path, out_path):
    if "!" in  file_path:
        return

    with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
        lines = f.readlines()

    if (file_path.endswith(".m~.pd") or file_path.endswith(".m.pd")) and "brane" not in file_path:
        lines[-1].rstrip()
        restore_x, restore_y, size_w, size_h = find_restore_and_coords(lines)
        if restore_x is not None and size_w is not None:
            update_coords(lines, restore_x, restore_y, size_w, size_h)

    with open(out_path, 'w', encoding='utf-8') as f:
        f.writelines(lines)


def process(folder_path, out_path):
    os.mkdir(out_path)
    for root, _, files in os.walk(folder_path):
        for file in files:
            process_patch(os.path.join(root, file), os.path.join(out_path, file))


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python update_pd_patch.py <folder_path> <out_path>")
    else:
        process(sys.argv[1], sys.argv[2])
