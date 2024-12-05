import inspect
import os
import argparse
import threading
from typing import List, Optional, Callable
import logging

# logging.basicConfig(level=logging.INFO, format="%(asctime)s - %(levelname)s - %(message)s")

OUTPUT_FILE = "res.txt"
f = open(OUTPUT_FILE, "w")
def custom_print(message: str):
    f.write(message + '\n')

find_context = None

class FindContext:
    def __init__(self, max_depth: int = -1):
        self.max_depth = max_depth
        self.timeout_occurred = False

    def handle_timeout(self, agent_helper: Optional[Callable[[Optional[str]], str]] = None):
        self.timeout_occurred = True
        if agent_helper is None:
            logging.warning("Timeout occurred. Setting max search depth to 0.")
            self.max_depth = 0
        else:
            if len(inspect.signature(agent_helper).parameters) > 0:
                prompt = "A timeout event has occurred, please take appropriate action."
                agent_ret = agent_helper(prompt)
            else:
                agent_ret = agent_helper()
            print("From agent, code to be executed:\n  " + agent_ret)
            global find_context
            exec(agent_ret)


def start_timer(timeout_seconds: int, callback: Callable):
    timer = threading.Timer(timeout_seconds, callback)
    timer.start()
    return timer

def walk_dir(directory: str, depth: int, context: FindContext, process_dir_first: bool, follow_symlinks: bool):
    try:
        entries = os.listdir(directory)
    except (OSError, FileNotFoundError, PermissionError) as e:
        logging.error(f"Error accessing {directory}: {e}")
        return

    if not process_dir_first:
        custom_print(directory)

    for entry in entries:
        entry_path = os.path.join(directory, entry)
        try:
            if os.path.islink(entry_path) and not follow_symlinks:
                custom_print(entry_path)
                continue

            if os.path.isdir(entry_path):
                if context.max_depth < 0 or depth < context.max_depth:
                    walk_dir(entry_path, depth + 1, context, process_dir_first, follow_symlinks)
            else:
                custom_print(entry_path)
        except (OSError, FileNotFoundError, PermissionError) as e:
            logging.error(f"Error processing {entry_path}: {e}")

    if process_dir_first:
        custom_print(directory)

def find(
    folders: List[str],
    follow_symlink_signal: int = 0,
    process_dir_first: bool = False,
    timeout: Optional[int] = None,
    agent_helper: Optional[Callable[[str], None]] = None
):
    visited = set()
    global  find_context
    find_context = FindContext()
    follow_symlinks = follow_symlink_signal == 2

    if timeout is not None:
        timer = start_timer(timeout, lambda: find_context.handle_timeout(agent_helper))

    for folder in folders:
        folder = os.path.abspath(os.path.expanduser(folder))    # Handle cases like '~'
        if not os.path.exists(folder):
            logging.error(f"Error: The directory {folder} does not exist!")
            continue

        if folder in visited:
            continue
        visited.add(folder)

        if os.path.islink(folder) and not follow_symlinks:
            custom_print(folder)
            continue

        if os.path.isdir(folder):
            walk_dir(folder, 1, find_context, process_dir_first, follow_symlinks)
        else:
            custom_print(folder)

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
        timeout=5,
        process_dir_first=args.d,
    )

if __name__ == "__main__":
    main()
