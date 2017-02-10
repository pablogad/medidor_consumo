#include <sqlite_excp.h>

#include <iostream>

using std::cout;
using std::endl;

sqlite_excp::sqlite_excp( const string& err_msg )
{
   cout << "ERROR en sqlite [" << err_msg << "]" << endl;
}

sqlite_excp::sqlite_excp( const int rc )
{
   cout << "ERROR en sqlite [" << rc << "]" << endl;
}

sqlite_excp::sqlite_excp( void ) {}

sqlite_excp::~sqlite_excp()
{
}


// sqlite_index_excp:
sqlite_index_excp::sqlite_index_excp( const int rc ) : sqlite_excp( rc )
{
   cout << "ERROR índice con valor " << rc << "fuera de rango" << endl;
}

// sqlite_busy_excp:
sqlite_busy_excp::sqlite_busy_excp( void ) : sqlite_excp()
{
}

// sqlite_insert_excp:
sqlite_insert_excp::sqlite_insert_excp( const int rc )
{
   cout << "ERROR en INSERT sqlite [" << rc << "]" << endl;
}

