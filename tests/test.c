#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>

#include "../libfractal/fractal.c"

int main()
{
  if(CUE_SUCCESS != CU_initialize_registry())
    return CU_get_error();
  int setup(void) { return 0; }
  int teardown(void) { return 0; }
  CU_pSuite s_fractal= NULL;
  s_fractal = CU_add_suite("suite_fractal", setup, teardown);
  if(s_fractal == NULL)
    {
      CU_cleanup_registry();
      return CU_get_error();
    }

  if((NULL == CU_add_test(s_fractal, "Test_fractal_new", test1_fractal_new))
    )
    {
      CU_cleanup_registry();
      return CU_get_error();
    }

  CU_basic_run_tests();
  CU_basic_show_failures(CU_get_failure_list());
  CU_cleanup_registry();
  return CU_get_error();
}
