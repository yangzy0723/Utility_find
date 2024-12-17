import argparse
import fnmatch
import logging
import os
import threading

from typing import Callable, List, Optional

from context import FindContext

# logging.basicConfig(level=logging.INFO, format="%(asctime)s - %(levelname)s - %(message)s")

f = open("res.txt", "w")
def out_put(message: str, op: int):
    if op == 0:
        print(message)
    elif op == 1:
        f.write(message + '\n')
        find_context.has_result = True

def start_timer(timeout_seconds: int, callback: Callable):
    timer = threading.Timer(timeout_seconds, callback)
    timer.start()
    return timer

def walk_dir(
        directory: str,
        follow_symlink_flag: bool,
        process_dir_first: bool,
        depth: int,
        context: FindContext,
        name_pattern: Optional[str] = None
):
    try:
        entries = os.listdir(directory)
    except (OSError, FileNotFoundError, PermissionError) as e:
        logging.error(f"Error accessing {directory}: {e}")
        return

    if not process_dir_first:
        out_put(directory, 0)
        if name_pattern is None or fnmatch.fnmatch(directory, name_pattern):
            out_put(directory, 1)

    for entry in entries:
        entry_path = os.path.join(directory, entry)
        try:
            if os.path.islink(entry_path) and not follow_symlink_flag:
                out_put(entry_path, 0)
                if name_pattern is None or fnmatch.fnmatch(entry, name_pattern):
                    out_put(entry_path, 1)
                continue

            if os.path.isdir(entry_path):
                if context.max_depth < 0 or depth < context.max_depth:
                    walk_dir(
                        directory=entry_path,
                        follow_symlink_flag=follow_symlink_flag,
                        process_dir_first=process_dir_first,
                        depth=depth + 1,
                        context=context,
                        name_pattern=name_pattern
                    )
            else:
                out_put(entry_path, 0)
                if name_pattern is None or fnmatch.fnmatch(entry, name_pattern):
                    out_put(entry_path, 1)
        except (OSError, FileNotFoundError, PermissionError) as e:
            logging.error(f"Error processing {entry_path}: {e}")

    if process_dir_first:
        out_put(directory, 0)
        if name_pattern is None or fnmatch.fnmatch(directory, name_pattern):
            out_put(directory, 1)

find_context = None

def find(
    folders: List[str],
    follow_symlink_signal: Optional[int] = 0,
    process_dir_first: Optional[bool] = False,
    name: Optional[str] = None,
    timeout: Optional[int] = None,
    search_depth: Optional[int] = -1,
    agent_helper: Optional[Callable[[str], None]] = None
):

    visited = set()

    global find_context
    find_context = FindContext()
    find_context.max_depth = search_depth

    follow_symlink_flag = follow_symlink_signal == 2

    if timeout is not None:
        timer = start_timer(timeout, lambda: find_context.handle_timeout(agent_helper))

    for folder in folders:
        folder = os.path.abspath(os.path.expanduser(folder))    # expanduser：handle cases like '~'

        if not os.path.exists(folder):
            logging.error(f"Error: The directory {folder} does not exist!")
            continue

        if folder in visited:
            continue

        visited.add(folder)

        if os.path.islink(folder) and not follow_symlink_signal:
            out_put(folder, 0)
            if name is None or fnmatch.fnmatch(folder, name):
                out_put(folder, 1)
            continue

        if os.path.isdir(folder):
            walk_dir(
                directory=folder,
                follow_symlink_flag=follow_symlink_flag,
                process_dir_first=process_dir_first,
                name_pattern=name,
                depth=1,
                context=find_context
            )
        else:
            out_put(folder, 0)
            if name is None or fnmatch.fnmatch(folder, name):
                out_put(folder, 1)

    if timeout is not None and timer.is_alive():
        timer.cancel()

def main():
    parser = argparse.ArgumentParser(description="Custom find command simulation.")

    parser.add_argument("folders", nargs="*", default=["."], help="Folders to search.")

    group = parser.add_mutually_exclusive_group()
    group.add_argument("-H", action="store_true", help="Do not follow symlinks unless specified on command line.")
    group.add_argument("-L", action="store_true", help="Follow symlinks.")
    group.add_argument("-P", action="store_true", help="Never follow symlinks (default).")

    parser.add_argument("-d", action="store_true", help="Process directories before their contents.")

    parser.add_argument("-name", help="Filter results by file or directory name pattern.")

    args = parser.parse_args()

    # Determine symlink behavior
    follow_symlink = 0
    if args.L:
        follow_symlink = 2
    elif args.H:
        follow_symlink = 1

    find(
        folders=args.folders,
        follow_symlink_signal=follow_symlink,
        process_dir_first=args.d,
        name=args.name,
        timeout=1
    )

if __name__ == "__main__":
    main()
