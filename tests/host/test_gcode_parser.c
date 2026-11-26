#include "GCodeParser.h"

#include <stdio.h>

static unsigned int testsRun = 0;
static unsigned int testsFailed = 0;

static void check(const char * text, char letter, bool expectedValid, int32_t expectedValue)
{
  int32_t value = 12345;
  bool valid = parseCommandCode(text, letter, &value);
  testsRun++;

  if (valid != expectedValid || (valid && value != expectedValue))
  {
    fprintf(stderr, "FAIL: %s (letter=%c, valid=%d, value=%ld)\n",
            text == NULL ? "<null>" : text, letter, valid, (long)value);
    testsFailed++;
  }
}

int main(void)
{
  check(NULL, 'M', false, 0);
  check("M", 'M', false, 0);
  check("M-1", 'M', false, 0);
  check("M0", 'M', true, 0);
  check("M1\n", 'M', true, 1);
  check("M112*42", 'M', true, 112);
  check("M2147483647", 'M', true, 2147483647);
  check("M2147483648", 'M', false, 0);
  check("G28", 'G', true, 28);
  check("G1X10Y20", 'G', true, 1);
  check("T0", 'T', true, 0);
  check("T-1", 'T', true, -1);
  check("T-2147483648", 'T', true, (-2147483647 - 1));
  check("T-2147483649", 'T', false, 0);
  check("X1", 'M', false, 0);

  if (testsFailed != 0)
  {
    fprintf(stderr, "%u of %u tests failed\n", testsFailed, testsRun);
    return 1;
  }

  printf("PASS: %u G-code parser tests\n", testsRun);
  return 0;
}
