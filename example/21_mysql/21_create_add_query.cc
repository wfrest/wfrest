#include "wfrest/Mysql.h"
#include "wfrest/MysqlUtil.h"

using namespace wfrest;
using namespace protocol;

int main()
{
    MySQL db("mysql://root:111111@localhost/wfrest_test");
    
    db.query("DROP TABLE IF EXISTS wfrest", nullptr);
    db.query("CREATE TABLE wfrest(id INT PRIMARY KEY AUTO_INCREMENT, name VARCHAR(255), price INT)", nullptr);
    db.query("INSERT INTO wfrest VALUES(1,'workflow',52642)", nullptr);
    db.query("INSERT INTO wfrest VALUES(2,'srpc',57127)", nullptr);
    db.query("INSERT INTO wfrest VALUES(3,'wfrest',9000)", nullptr);
    
    db.query("SELECT * FROM wfrest", [](MySQLResultCursor &cursor, 
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
    
    db.start();
    getchar();
}