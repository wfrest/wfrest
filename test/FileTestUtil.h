#include <string>

class FileTestUtil
{
public:
    static bool write_file(const std::string &file_name, const std::string& body);

    static int create_dir(const char *path, mode_t mode);

    static int mkpath(const char *path, mode_t mode);

    static int recursive_delete(const char *dir);
};




