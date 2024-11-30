#pragma once
#include <string>

using namespace std;

class FileManager
{
public:
	FileManager(const string& base_dir);

	void write_to_file(const string& filename, const string& data, bool overwrite);
	void read_from_file(const std::string& filename, std::ostream& output_stream);
	void delete_file(const std::string& filename);
private:
	string get_unique_filename(const string& filename); //  если файл уже существует и мы его не перезаписываем

	bool file_exists(const std::string& filename);

	string base_directory_;


};
