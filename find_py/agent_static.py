import time

from llm import Chatbot

chatbot = Chatbot(
    api_key='sk-CyDXPaWtLzftviXwCQtZZAAYO4EuvhQQ4nzBzEwy8I7xIEvx',
    base_url='https://api.chatanywhere.tech/v1',
    model='gpt-4o-mini'
)

system_prompt = '''
This is how find work:
def find(
    folders: List[str],
    follow_symlink_signal: Optional[int] = 0,
    process_dir_first: Optional[bool] = False,
    name: Optional[str] = None,
    timeout: Optional[int] = None,
    agent_helper: Optional[Callable[[str], None]] = None
) -> None:
Parameters:
    folders (List[str]):
        A list of directory paths to search through.
        If empty, the function defaults to searching the current working directory (".").
    follow_symlink_signal (int, default 0):
        A signal indicating how symlinks should be handled. It can take one of the following values:
            0: Do not follow symlinks (default behavior)
            1: Follow symlinks only if specified via the command-line
            2: Follow symlinks unconditionally
    process_dir_first (bool, default True):
        If True, directories will be processed first (printed before their contents).
        If False, directories will be processed after their contents (printed after traversal).
    name (str, optional):
        The file need to be matched.
        Wildcard expressions can be used to satisfy fields.
    timeout (Optional[int], default None):
        Specifies the number of seconds before a timeout occurs.
        If provided, a timer will be started, and a callback (via agent_helper) will be triggered if the timeout is reached.

    agent_helper(Timeout Callback):
        Task: When a timeout occurs, specific code needs to be executed. 
            Please define the agent_helper_static function to achieve the request.
            There are some functions you can use directly:
                ```python
                set_search_depth(_d: int) # change search depth to _d
                ```
            Then, return the code as a ** string **.
    agent_helper
        MUST use exp: agent_helper=agent_helper_static
        
You need implement agent_helper_static in advance, it will be used later.
You can use the find function to achieve the goal. No define, just use!
Please only use python code to finish my request.
No markdown! No other explanations!
'''
chatbot.set_background_message(system_prompt)

# start_time = time.time()
llm_ret = chatbot.get_response('''
    Search ~ to find all files with the "mlsys" field in their names.
    If the time exceeds 60 seconds, modify search_max_depth to 5.
''')
# end_time = time.time()
# execution_time = end_time - start_time
# print(f"Time taken to get response from LLM: {execution_time:.2f} seconds")

print("------------ code to be executed -----------------")
print(llm_ret)
print("--------------------------------------------------")

from utility_find import find
exec(llm_ret)