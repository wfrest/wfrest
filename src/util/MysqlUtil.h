#ifndef WFREST_MYSQLUTIL_H_
#define WFREST_MYSQLUTIL_H_

#include <string>
#include <vector>
#include "workflow/MySQLResult.h"

namespace wfrest
{

class MySQLUtil
{
public:
    static std::vector<std::string> fields(const protocol::MySQLResultCursor &cursor);
    
    static std::vector<std::string> data_type(const protocol::MySQLResultCursor &cursor);

    static std::string to_string(const protocol::MySQLCell &cell);
};

} // namespace wfrest

#endif // WFREST_MYSQLUTIL_H_