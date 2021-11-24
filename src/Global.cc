#include "Global.h"
#include "HttpFile.h"

using namespace wfrest;

HttpFile *Global::get_http_file()
{
    return HttpFile::get_instance();
}


