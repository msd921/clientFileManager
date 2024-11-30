#include "file_manager.h"
#include <filesystem>
#include <iostream>
#include <sstream>
#include <fstream>

namespace fs = std::filesystem;

FileManager::FileManager(const string& base_dir)
	: base_directory_(base_dir)
{
	if (!fs::exists(base_directory_)) {
		fs::create_directory(base_directory_);
	}
}

void FileManager::write_to_file(const string& filename, const string& data, bool overwrite)
{
	std::string target_filename = base_directory_ + "/" + filename;

	if (!overwrite && file_exists(filename)) {
		target_filename = get_unique_filename(filename);
		std::cout << "Файл уже существует. Сохраняю как: " << target_filename << "\n";
	}

	std::ofstream new_file(target_filename, std::ios::binary);
	if (!new_file) {
		throw std::runtime_error("Не удалось открыть файл для записи: " + target_filename);
	}
	new_file.write(data.data(), data.size());
	new_file.close();
}

void FileManager::read_from_file(const std::string& filename, std::ostream& output_stream)
{
	std::ifstream file(base_directory_ + "/" + filename, std::ios::binary);
	if (!file) {
		throw std::runtime_error("Не удалось открыть файл для чтения: " + filename);
	}

	const std::size_t buffer_size = 4096; // Размер буфера
	std::vector<char> buffer(buffer_size);
	
	while (file.read(buffer.data(), buffer_size) || file.gcount() > 0) {
		output_stream.write(buffer.data(), file.gcount());
	}
}

void FileManager::delete_file(const std::string& filename)
{
	if (fs::remove(base_directory_ + "/" + filename) == 0)
	{
		std::cout << "Файл " << filename << " успешно удален" << std::endl;
	}
	else {
		std::cout << "При удалении файла " << filename << " произошла ошибка" << std::endl;
	}
}

string FileManager::get_unique_filename(const string& filename)
{
	std::string new_filename = filename;
	int count = 1;

	while (file_exists(new_filename)) {
		std::ostringstream oss;
		oss << filename << "_copy" << count++ << fs::path(filename).extension().string();
		new_filename = oss.str();
	}

	return new_filename;
}

bool FileManager::file_exists(const std::string& filename)
{
	return fs::exists(base_directory_ + "/" + filename);
}
