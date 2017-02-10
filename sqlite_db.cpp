#include <sqlite_db.h>
#include <iostream>

#include <string.h>

// TEMP
using std::string;
using std::cout;
using std::endl;

sqlite_db::sqlite_db( const string& db_file_name ) throw( sqlite_excp )
{
   int rc;

   db_status = CLOSED;

   // Open db file
   rc = sqlite3_open( db_file_name.c_str(), &db );

   if( rc != SQLITE_OK )
   {
       sqlite_excp ex( rc );
       throw ex;
   }

   db_status = OPEN;
}

sqlite_db::~sqlite_db()
{
   try {
      close_db();
   }
   catch(...) {};
}

int sqlite_db::callback( void *NotUsed, int argc, char **argv, char **azColName )
{
   int i;
   for (i = 0; i < argc; i++)
   {
       string chorr( azColName[i] );
       string chorr1( argv[i] ? argv[i] : "NULL" );
       cout << chorr << " = " << chorr1 << endl;
   }
   return 0;
}

void sqlite_db::query_db( const string& db_query ) throw( sqlite_excp )
{
   int rc;

   if( db_status == OPEN )
   {
      char *db_err_msg;
      rc = sqlite3_exec( db, db_query.c_str(), callback, this, &db_err_msg );

      if( rc != SQLITE_OK )
      {
         string err( db_err_msg );
         sqlite_excp ex( err );
         sqlite3_free( db_err_msg );
         throw ex;
      }
   }
}


// Especialización de binder:

static inline void binder_error_check( const int rc )
{
   if( rc == SQLITE_RANGE )
      throw sqlite_index_excp( rc );
   else if( rc != SQLITE_OK )
      throw sqlite_excp( rc );
}

template <> bool sqlite_db::binder<int>( int column, int col_bind, null_db* nuls, sqlite3_stmt *stmt )
{
   int rc;
   bool nul=false;

#ifdef SQLDEBUG
cout << "binder:INT:" << col_bind << endl;
#endif
   if( nuls )
   if( nuls->col_es_null( column ) )
   {
cout << "         :NULL!" << endl;
      rc = sqlite3_bind_null( stmt, column );
      nul=true;
   }

   if( nul == false )
      rc = sqlite3_bind_int( stmt, column, col_bind );

   // Tratamiento de errores:
   binder_error_check( rc );

   return nul;
}

template <> bool sqlite_db::binder<double>( int column, double col_bind, null_db* nuls, sqlite3_stmt *stmt )
{
   int rc;
   bool nul=false;

#ifdef SQLDEBUG
cout << "binder:DOUBLE:" << col_bind << endl;
#endif
   if( nuls )
   if( nuls->col_es_null( column ) )
   {
#ifdef SQLDEBUG
cout << "         :NULL!" << endl;
#endif
      rc = sqlite3_bind_null( stmt, column );
      nul = true;
   }

   if( nul == false )
      rc = sqlite3_bind_double( stmt, column, col_bind );

   // Tratamiento de errores:
   binder_error_check( rc );

   return nul;
}

template <> bool sqlite_db::binder<char*>( int column, char* col_bind, null_db* nuls, sqlite3_stmt *stmt )
{
   int rc;
   bool nul=false;

#ifdef SQLDEBUG
   cout << "binder:TEXT:" << col_bind << endl;
#endif
   if( nuls )
   if( nuls->col_es_null( column ) )
   {
#ifdef SQLDEBUG
cout << "         :NULL!" << endl;
#endif
      rc = sqlite3_bind_null( stmt, column );
      nul = true;
   }

   if( nul == false )
      rc = sqlite3_bind_text( stmt, column, col_bind, strlen( col_bind )+1, SQLITE_STATIC );

   // Tratamiento de errores:
   binder_error_check( rc );

   return nul;
}

template <> bool sqlite_db::binder<string>( int column, string col_bind, null_db* nuls, sqlite3_stmt *stmt )
{
   int rc;
   bool nul=false;

#ifdef SQLDEBUG
   cout << "binder:STRING:" << col_bind << endl;
#endif
   if( nuls )
   if( nuls->col_es_null( column ) )
   {
#ifdef SQLDEBUG
cout << "         :NULL!" << endl;
#endif
      rc = sqlite3_bind_null( stmt, column );
      nul = true;
   }

   if( nul == false )
      rc = sqlite3_bind_text( stmt, column, col_bind.c_str(), col_bind.length()+1, SQLITE_STATIC );

   // Tratamiento de errores:
   binder_error_check( rc );

   return nul;
}

