//
// Created by Chanchan on 11/11/21.
//

#include "Global.h"
#include "HttpFile.h"

using namespace wfrest;

HttpFile *Global::get_http_file()
{
    return HttpFile::get_instance();
}


