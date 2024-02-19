
#include <mysql/jdbc.h>

#define EXAMPLE_DB "example_db"  


int main(int argc, char const *argv[])
{
    sql::mysql::MySQL_Driver *driver;
    sql::Connection *con;
    sql::Statement *stmt;

    driver = sql::mysql::get_mysql_driver_instance();
    con = driver->connect("tcp://127.0.0.1:3306", "user", "password");

    stmt = con->createStatement();
    stmt->execute("USE " EXAMPLE_DB);
    stmt->execute("DROP TABLE IF EXISTS test");
    stmt->execute("CREATE TABLE test(id INT, label CHAR(1))");
    stmt->execute("INSERT INTO test(id, label) VALUES (1, 'a')");

    delete stmt;
    delete con;
    return 0;
}
