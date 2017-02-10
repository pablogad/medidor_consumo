#ifndef __SQLITE_EXCP__
#define __SQLITE_EXCP__

#include <string>

using std::string;

class sqlite_excp
{
public:
   sqlite_excp( const string& );
   sqlite_excp( const int );
   sqlite_excp( void );
   ~sqlite_excp();
};

class sqlite_index_excp : public sqlite_excp
{
public:
   sqlite_index_excp( int );
};

class sqlite_busy_excp : public sqlite_excp
{
public:
   sqlite_busy_excp( void );
};

class sqlite_insert_excp : public sqlite_excp
{
public:
   sqlite_insert_excp( int );
};
#endif  // __SQLITE_EXCP__

