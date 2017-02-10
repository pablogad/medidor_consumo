#ifndef __SQLITE_H__
#define __SQLITE_H__

#include <sqlite3.h>
#include <sqlite_excp.h>

#include <vector>
#include <iostream> //TEMP

using std::vector;

enum { CLOSED=0, OPEN };

// Estructura para almacenar campos que deben ir a null / se han leído a null:
class null_db {
public:
   null_db() : f(0) {};
   null_db( int flds ) : f( flds ) {};

   bool col_es_null( int column )
   {
      if( f & 1<<column ) return true;
      return false;
   }

   void poner_null( int column )
   {
      f |= 1<<column;
#ifdef SQLDEBUG
std::cout << "null_db::" << column << " es NULL. f=" << f << std::endl;
#endif
   }

   void quitar_null( int column )
   {
      f &= ~(1<<column);
#ifdef SQLDEBUG
std::cout << "null_db::" << column << " no es NULL. f=" << f << std::endl;
#endif
   }

   // Truko pa imprimir con cout:
   inline friend std::ostream& operator<<(std::ostream& out, const null_db& s)
   {
      out << s.f;
   }

private:
   int f;
};

// Objeto de base de datos con tipos genéricos: para leer/insertar/update:
template<typename A,typename B=void*,typename C=void*,typename D=void*,typename E=void*,typename F=void*> \
struct obj_db {
   A v1;
   B v2;
   C v3;
   D v4;
   E v5;
   F v6;
};

using std::string;

class sqlite_db
{
public:
   sqlite_db( const string& db_file_name ) throw (sqlite_excp);
   ~sqlite_db();

   void query_db( const string& db_query ) throw (sqlite_excp);
   void close_db( void ) throw( sqlite_excp, sqlite_busy_excp );

   // columnas: nombres de las columnas a insertar.
   // t: estructura con los datos a insertar.
   // nulls: indicador de campos que deben ser null en la inserción. Tambien se
   //        puede poner el campo a nulear en T con el objeto null_db. Si es
   //        NULL no se hace ni puto caso.
   template <typename T> void insert_obj( const string& tabla, \
                                          vector<string>& columnas, \
                                          T& t, \
                                          null_db* nulls, \
                                          const string* ins_opt=NULL )  // options: REPLACE/IGNORE/ABORT/FAIL... etc
   {
      sqlite3_stmt *stmt;
      string cmd( "INSERT" );

// TODO:verificar opciones válidas
      if( ins_opt != NULL )
         cmd += " OR " + *ins_opt;

      cmd += " INTO " + tabla + " (";
      string valstr( ") VALUES (" );
      vector<string>::iterator it;
      int rc;

      for( it = columnas.begin(); it < columnas.end(); it++ )
      {
         cmd.append( ( it != columnas.begin() ? "," : "" ) + *it );
         valstr.append( ( it != columnas.begin() ? "," : "" ) + string("?") );
      }

      cmd.append( valstr.append( ")" ) );
#ifdef SQLDEBUG
std::cout << "CMD TRAS " << columnas.size() << " COLUMNAS:" << cmd << std::endl;
#endif

        // Pillar tipos y valores de los campos definidos y hacer binds:
      rc = sqlite3_prepare_v2( db, cmd.c_str(), cmd.length()+1, &stmt, NULL );

      binder( 1, t.v1, nulls, stmt );
      binder( 2, t.v2, nulls, stmt );
      binder( 3, t.v3, nulls, stmt );
      binder( 4, t.v4, nulls, stmt );
      binder( 5, t.v5, nulls, stmt );
      binder( 6, t.v6, nulls, stmt );

      // Ejecutar el INSERT:
      rc = sqlite3_step( stmt );
#ifdef SQLDEBUG
std::cout << "sqlite3_step:rc=" << rc << std::endl;
#endif

      // Borra los parámetros anteriores:
      rc = sqlite3_reset( stmt );
#ifdef SQLDEBUG
std::cout << "sqlite3_reset:rc=" << rc << std::endl;
#endif

      // Borrar la sentencia ejecutada:
      sqlite3_finalize( stmt );

      if( rc != SQLITE_OK && rc != SQLITE_DONE )
      { if( rc == SQLITE_BUSY )
           throw sqlite_busy_excp();
        else
        {  sqlite_insert_excp ex( rc );
           throw ex;
        }
      }
   };

