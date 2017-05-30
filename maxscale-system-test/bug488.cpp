/**
 * @file bug488.cpp regression case for bug 488 ( SHOW VARIABLES randomly failing with "Lost connection to MySQL server")
 *
 * - try "SHOW VARIABLES;" 100 times against all Maxscale services
 * First round: 100 iterations for RWSplit, then ReadConn Master, then ReadConn Slave
 * Second round: 100 iteration and in every iterations all three Maxscale services are in use.
 * - check if Maxscale is alive.
 */

#include <my_config.h>
#include <iostream>
#include "testconnections.h"

int main(int argc, char *argv[])
{
    TestConnections * Test = new TestConnections(argc, argv);
    Test->set_timeout(10);
    int i;

    Test->repl->connect();
    Test->connect_maxscale();

    Test->tprintf("Trying SHOW VARIABLES to different Maxscale services\n");  fflush(stdout);
    Test->tprintf("RWSplit\n");
    for (i = 0; i < 100; i++) {Test->set_timeout(5);Test->try_query(Test->conn_rwsplit, (char *) "SHOW VARIABLES;");}
    Test->tprintf("ReadConn master\n");
    for (i = 0; i < 100; i++) {Test->set_timeout(5);Test->try_query(Test->conn_master, (char *) "SHOW VARIABLES;");}
    Test->tprintf("ReadConn slave\n");
    for (i = 0; i < 100; i++) {Test->set_timeout(5);Test->try_query(Test->conn_slave, (char *) "SHOW VARIABLES;");}

    Test->tprintf("All in one loop\n");
    for (i = 0; i < 100; i++) {
        Test->set_timeout(5);
        Test->try_query(Test->conn_rwsplit, (char *) "SHOW VARIABLES;");
        Test->try_query(Test->conn_master, (char *) "SHOW VARIABLES;");
        Test->try_query(Test->conn_slave, (char *) "SHOW VARIABLES;");
    }

    Test->set_timeout(10);
    Test->close_maxscale_connections();
    Test->repl->close_connections();

    Test->check_maxscale_alive();

    Test->copy_all_logs(); return(Test->global_result);
}