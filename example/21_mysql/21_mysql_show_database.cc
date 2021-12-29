#include "wfrest/Mysql.h"

using namespace wfrest;
using namespace protocol;

void show_database_callback(WFMySQLTask *task)
{
    MySQLResponse *resp = task->get_resp();
    MySQLResultCursor cursor(resp);
    const MySQLField *const *fields;

    if (task->get_state() != WFT_STATE_SUCCESS)
    {
        fprintf(stderr, "error msg: %s\n",
                WFGlobal::get_error_string(task->get_state(),
                                            task->get_error()));
        return;
    }
    if (cursor.get_cursor_status() == MYSQL_STATUS_GET_RESULT)
    {
        fprintf(stderr, "cursor_status=%d field_count=%u rows_count=%u\n",
                cursor.get_cursor_status(), cursor.get_field_count(),
                cursor.get_rows_count());
    }
    fprintf(stderr, "databases list :\n");
    std::vector<MySQLCell> arr;
    while (cursor.fetch_row(arr))
    {
        fprintf(stderr, "[%s]\n", arr[0].as_string().c_str());
    }
}

int main()
{
    Mysql db("mysql://root:111111@localhost");

    db.execute("SHOW DATABASES", show_database_callback);

    getchar();
}