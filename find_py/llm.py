from langchain_core.chat_history import InMemoryChatMessageHistory
from langchain_core.output_parsers import StrOutputParser
from langchain_core.prompts import ChatPromptTemplate
from langchain_core.runnables import RunnableWithMessageHistory
from langchain_openai import ChatOpenAI

class Chatbot:

    def __init__(self, api_key, base_url, model):
        self.history = InMemoryChatMessageHistory()
        self.llm_config = ChatOpenAI(
            api_key=api_key,
            base_url=base_url,
            model=model
        )
        self.prompt = None
        self.chain = None
        self.wrapped_chain = None

    def set_background_message(self, input_text):
        background_message = input_text
        self.prompt = ChatPromptTemplate(
            [
                ("system", background_message),
                ("placeholder", "{chat_history}"),
                ("human", "{input}"),
            ]
        )
        chain = self.prompt | self.llm_config | StrOutputParser()
        self.wrapped_chain = RunnableWithMessageHistory(
            chain,
            self.get_history,
            history_messages_key="chat_history",
        )

    def get_history(self):
        return self.history

    def get_response(self, input_text):
        print("prompt: " + input_text)
        if not self.wrapped_chain:
            raise ValueError("The chatbot is not initialized. Call `set_background_message` first.")
        return self.wrapped_chain.invoke({"input": input_text})


def main():
    print("Welcome to the Chatbot! Type 'exit' to quit.")

    chatbot = Chatbot(
        api_key='sk-CyDXPaWtLzftviXwCQtZZAAYO4EuvhQQ4nzBzEwy8I7xIEvx',
        base_url='https://api.chatanywhere.tech/v1',
        model='gpt-4o-mini'
    )
    chatbot.set_background_message("You are a good helper!")

    try:
        while True:
            user_input = input("You: ").strip()
            if user_input.lower() == 'exit':
                print("Exiting... Goodbye!")
                break
            response = chatbot.get_response(user_input)
            print("LLM: " + response)
    except KeyboardInterrupt:
        print("\nExiting... Goodbye!")
    except Exception as e:
        print(f"An error occurred: {e}")


if __name__ == '__main__':
    main()
