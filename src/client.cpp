#include "client.h"

TcpClient::TcpClient(boost::asio::io_context& io_context, const std::string& base_dir)
	: resolver_(io_context), socket_(io_context),
    file_manager_(std::make_unique<FileManager>(base_dir))
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
                std::cout << "Ответ от сервера: " << self->response_buffer_.substr(0, length) << "\n";
                if (self->response_buffer_.substr(0, length) == "END\n")
                {
                    self->response_buffer_.erase(0, length);
                    std::cout << "Введите следующую команду\n";
                    std::string command;
                    std::getline(std::cin, command);
                    self->send_command(command);
                    return;
                }
                self->response_buffer_.erase(0, length);
                self->receive_response();
            }
            else {
                std::cerr << "Ошибка чтения: " << ec.message() << "\n";
            }
        });
}

void TcpClient::on_write(std::size_t bytes_transferred) {
    std::cout << "Отправлено " << bytes_transferred << " байт.\n";
    receive_response();
}

void TcpClient::list_files() {
    current_state = ClientState::ListingFiles;
    send_command("LIST");
}

// Команда GET
void TcpClient::get_file(const std::string& path) {
    current_state = ClientState::ReceivingFile;
    send_command("GET " + path);
}
