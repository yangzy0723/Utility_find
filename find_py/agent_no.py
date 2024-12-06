import multiprocessing

from llm import Chatbot
from utility_find import find

timeout = 10
depth = 3

system_prompt = '''
This is how find work:
def find(
    folders: List[str],
    follow_symlink_signal: int = 0,
    process_dir_first: bool = True,
    search_depth: Optional[int] = None
) -> None:
Parameters:
    folders (List[str]):
        A list of directory paths to search through. If empty, the function defaults to searching the current working directory (".").
    follow_symlink_signal (int, default 0):
        A signal indicating how symlinks should be handled. It can take one of the following values:
            0: Do not follow symlinks (default behavior).
            1: Follow symlinks only if specified via the command-line argument -H.
            2: Follow symlinks unconditionally, as specified by the command-line argument -L.
    process_dir_first (bool, default True):
        If True, directories will be processed first (printed before their contents). If False, directories will be processed after their contents (printed after traversal).
        
Please only use python code to finish my request, no markdown, no other explanations!
You code should be executed by exec().
'''
chatbot = Chatbot(
    api_key='sk-CyDXPaWtLzftviXwCQtZZAAYO4EuvhQQ4nzBzEwy8I7xIEvx',
    base_url='https://api.chatanywhere.tech/v1',
    model='gpt-4o-mini'
)
chatbot.set_background_message(system_prompt)
llm_ret = chatbot.get_response(f'''
    You need to use the find function to search the directory  ~ within a given time {timeout}s. 
    If the search is not completed, set the search depth to {depth} and search again.
    Maybe you can use multiprocessing. If timeout, just terminate the sub_thread and restart with a given depth.
''')
print("From LLM, code to be executed:\n  " + llm_ret)
# exec (llm_ret)

def target_find(_depth):
    find(["~"], search_depth=_depth)

def execute_with_timeout(_depth, _timeout):
    process = multiprocessing.Process(target=target_find, args=(_depth if _depth is not None else -1,))
    process.start()
    if _timeout is not None:
        process.join(_timeout)
        if process.is_alive():
            process.terminate()

execute_with_timeout(_depth=None, _timeout=timeout)
execute_with_timeout(_depth=depth, _timeout=None)