#include "wfrest/Mysql.h"
#include "wfrest/MysqlUtil.h"

using namespace wfrest;
using namespace protocol;

int main()
{
    MySQL db("mysql://root:111111@localhost");

    db.query("DROP DATABASE IF EXISTS wfrest_test; CREATE DATABASE IF NOT EXISTS wfrest_test;", nullptr);

    db.query("SHOW DATABASES", [](MySQLResultCursor &cursor, 
                                    const MySQL::Status &status)
    {
        fprintf(stderr, "Show databases\n");

        std::vector<MySQLCell> arr;
        while (cursor.fetch_row(arr))
        {
            fprintf(stderr, "[%s]\n", arr[0].as_string().c_str());
        }
    });
    db.start();
    
    getchar();
}