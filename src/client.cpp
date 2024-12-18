#include "client.h"

TcpClient::TcpClient(boost::asio::io_context& io_context, const std::string& base_dir)
	: resolver_(io_context), socket_(io_context),
	file_manager_(std::make_unique<FileHandler>(base_dir))
{
}

void TcpClient::start(const std::string& host, const std::string& port)
{
	std::cout << "Вызов конструктора TcpClient" << std::endl;
	resolver_.async_resolve(
		host, port,
		[self = shared_from_this()](boost::system::error_code ec, tcp::resolver::results_type results) {
			self->on_resolve(ec, results);
		});
}

void TcpClient::on_resolve(const boost::system::error_code& ec, const boost::asio::ip::tcp::resolver::results_type& endpoints) {
	if (!ec) {
		boost::asio::async_connect(
			socket_,
			endpoints,
			[self = shared_from_this()](boost::system::error_code ec, const boost::asio::ip::tcp::endpoint&) {
				self->on_connect(ec);
			});
	}
	else {
		std::cerr << "Ошибка разрешения адреса: " << ec.message() << "\n";
	}
}

//  пробная функция, для тестировки
void TcpClient::on_connect(const boost::system::error_code& ec) {
	if (!ec) {
		std::cout << "Подключено к серверу.\n";
		list_files(); // Для примера сразу запрашиваем список файлов
	}
	else {
		std::cerr << "Ошибка подключения: " << ec.message() << "\n";
	}
}

void TcpClient::send_command(const std::string& command) {
	auto self = shared_from_this();

	auto command_ptr = std::make_shared<std::string>(command + "\n");

	boost::asio::async_write(
		socket_,
		boost::asio::buffer(*command_ptr),
		[self, command_ptr](boost::system::error_code ec, std::size_t bytes_transferred) {
			if (!ec) {
				self->on_write(bytes_transferred);
			}
			else {
				std::cerr << "Ошибка при отправке команды: " << ec.message() << "\n";
			}
		});
}

void TcpClient::receive_response() {
	auto self = shared_from_this();
	boost::asio::async_read_until(
		socket_,
		boost::asio::dynamic_buffer(response_buffer_),
		"\n",
		[self](boost::system::error_code ec, std::size_t length) {
			if (!ec) {
				std::string response = self->response_buffer_.substr(0, length);
				self->response_buffer_.erase(0, length);

				switch (self->current_state) {
				case ClientState::ListingFiles:
					self->handle_list_response(response);
					break;

				case ClientState::ReceivingFile:
					self->handle_file_response(response, length);
					break;

				case ClientState::UploadingFile:
					if (response == "READY\n") {
						self->send_file_data();
					}
					break;

				case ClientState::Idle:
				default:
					std::cout << "Ответ сервера: " << response << "\n";
					break;
				}

				if (response == "END\n") {
					//  Временное решение
					std::cout << "Введите следующую команду\n";
					std::string command;
					std::getline(std::cin, command);
					size_t space_pos = command.find(' '); // Находим первый пробел
					std::string key_command = command.substr(0, space_pos); // Извлекаем первое слово
					std::string filename;
					if (key_command == "LIST")
					{
						self->list_files();
						return;
					}
					if (key_command == "GET")
					{
						filename = command.substr(space_pos + 1); // Извлекаем имя файла
						self->get_file(filename);
						return;
					}
					if (key_command == "UPLOAD")
					{
						filename = command.substr(space_pos + 1); // Извлекаем имя файла
						self->upload_file(filename);
						return;
					}
					else {
						self->send_command(command);
					}
					return;
				}
				if (self->current_state != ClientState::ReceivingFile) {
					self->receive_response();
				}
			}
			else {
				std::cerr << "Ошибка чтения: " << ec.message() << "\n";
			}
		});
}

