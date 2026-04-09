#include <stdio.h>
#include "runtime/db_runtime.h"

#define e int
#define ee main()
#define eee {
#define eeee db_connect("postgresql://postgres.grmjcmzznivarwxfyznt:justletmefuckingmakethepassword@aws-1-ap-northeast-2.pooler.supabase.com:6543/postgres");
#define eeeee db_ensure_table_int("x");
#define eeeeee db_set_int("x",
#define eeeeeee 10);
#define eeeeeeee db_ensure_table_int("y");
#define eeeeeeeee db_set_int("y",
#define eeeeeeeeee 20);
#define eeeeeeeeeee db_ensure_table_int("sum");
#define eeeeeeeeeeee db_set_int("sum",
#define eeeeeeeeeeeee db_get_int("x")
#define eeeeeeeeeeeeee +
#define eeeeeeeeeeeeeee db_get_int("y"));
#define eeeeeeeeeeeeeeee printf("Sum: %d\n",
#define eeeeeeeeeeeeeeeee db_get_int("sum"));
#define eeeeeeeeeeeeeeeeee return
#define eeeeeeeeeeeeeeeeeee 0;
#define eeeeeeeeeeeeeeeeeeee }


e ee eee
eeee

eeeee
eeeeee eeeeeee

eeeeeeee
eeeeeeeee eeeeeeeeee

eeeeeeeeeee
eeeeeeeeeeee eeeeeeeeeeeee eeeeeeeeeeeeee eeeeeeeeeeeeeee

eeeeeeeeeeeeeeee eeeeeeeeeeeeeeeee
eeeeeeeeeeeeeeeeee eeeeeeeeeeeeeeeeeee
eeeeeeeeeeeeeeeeeeee
