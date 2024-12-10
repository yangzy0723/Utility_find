import multiprocessing

from llm import Chatbot
from utility_find import find

timeout = 1
depth = 5
name_pattern = "*mlsys*"

system_prompt = '''
This is how find work:
def find(
    folders: List[str],
    follow_symlink_signal: int = 0,
    process_dir_first: bool = True,
    name: Optional[str] = None,
    search_depth: Optional[int] = -1
) -> None:
Parameters:
    - folders (List[str]):
        A list of directory paths to search through.
        If empty, the function defaults to searching the current working directory (".").
    - follow_symlink_signal (int, default 0):
        A signal indicating how symlinks should be handled. It can take one of the following values:
            0: Do not follow symlinks (default behavior)
            1: Follow symlinks only if specified via the command-line
            2: Follow symlinks unconditionally
    - process_dir_first (bool, default True):
        If True, directories will be processed first (printed before their contents).
        If False, directories will be processed after their contents (printed after traversal).
    - name (str, optional):
        The file need to be matched.
        Wildcard expressions can be used to satisfy fields.
    - search_depth (int, optional):
        The maximum search depth for recursive search, -1 represents unlimited search depth.
        
You can use the find function to achieve the goal. No define, just use!
Please only use python code to finish my request.
No markdown! No other explanations!
'''
chatbot = Chatbot(
    api_key='sk-CyDXPaWtLzftviXwCQtZZAAYO4EuvhQQ4nzBzEwy8I7xIEvx',
    base_url='https://api.chatanywhere.tech/v1',
    model='gpt-4o-mini'
)
chatbot.set_background_message(system_prompt)
llm_ret = chatbot.get_response(f'''
Search ~ to find files with the {name_pattern} field in their names.
You may use multithread programming.
If the time exceeds {timeout} second, stop the search and modify search_max_depth to {depth}.
''')
print("From LLM, code to be executed:\n  " + llm_ret)
exec (llm_ret)

# def target_find(_depth):
#     find(["~"], name = name_pattern, search_depth=_depth)
#
# def execute_with_timeout(_depth, _timeout):
#     process = multiprocessing.Process(target=target_find, args=(_depth if _depth is not None else -1,))
#     process.start()
#     if _timeout is not None:
#         process.join(_timeout)
#         if process.is_alive():
#             process.terminate()
#
# execute_with_timeout(_depth=None, _timeout=timeout)
# execute_with_timeout(_depth=depth, _timeout=None)