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
    UploadingFile
};

class TcpClient : public std::enable_shared_from_this<TcpClient> {
public:
	TcpClient(boost::asio::io_context& io_context, const std::string& base_dir);
    void start(const std::string& host, const std::string& port);
    
    void list_files(); //  получить список файлов с сервера
    void get_file(const std::string& filename); //  получить конкретный файл
    void upload_file(const std::string& filename);

private:
    void send_command(const std::string& command);
    void on_resolve(const boost::system::error_code& ec, const tcp::resolver::results_type& endpoints); //  подключение
    
    void receive_response();  //  получаем ответ
    void send_file_data();
    void on_connect(const boost::system::error_code& ec);
    void on_write(std::size_t bytes_transferred);

    void handle_list_response(const std::string& response);
    void handle_file_response(const std::string& response, std::size_t bytes_transfered);
    void receive_file_chunk();

    tcp::resolver resolver_;
    tcp::socket socket_;
    std::string response_buffer_;
    std::unique_ptr<FileHandler> file_manager_;
    std::string current_file; //  текущий файл для чтения/записи
    std::size_t receiving_file_size;
    std::size_t bytes_received = 0;

    ClientState current_state = ClientState::Idle;
    bool is_new_file = true; //  флаг создания нового файла или записываем в уже созданный
};