void TcpClient::send_file_data()
{
	auto self = shared_from_this();
	const std::size_t chunk_size = 1024 * 1024; // Размер блока данных

	// Позиция в файле
	auto current_position = std::make_shared<std::size_t>(0);
	auto chunk_data = std::make_shared<std::string>();
	auto send_next_chunk = std::make_shared<std::function<void()>>();
	*send_next_chunk = [self, current_position, chunk_size, send_next_chunk, chunk_data]() {
		try {
			// Буфер для хранения данных текущего блока

			std::ostringstream buffer_stream;


			// Читаем следующую часть файла
			self->file_manager_->read_from_file(self->current_file, buffer_stream, *current_position, chunk_size);

			// Извлекаем данные в строку
			*chunk_data = buffer_stream.str();
			if (chunk_data->empty()) {
				// Все данные отправлены
				std::cout << "Файл полностью отправлен!" << std::endl;
				self->file_manager_->close_file();
				return;
			}

			*current_position += chunk_data->size(); // Обновляем текущую позицию

			// Отправляем данные через сокет
			boost::asio::async_write(
				self->socket_,
				boost::asio::buffer(*chunk_data),
				[self, send_next_chunk](const boost::system::error_code& error, std::size_t bytes_transferred) {
					if (error) {
						std::cerr << "Ошибка при отправке данных: " << error.message() << std::endl;
						return;
					}
					std::cout << "Отправлено: " << bytes_transferred << " байт." << std::endl;

					// Отправляем следующий блок
					(*send_next_chunk)();
				}
			);
		}
		catch (const std::exception& e) {
			self->file_manager_->close_file();
			std::cerr << "Ошибка при чтении или отправке данных: " << e.what() << std::endl;
		}
		};

	// Начинаем отправку
	(*send_next_chunk)();
}

void TcpClient::on_write(std::size_t bytes_transferred) {
	std::cout << "Отправлено " << bytes_transferred << " байт.\n";
	receive_response();
}

void TcpClient::handle_list_response(const std::string& response)
{
	if (response == "END\n") {
		std::cout << "Список файлов получен.\n";
		current_state = ClientState::Idle;
	}
	else if (response != "\n") {
		std::cout << "Файл: " << response;
	}
}

void TcpClient::handle_file_response(const std::string& response, std::size_t bytes_transfered) {
	if (response == "ERROR: file not found\n") {
		std::cout << "Файл " << current_file << " не был найден.\n";
		current_state = ClientState::Idle;
		is_new_file = true;
		file_manager_->close_file();
		bytes_received = 0;
		return;
	}
	if (response == "END\n") {
		std::cout << "Файл " << current_file << " получен.\n";
		current_state = ClientState::Idle;
		is_new_file = true;
		bytes_received = 0;
		file_manager_->close_file();
		receive_response();
		
	}
	else {
		if (is_new_file) {
			bytes_received = 0;
			// Пишем данные в файл через FileHandler
			current_file = file_manager_->create_file(current_file, false);
			size_t space_pos = response.find(' ');
			std::string receiving_filename = response.substr(0, space_pos);
			if (current_file == receiving_filename) {
				std::cout << "Файлы сходятся\n";
			}
			receiving_file_size = std::stoull(response.substr(space_pos + 1));
			is_new_file = false;
			receive_file_chunk();
		}
		else {
			// Накопление данных в буфере
			bytes_received += bytes_transfered;
			// Записываем в файл, если буфер достиг порогового значения

			if (bytes_received >= receiving_file_size) {
				file_manager_->add_to_file(current_file, response.substr(0, (bytes_transfered - (bytes_received - receiving_file_size))));
				handle_file_response("END\n", 10);
			}
			else {
				file_manager_->add_to_file(current_file, response);
				receive_file_chunk();
			}

		}
	}
}

void TcpClient::receive_file_chunk()
{
	auto buffer = std::make_shared<std::vector<char>>((1024 * 64));
	auto self = shared_from_this();

	self->socket_.async_read_some(boost::asio::buffer(*buffer),
		[self, buffer](const boost::system::error_code& error, std::size_t bytes_transferred) {
			if (!error) {
				std::string data_to_write(buffer->begin(), buffer->begin() + bytes_transferred);
				self->handle_file_response(data_to_write.c_str(), bytes_transferred);
				// Проверяем, достигли ли ожидаемого размера файла
			}
			else if (error == boost::asio::error::eof) {
				std::cout << "Передача файла завершена досрочно!" << std::endl;
			}
			else {
				std::cerr << "Ошибка приема файла: " << error.message() << std::endl;
			}
		});
}


void TcpClient::list_files() {
	current_state = ClientState::ListingFiles;
	send_command("LIST");
}

// Команда GET
void TcpClient::get_file(const std::string& filename) {
	current_state = ClientState::ReceivingFile;
	current_file = filename;
	send_command("GET " + filename);
}

void TcpClient::upload_file(const std::string& filename)
{
	if (file_manager_->file_exists(filename)) {
		current_state = ClientState::UploadingFile;
		current_file = filename;
		send_command("UPLOAD " + filename + " " + std::to_string(file_manager_->get_file_size(filename)));
	}
	else {

		send_command("Error filename");//  чтобы не закрывать подключение - временное решение
		//receive_response();
		std::cerr << "Файла " << filename << " не существует!\n";
	}
}
