#ifndef c_GLOBAL_H_
#define WFREST_GLOBAL_H_

namespace wfrest
{
class HttpFile;

class Global
{
public:
    static HttpFile *get_http_file();
};

}  // namespace wfrest


#endif // WFREST_GLOBAL_H_