template <> bool sqlite_db::binder<null_db>( int column, null_db col_bind, null_db* nuls, sqlite3_stmt *stmt )
{
   int rc;

#ifdef SQLDEBUG
   cout << "binder:NULL" << endl;
#endif
   rc = sqlite3_bind_null( stmt, column );

   // Tratamiento de errores:
   binder_error_check( rc );

   return false;
}

template <> bool sqlite_db::binder<void*>( int column, void* col_bind, null_db* nuls, sqlite3_stmt *stmt )
{
#ifdef SQLDEBUG
cout << "binder:IGNORE" << endl;
#endif
   return true;
}

// Especialización de extractor_col:
template <> bool sqlite_db::extractor_col<int>( int column, int* col_out, null_db* nuls, sqlite3_stmt *stmt )
{
   int ctype;
   // Si es NULL, activar indicador de ello para esta columna:
   if( (ctype = sqlite3_column_type( stmt, column ) ) == SQLITE_NULL )
   {
#ifdef SQLDEBUG
cout << "INT:es NULL" << endl;
#endif
      if( nuls ) nuls->poner_null( column+1 );
      *col_out = 0;
   }
   else
   {
// TODO: throw excep si tipo no coincide
      *col_out = sqlite3_column_int( stmt, column );
      nuls->quitar_null( column+1 );
      cout << "INT:" << *col_out << endl;
   }
   return true;
}

template <> bool sqlite_db::extractor_col<double>( int column, double* col_out, null_db* nuls, sqlite3_stmt *stmt )
{
   int ctype;
   // Si es NULL, activar indicador de ello para esta columna:
   if( (ctype = sqlite3_column_type( stmt, column ) ) == SQLITE_NULL )
   {
#ifdef SQLDEBUG
cout << "DOUBLE:es NULL" << endl;
#endif
      if( nuls ) nuls->poner_null( column+1 );
      *col_out = 0;
   }
   else
   {
// TODO: throw excep si tipo no coincide
      *col_out = sqlite3_column_double( stmt, column );
      nuls->quitar_null( column+1 );
      cout << "DOUBLE:" << *col_out << endl;
   }
   return true;
}

template <> bool sqlite_db::extractor_col<char*>( int column, char** col_out, null_db* nuls, sqlite3_stmt *stmt )
{
   int ctype;
   // Si es NULL, activar indicador de ello para esta columna:
   if( (ctype = sqlite3_column_type( stmt, column ) ) == SQLITE_NULL )
   {
#ifdef SQLDEBUG
cout << "CHAR*:es NULL" << endl;
#endif
      if( nuls ) nuls->poner_null( column+1 );
      *col_out = NULL;
   }
   else
   {
// TODO: throw excep si tipo no coincide
      const unsigned char* tmp = sqlite3_column_text( stmt, column );
      *col_out = new char[ strlen((const char*)tmp)+1 ];
      strcpy( (char*)*col_out, (char*)tmp );
      nuls->quitar_null( column+1 );
      cout << "TEXT:" << *col_out << endl;
   }
   return true;
}

template <> bool sqlite_db::extractor_col<string>( int column, string* col_out, null_db* nuls, sqlite3_stmt *stmt )
{
   int ctype;
   // Si es NULL, activar indicador de ello para esta columna:
   if( (ctype = sqlite3_column_type( stmt, column ) ) == SQLITE_NULL )
   {
#ifdef SQLDEBUG
cout << "STRING:es NULL" << endl;
#endif
      if( nuls ) nuls->poner_null( column+1 );
      col_out->clear();
   }
   else
   {
// TODO: throw excep si tipo no coincide
      const unsigned char* tmp = sqlite3_column_text( stmt, column );
      *col_out = string( (char*)tmp );
      nuls->quitar_null( column+1 );
      cout << "TEXT:" << *col_out << endl;
   }
   return true;
}

// Caso de última columna:
template <> bool sqlite_db::extractor_col<void*>( int column, void** col_out, null_db* nuls, sqlite3_stmt *stmt )
{
   return false;
}


void sqlite_db::close_db() throw( sqlite_excp, sqlite_busy_excp )
{
   int rc;

   if( db_status == OPEN )
   {
      rc = sqlite3_close( db );

      if( rc == SQLITE_BUSY )
      {
          sqlite_busy_excp exb;
          throw exb;
      }
      else if( rc != SQLITE_OK )
      {
          sqlite_excp ex( rc );
          throw ex;
      }
   }
}

