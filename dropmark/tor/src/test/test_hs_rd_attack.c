
#define CONTROL_PRIVATE
#include "or.h"
#include "control.h"
#include "hs_rd_attack.h"
#include "test.h"



static void 
test_hs_attack_entry_point(void *args)
{
  hs_attack_cmd_t cmd = ESTABLISH_RDV;
  const char *onionaddress = "x2glf6t2j2bimw2k.onion";
  hs_attack_stats_t *stats;

  stats = hs_attack_entry_point(cmd, onionaddress, 1, NULL);
  tt_assert(!stats)

  done:
    return;
}

struct testcase_t hs_attack_tests[] = {
  { "hs_attack_entry_point", test_hs_attack_entry_point, NULL},
  END_OF_TESTCASES
};
