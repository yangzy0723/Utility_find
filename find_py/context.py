import inspect
import logging
import time

from typing import Optional, Callable

def set_search_depth(search_depth: int):
    from utility_find import find_context
    find_context.max_depth = search_depth

def execute_llm_code(code_str, globals):
    print("-------------- this is code str-------------------")
    print(code_str)
    print("--------------------------------------------------")
    exec_globals = {'__builtins__': None}    # Create a restricted execution environment.
    exec_globals.update(globals)

    try:
        exec(code_str, exec_globals)
    except Exception as e:
        print("Error executing LLM-generated code:", e)
        raise
    return exec_globals


class FindContext:
    def __init__(self, max_depth: int = -1):
        self.has_result = False
        self.max_depth = max_depth
        self.timeout_occurred = False
        self.allowed_functions = {'set_search_depth': set_search_depth}

    def handle_timeout(self, agent_helper: Optional[Callable[[Optional[str]], str]]):
        print("Timeout")
        from utility_find import find_context
        self.timeout_occurred = True
        if not self.has_result:
            if agent_helper is None:
                logging.warning('''
                    Timeout occurred, no result...
                    No Agent Helper provided.
                    Skipping!
                ''')
                return
            else:
                if len(inspect.signature(agent_helper).parameters) > 0:
                    prompt = "A timeout event has occurred, please take appropriate action."
                    # start_time = time.time()
                    agent_ret = agent_helper(prompt)
                    # end_time = time.time()
                    # execution_time = end_time - start_time
                    # print(f"Time taken to get response from LLM: {execution_time:.2f} seconds")
                else:
                    agent_ret = agent_helper()
                execute_llm_code(agent_ret, self.allowed_functions)