// clientFileManager.cpp : Defines the entry point for the application.
//

#include "clientFileManager.h"
#include "client.h"
#include <locale>
#include <iostream> 
#include <future>
#include <thread>
#include <chrono>

using namespace std;

static string getAnswer()
{
	string answer;
	cin >> answer;
	return answer;
}

int main()
{
	SetConsoleCP(1251);
	SetConsoleOutputCP(1251);
	try {
		boost::asio::io_context io_context;

		// Создаем клиента
		auto client = std::make_shared<TcpClient>(io_context, "C:/Users/MAKSMSD/Desktop/outClient");
		client.get()->start("127.0.0.1", "200");
		// Запуск 

		io_context.run();	
	}
	catch (const std::exception& e) {
		std::cerr << "Ошибка: " << e.what() << "\n";
	}

	return 0;
}
