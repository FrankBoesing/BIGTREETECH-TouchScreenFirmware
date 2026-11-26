#ifndef GCODE_PARSER_H
#define GCODE_PARSER_H

#include <stdbool.h>
#include <stdint.h>

// Parse the leading numeric G-code identifier (M/G/T) without treating
// a missing number as zero. A truncated line such as "M" must never become M0.
static bool parseCommandCode(const char * cmd, const char expectedLetter, int32_t * value)
{
  if (cmd == NULL || value == NULL || cmd[0] != expectedLetter)
    return false;

  const char * p = cmd + 1;
  bool negative = false;

  // T-1 is used by some G-code senders to deselect the active tool.
  if (expectedLetter == 'T' && *p == '-')
  {
    negative = true;
    p++;
  }

  if (*p < '0' || *p > '9')
    return false;

  uint32_t parsed = 0;
  const uint32_t limit = negative ? 2147483648U : 2147483647U;

  while (*p >= '0' && *p <= '9')
  {
    uint32_t digit = (uint32_t)(*p - '0');

    // Keep the result representable as a signed 32-bit integer.
    if (parsed > (limit - digit) / 10U)
      return false;

    parsed = parsed * 10U + digit;
    p++;
  }

  *value = negative ? (parsed == 2147483648U ? (-2147483647 - 1) : -(int32_t)parsed)
                    : (int32_t)parsed;
  return true;
}

#endif  // GCODE_PARSER_H
