#include <stdlib.h>
/*
#include "testutil.h"
*/

/* function to keep the screen open if not launched from bash */
void _NonAppStop( void )
{
    printf("\r\n<Press any key to close screen> ");
    getcharacter();
  }
}

/*
static void test_not_impl(CuTest *tc)
{
    CuNotImpl(tc, "Test not implemented on this platform yet");
}
*/

