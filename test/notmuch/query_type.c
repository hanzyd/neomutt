/**
 * @file
 * Test code for nm_string_to_query_type()
 *
 * @authors
 * Copyright (C) 2021 Austin Ray <austin@austinray.io>
 *
 * @copyright
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define TEST_NO_MAIN
#include <stdarg.h>
#include "config.h"
#include "acutest.h"
#include "mutt/lib.h"
#include "notmuch/query.h"

struct NmParseTypeTest
{
  char *input;
  enum NmQueryType expected;
};

void test_nm_parse_type_from_query(void)
{
  struct NmParseTypeTest tests[] = {
    { "&type=threads", NM_QUERY_TYPE_THREADS },
    { "&type=messages", NM_QUERY_TYPE_MESGS },
    { "type=threads&", NM_QUERY_TYPE_THREADS },
    { "type=messages&", NM_QUERY_TYPE_MESGS },
    { "type=threads", NM_QUERY_TYPE_THREADS },
    { "type=messages", NM_QUERY_TYPE_MESGS },
    { "", NM_QUERY_TYPE_MESGS },
    { NULL, NM_QUERY_TYPE_MESGS },
    { "type=non-existent", NM_QUERY_TYPE_MESGS },
    { "type=threads&type=non-existent", NM_QUERY_TYPE_THREADS },
    { "type=messages&type=non-existent", NM_QUERY_TYPE_MESGS },
  };

  char buf[1024];
  for (int i = 0; i < mutt_array_size(tests); i++)
  {
    struct NmParseTypeTest *t = &tests[i];
    memset(buf, 0, sizeof(buf));
    mutt_str_copy(buf, t->input, sizeof(buf));
    TEST_CASE(buf);
    TEST_CHECK(nm_parse_type_from_query(buf) == t->expected);
  }
}

static int test_log_capture(time_t stamp, const char *file, int line, const char
    *function, enum LogLevel level, ...)
{

  {
    char buf[1024];

    va_list ap;
    va_start(ap, level);
    const char *fmt = va_arg(ap, const char *);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    TEST_CHECK(level == LL_ERROR);
    TEST_CHECK(mutt_str_equal(buf, "failed to parse notmuch query type: junk"));
  }

  return 0;
}

void test_nm_string_to_query_type(void)
{
  {
    TEST_CHECK(nm_string_to_query_type("threads") == NM_QUERY_TYPE_THREADS);
  }

  {
    TEST_CHECK(nm_string_to_query_type("messages") == NM_QUERY_TYPE_MESGS);
  }

  // Test that we're handling the error condition.
  {
    MuttLogger = test_log_capture;
    TEST_CHECK(nm_string_to_query_type("junk") == NM_QUERY_TYPE_MESGS);
  }
}

void test_nm_string_to_query_type_mapper(void)
{
  {
    TEST_CHECK(nm_string_to_query_type_mapper("threads") == NM_QUERY_TYPE_THREADS);
  }

  {
    TEST_CHECK(nm_string_to_query_type_mapper("messages") == NM_QUERY_TYPE_MESGS);
  }

  {
    TEST_CHECK(nm_string_to_query_type_mapper("junk") == NM_QUERY_TYPE_UNKNOWN);
  }
}

void test_nm_query_type_to_string(void)
{
  {
    TEST_CHECK(mutt_str_equal(nm_query_type_to_string(NM_QUERY_TYPE_THREADS), "threads"));
  }

  {
    TEST_CHECK(mutt_str_equal(nm_query_type_to_string(NM_QUERY_TYPE_MESGS),
                              "messages"));
  }
}
