#include <stdio.h>
#include "runtime/db_runtime.h"

int main() {
    db_connect("postgresql://postgres.grmjcmzznivarwxfyznt:justletmefuckingmakethepassword@aws-1-ap-northeast-2.pooler.supabase.com:6543/postgres");

    db_ensure_table_int("x");
    db_set_int("x", 10);

    db_ensure_table_int("y");
    db_set_int("y", 20);

    db_ensure_table_int("sum");
    db_set_int("sum", db_get_int("x") + db_get_int("y"));

    printf("Sum: %d\n", db_get_int("sum"));
    return 0;
}