#ifndef _GLOBAL_H_
#define _GLOBAL_H_

namespace wfrest
{
class HttpFile;

class Global
{
public:
    static HttpFile *get_http_file();
};

}  // namespace wfrest


#endif //_GLOBAL_H_
