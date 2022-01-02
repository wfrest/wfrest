#include "wfrest/Mysql.h"
#include "wfrest/MysqlUtil.h"

using namespace wfrest;
using namespace protocol;

int main()
{
    MySQL db("mysql://root:111111@localhost");

    db.execute("DROP DATABASE IF EXISTS wfrest_test; CREATE DATABASE IF NOT EXISTS wfrest_test;", nullptr);

    db.execute("SHOW DATABASES", [](MySQLResultCursor &cursor, 
                                    const MySQL::Status &status)
    {
        fprintf(stderr, "Show databases\n");

        std::vector<MySQLCell> arr;
        while (cursor.fetch_row(arr))
        {
            fprintf(stderr, "[%s]\n", arr[0].as_string().c_str());
        }
    });

    MySQL db1("mysql://root:111111@localhost/wfrest_test");
    
    db1.execute("DROP TABLE IF EXISTS wfrest", nullptr);
    db1.execute("CREATE TABLE wfrest(id INT PRIMARY KEY AUTO_INCREMENT, name VARCHAR(255), price INT)", nullptr);
    db1.execute("INSERT INTO wfrest VALUES(1,'workflow',52642)", nullptr);
    db1.execute("INSERT INTO wfrest VALUES(2,'srpc',57127)", nullptr);
    db1.execute("INSERT INTO wfrest VALUES(3,'wfrest',9000)", nullptr);
    
    db1.execute("SELECT * FROM wfrest", [](MySQLResultCursor &cursor, 
                                        const MySQL::Status &status)
    {
        std::vector<MySQLCell> arr;
        while (cursor.fetch_row(arr))
        {
            for(int i = 0; i < arr.size(); i++)
            {
                fprintf(stderr, "|%s", MySQLUtil::to_string(arr[i]).c_str());
            }
            fprintf(stderr, "|\n");
        }
    });

    getchar();
}