   // columnas: nombres de las columnas a insertar.
   // t_set: estructura con los datos a insertar.
   // t_where: estructura con los datos para selección de objetos a actualizar.
   //    Se puede poner el campo a nulear en T con el objeto null_db.
   // columnas: columnas a insertar seguido de columnas para el where. Debe coincidir el orden
   //           con lo indicado en t_set / t_where.
   template <typename T1, typename T2> void update_obj( const string& tabla, \
                                          T1& t_set, \
                                          T2& t_where,
                                          vector<string>& columnas )
   {
      sqlite3_stmt *stmt;
      null_db* nulls = NULL;
      string cmd( "UPDATE " + tabla + " SET " );

      vector<string>::iterator it;
      int cnt;
      int columnas_set, columnas_where;
      int rc;

      columnas_set = get_obj_db_size( t_set );
      columnas_where = get_obj_db_size( t_where );

      for( it = columnas.begin(), cnt=0; it < columnas.end(); it++ )
      {
         if( cnt != 0 ) if( cnt < columnas_set )      cmd.append( "," );
                        else if( cnt > columnas_set ) cmd.append( " AND " );

         if( cnt == columnas_set )
         {
            cmd.append( " WHERE " );
         }

         cmd.append( *it + " = ?" );

         cnt++;
      }
#ifdef SQLDEBUG
std::cout << "CMD :" << cmd << std::endl;
#endif

        // Pillar tipos y valores de los campos definidos y hacer binds:
      rc = sqlite3_prepare_v2( db, cmd.c_str(), cmd.length()+1, &stmt, NULL );

      cnt = 1;
      if( cnt <= columnas_set )  binder( cnt++, t_set.v1, nulls, stmt );
      if( cnt <= columnas_set )  binder( cnt++, t_set.v2, nulls, stmt );
      if( cnt <= columnas_set )  binder( cnt++, t_set.v3, nulls, stmt );
      if( cnt <= columnas_set )  binder( cnt++, t_set.v4, nulls, stmt );
      if( cnt <= columnas_set )  binder( cnt++, t_set.v5, nulls, stmt );
      if( cnt <= columnas_set )  binder( cnt++, t_set.v6, nulls, stmt );

      if( cnt <= columnas_set+columnas_where ) 
          binder( cnt++, t_where.v1, nulls, stmt );
      if( cnt <= columnas_set+columnas_where ) 
          binder( cnt++, t_where.v2, nulls, stmt );
      if( cnt <= columnas_set+columnas_where ) 
          binder( cnt++, t_where.v3, nulls, stmt );
      if( cnt <= columnas_set+columnas_where ) 
          binder( cnt++, t_where.v4, nulls, stmt );
      if( cnt <= columnas_set+columnas_where ) 
          binder( cnt++, t_where.v5, nulls, stmt );
      if( cnt <= columnas_set+columnas_where ) 
          binder( cnt++, t_where.v6, nulls, stmt );

      // Ejecutar el UPDATE:
      rc = sqlite3_step( stmt );
#ifdef SQLDEBUG
std::cout << "sqlite3_step:rc=" << rc << std::endl;
#endif

      // Borra los parámetros anteriores:
      rc = sqlite3_reset( stmt );
#ifdef SQLDEBUG
std::cout << "sqlite3_reset:rc=" << rc << std::endl;
#endif

      // Borrar la sentencia ejecutada:
      sqlite3_finalize( stmt );

      if( rc != SQLITE_OK && rc != SQLITE_DONE )
      { if( rc == SQLITE_BUSY )
           throw sqlite_busy_excp();
        else
        {  sqlite_insert_excp ex( rc );
           throw ex;
        }
      }
   };

   template <typename T> void insert_or_replace_obj( const string& tabla,vector<string>& columnas,T& t,null_db* nulls )
   {
       const string str("REPLACE");
       insert_obj<T>(tabla,columnas,t,nulls,&str);
   };

   template <typename T> void insert_or_ignore_obj( const string& tabla,vector<string>& columnas,T& t,null_db* nulls )
   {
       const string str("IGNORE");
       insert_obj<T>(tabla,columnas,t,nulls,&str);
   };

   // Para saber tipo de datos de un dato:
   enum { TIPO_DESCONOCIDO=0, TIPO_INT, TIPO_REAL, TIPO_CHAR, \
          TIPO_STRING, CAMPO_NULL, FIN_DE_DATOS };

   static int tipo_de(int&)    {return TIPO_INT; };
   static int tipo_de(double&) {return TIPO_REAL; };
   static int tipo_de(char&)   {return TIPO_CHAR; };
   static int tipo_de(char*)   {return TIPO_CHAR; };
   static int tipo_de(string&) {return TIPO_STRING; };
   static int tipo_de(null_db&) {return CAMPO_NULL; };
   static int tipo_de(void*)   {return FIN_DE_DATOS; };

   // Plantillas genéricas para especializar - ver cpp:
   template <typename T> bool extractor_col( int, T*, null_db*, sqlite3_stmt * ) {}
   template <typename T> bool binder( int, T, null_db*, sqlite3_stmt * ) {}

