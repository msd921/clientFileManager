#pragma once
#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include "file_manager.h"

using boost::asio::ip::tcp;

//  перечисление для текущего состояния клиента
enum class ClientState {
    Idle,
    ListingFiles,
    ReceivingFile,
    SendingFile
};

class TcpClient : public std::enable_shared_from_this<TcpClient> {
public:
	TcpClient(boost::asio::io_context& io_context, const std::string& base_dir);
    void start(const std::string& host, const std::string& port);
    void send_command(const std::string& command);
    void list_files(); //  получить список файлов с сервера
    void get_file(const std::string& path); //  получить конкретный файл
    //void upload_file(const std::string& path);

private:
    void on_resolve(const boost::system::error_code& ec, const tcp::resolver::results_type& endpoints); //  подключение
    
    void receive_response();  //  получаем ответ
    //void send_file_data(const std::vector<char>& file_data);
    void on_connect(const boost::system::error_code& ec);
    void on_write(std::size_t bytes_transferred);

    tcp::resolver resolver_;
    tcp::socket socket_;
    std::string response_buffer_;
    std::unique_ptr<FileManager> file_manager_;

    ClientState current_state = ClientState::Idle;
};