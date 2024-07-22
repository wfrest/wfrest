#include "MysqlUtil.h"

using namespace wfrest;
using namespace protocol;

std::vector<std::string> MySQLUtil::fields(const MySQLResultCursor &cursor)
{
    std::vector<std::string> fields_name;
    if (cursor.get_field_count() < 0) 
    {
        return fields_name;
    }
    fields_name.reserve(cursor.get_field_count());
    const MySQLField *const *fields = cursor.fetch_fields();
    for (int i = 0; i < cursor.get_field_count(); i++) {
        fields_name.emplace_back(fields[i]->get_name());
    }
    return fields_name;
}

std::vector<std::string> MySQLUtil::data_type(const protocol::MySQLResultCursor &cursor)
{
    std::vector<std::string> data_type;
    if (cursor.get_field_count() < 0) 
    {
        return data_type;
    }
    data_type.reserve(cursor.get_field_count());
    const MySQLField *const *fields = cursor.fetch_fields();
    for (int i = 0; i < cursor.get_field_count(); i++)
    {
        data_type.emplace_back(datatype2str(fields[i]->get_data_type()));
    }
    return data_type;
}

std::string MySQLUtil::to_string(const MySQLCell &cell)
{
    if(cell.is_null())
    {
        return "";
    }
    else if(cell.is_int())
    {
        return std::to_string(cell.as_int());
    } 
    else if(cell.is_string())
    {
        return cell.as_string();
    }
    else if(cell.is_float())
    {
        return std::to_string(cell.as_float());
    } 
    else if(cell.is_double())
    {
        return std::to_string(cell.as_double());
    } 
    else if(cell.is_ulonglong())
    {
        return std::to_string(cell.as_ulonglong());
    }
    else if(cell.is_date())
    {
        return cell.as_date();
    } 
    else if(cell.is_time())
    {
        return cell.as_time();
    } 
    else if(cell.is_datetime())
    {
        return cell.as_datetime();
    }
    return "";
}
