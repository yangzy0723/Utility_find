from llm import Chatbot

timeout = 2
depth = -1
name_pattern = "mlsys"

chatbot = Chatbot(
    api_key='sk-CyDXPaWtLzftviXwCQtZZAAYO4EuvhQQ4nzBzEwy8I7xIEvx',
    base_url='https://api.chatanywhere.tech/v1',
    model='gpt-4o-mini'
)

system_prompt = '''
You are a fantastic python coder.
Please help me implement the find method that traverses a given folder to search for files matching a specified pattern while recording the traversal time.
If the traversal time exceeds a specified timeout, the traversal depth will be adjusted to a specified value, and the traversal will continue.
    - folders (List[str]):
        A list of directory paths to search through. Pay attention to ~.
        If empty, the function defaults to searching the current working directory (".").
    - timeout(int, optional):
        When timeout occurs, something would happen.
    - name (str, optional):
        The file need to be matched.
        Wildcard expressions can be used to satisfy fields.
    - search_depth (int, optional):
        The maximum search depth for recursive search, -1 represents unlimited search depth.
No markdown! No explanations!
'''
chatbot.set_background_message(system_prompt)

# llm_ret = chatbot.get_response(f'''
#    Find files under the ~ directory with filenames containing the field {name_pattern} and print them to the terminal.
#    Initially, search for {timeout} second with unlimited depth. When a timeout occurs, change the search depth to {depth}.
# ''')
llm_ret = chatbot.get_response(f'''
    Find all files under the ~ directory and print them to the terminal.
    Initially, search for {timeout} second with unlimited depth. When a timeout occurs, change the search_depth to {depth} and keep searching.
''')
print("------------ code to be executed -----------------")
print(llm_ret)
print("--------------------------------------------------")

exec(llm_ret)