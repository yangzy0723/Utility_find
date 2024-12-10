from langchain_core.chat_history import InMemoryChatMessageHistory
from langchain_core.output_parsers import StrOutputParser
from langchain_core.prompts import ChatPromptTemplate
from langchain_core.runnables import RunnableWithMessageHistory
from langchain_openai import ChatOpenAI

class Chatbot:
    def __init__(self, api_key, base_url, model='gpt-4o-mini'):
        """
        Initialize the Chatbot instance.
        :param api_key: API key for accessing the language model.
        :param base_url: Base URL for the API.
        :param model: Model name (default: 'gpt-4o-mini').
        """
        self.history = InMemoryChatMessageHistory()
        self.llm_config = ChatOpenAI(api_key=api_key, base_url=base_url, model=model)
        self.wrapped_chain = None

    def set_background_message(self, background_message):
        """
        Set the system message and initialize the prompt.
        :param background_message: Background message for the conversation context.
        """
        prompt = ChatPromptTemplate(
            [
                ("system", background_message),
                ("placeholder", "{chat_history}"),
                ("human", "{input}"),
            ]
        )
        chain = prompt | self.llm_config | StrOutputParser()
        self.wrapped_chain = RunnableWithMessageHistory(
            chain,
            self.get_history,
            history_messages_key="chat_history",
        )

    def get_history(self):
        """Return the chat history."""
        return self.history

    def get_response(self, input_text):
        """
        Get the model's response for a given input.
        :param input_text: User's input text.
        :return: Model's response.
        """
        print("prompt: " + input_text)
        if not self.wrapped_chain:
            raise ValueError("The chatbot is not initialized. Call `set_background_message` first.")
        return self.wrapped_chain.invoke({"input": input_text})

    def run(self):
        """
        Run the chatbot in an interactive loop.
        """
        print("Welcome to the Chatbot! Type 'exit' to quit.")

        try:
            while True:
                user_input = input("You: ").strip()
                if user_input.lower() == 'exit':
                    print("Exiting... Goodbye!")
                    break
                response = self.get_response(user_input)
                print("LLM: " + response)
        except KeyboardInterrupt:
            print("\nExiting... Goodbye!")
        except Exception as e:
            print(f"An error occurred: {e}")

if __name__ == '__main__':
    # Initialize the chatbot with API credentials and model configurations
    chatbot = Chatbot(
        api_key='sk-CyDXPaWtLzftviXwCQtZZAAYO4EuvhQQ4nzBzEwy8I7xIEvx',
        base_url='https://api.chatanywhere.tech/v1',
    )

    # Set the initial background message
    chatbot.set_background_message("You are a helpful assistant!")

    # Run the chatbot
    chatbot.run()
