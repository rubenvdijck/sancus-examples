#include "foo.h"
#include "bar.h"
#include <sancus_support/sm_io.h>

DECLARE_SM(foo, 0x1234);

int SM_FUNC(foo) foo_div( int i, unsigned int j)
{
    return (i / j);
}

int SM_ENTRY(foo) enter_foo( int i )
{
    ASSERT(sancus_get_caller_id() == SM_ID_UNPROTECTED);
    ASSERT(sancus_get_self_id() == 1);
    ASSERT(sancus_get_id((void*) bar_lookup) == 2);

    int j, k = bar_lookup(i);
    //TODO caller_id should remain 0 after merging Aion compiler changes
    ASSERT(sancus_get_caller_id() == 2);
    ASSERT(sancus_get_self_id() == 1);

    pr_info1("bar returned %d\n", k);

    j = foo_div(k, 5) % i;
    return i * j;
}
