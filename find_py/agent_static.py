from llm import Chatbot
from utility_find import find

chatbot = Chatbot(
    api_key='sk-CyDXPaWtLzftviXwCQtZZAAYO4EuvhQQ4nzBzEwy8I7xIEvx',
    base_url='https://api.chatanywhere.tech/v1',
    model='gpt-4o-mini'
)

system_prompt = '''
This is how find work:
def find(
    folders: List[str],
    follow_symlink_signal: int = 0,
    process_dir_first: bool = True,
    timeout: Optional[double] = None,
    agent_helper: Optional[Callable[[str], str]] = None
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
    timeout (Optional[int], default None):
        Specifies the number of seconds before a timeout occurs. If provided, a timer will be started, and a callback (via llm_helper) will be triggered if the timeout is reached.

    Timeout Callback (llm_helper):
        Task: When a timeout occurs, specific code needs to be executed. 
            Prepare this code in advance within the agent_helper_static function. For example, you may modify find_context.max_depth
            Then, return the code as a string.
    llm_helper
        ** MUST use exp: agent_helper=agent_helper_static **
You need implement agent_helper_static in advance, it will be used later.
You can use the find function to achieve the goal. No define, just use!
Please only use python code to finish my request, no markdown, no other explanations!
'''

chatbot.set_background_message(system_prompt)

llm_ret = chatbot.get_response('Search ~ to find all the files. If the time exceeds 0.001 second, modify search_max_depth to 10')
print("From LLM, code to be executed:\n  " + llm_ret)
exec(llm_ret)