   template <typename T> int get_obj_db_size( T& t )
   {
      int cnt=0;
      if( tipo_de( t.v1 ) == FIN_DE_DATOS ) return cnt;
      ++cnt;
      if( tipo_de( t.v2 ) == FIN_DE_DATOS ) return cnt;
      ++cnt;
      if( tipo_de( t.v3 ) == FIN_DE_DATOS ) return cnt;
      ++cnt;
      if( tipo_de( t.v4 ) == FIN_DE_DATOS ) return cnt;
      ++cnt;
      if( tipo_de( t.v5 ) == FIN_DE_DATOS ) return cnt;
      ++cnt;
      if( tipo_de( t.v6 ) == FIN_DE_DATOS ) return cnt;
   }


   // columnas: nombres de las columnas a seleccionar. Se puede poner un elemento * pa pillar tó:
   // T: tipos de columnas de salida: debe coincidir con columnas.
   // where: condición para el where:
   // out: salida de la consulta, vector de elementos T:
   template <typename T> void select_obj( const string& tabla, \
                                          vector<string>& columnas, \
                                          const string& where, \
                                          vector<T>* out, \
                                          vector<null_db>* nulos, \
                                          T& t )
   {
      sqlite3_stmt *stmt;
      string cmd( "SELECT " );
      string valstr( " FROM " + tabla );
      vector<string>::iterator it;
      int rc;

      for( it = columnas.begin(); it < columnas.end(); it++ )
      {
         cmd.append( ( it != columnas.begin() ? "," : "" ) + *it );
      }

      cmd.append( valstr );
      if( where != "" )
      {
         cmd.append( " WHERE " + where );
      }

#ifdef SQLDEBUG
std::cout << "select_obj::" + cmd << std::endl;
#endif
        // Pillar tipos y valores de los campos definidos y hacer binds:
      rc = sqlite3_prepare_v2( db, cmd.c_str(), cmd.length()+1, &stmt, NULL );

      // Ejecutar el SELECT e ir sacando resulteision's:
      T unElem;
      out->clear();
      nulos->clear();

      while( (rc = sqlite3_step( stmt ) ) == SQLITE_ROW )
      {
          null_db nul;

          // Extraer columnas:
          if( extractor_col( 0, &unElem.v1, &nul, stmt ) )
          if( extractor_col( 1, &unElem.v2, &nul, stmt ) )
          if( extractor_col( 2, &unElem.v3, &nul, stmt ) )
          if( extractor_col( 3, &unElem.v4, &nul, stmt ) )
          if( extractor_col( 4, &unElem.v5, &nul, stmt ) )
          if( extractor_col( 5, &unElem.v6, &nul, stmt ) );

          out->push_back( unElem );
          nulos->push_back( nul );
      }

      // Borra los parámetros anteriores:
      sqlite3_reset( stmt );

      // Borrar la sentencia ejecutada:
      sqlite3_finalize( stmt );
   };


   // columnas: nombres de las columnas a seleccionar. Se puede poner un elemento * pa pillar tó:
   // T: tipos de columnas de salida: debe coincidir con columnas. Máx 6 columnas.
   // where: condición para el where:
   // out: salida de la consulta, vector de elementos T:
   template <typename T> void select_qry_obj( const string& query, \
                                              vector<string>& columnas, \
                                              const string& where, \
                                              vector<T>* out, \
                                              vector<null_db>* nulos, \
                                              T& t )
   {
      sqlite3_stmt *stmt;
      int rc;

#ifdef SQLDEBUG
std::cout << "select_qry_obj::" + query << std::endl;
#endif

        // Pillar tipos y valores de los campos definidos y hacer binds:
      rc = sqlite3_prepare_v2( db, query.c_str(), query.length()+1, &stmt, NULL );

      // Ejecutar el SELECT:
      T unElem;
      out->clear();
      nulos->clear();

      while( (rc = sqlite3_step( stmt ) ) == SQLITE_ROW )
      {
          null_db nul;

          // Extraer columnas:
          if( extractor_col( 0, &unElem.v1, &nul, stmt ) )
          if( extractor_col( 1, &unElem.v2, &nul, stmt ) )
          if( extractor_col( 2, &unElem.v3, &nul, stmt ) )
          if( extractor_col( 3, &unElem.v4, &nul, stmt ) )
          if( extractor_col( 4, &unElem.v5, &nul, stmt ) )
          if( extractor_col( 5, &unElem.v6, &nul, stmt ) );

          out->push_back( unElem );
          nulos->push_back( nul );
      }

      // Borra los parámetros anteriores:
      sqlite3_reset( stmt );

      // Borrar la sentencia ejecutada:
      sqlite3_finalize( stmt );
   };

private:
   static int callback( void *, int, char **, char ** );

   sqlite3 *db;
   int db_status;
};

#endif // __SQLITE_H__

