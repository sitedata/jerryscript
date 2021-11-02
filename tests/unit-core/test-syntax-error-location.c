/*
 * Copyright JS Foundation and other contributors, http://js.foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "jerryscript.h"

#include "test-common.h"

static void
compare_string (jerry_value_t left_string_value, /**< left value to compare */
                const char *right_string_p) /**< right value to compare */
{
  TEST_ASSERT (jerry_value_is_string (left_string_value));

  jerry_char_t buffer[128];
  size_t size = strlen (right_string_p);

  TEST_ASSERT (size < sizeof (buffer));
  TEST_ASSERT (size == jerry_get_string_size (left_string_value));

  jerry_string_to_char_buffer (left_string_value, buffer, (jerry_size_t) size);
  TEST_ASSERT (memcmp (buffer, right_string_p, size) == 0);
} /* compare_string */

static void
compare_location (jerry_syntax_error_location_t *location_p, /**< expected location */
                  uint32_t line, /**< start line of the invalid token */
                  uint32_t column_start, /**< start column of the invalid token */
                  uint32_t column_end) /**< end column of the invalid token */
{
  TEST_ASSERT (location_p->line == line);
  TEST_ASSERT (location_p->column_start == column_start);
  TEST_ASSERT (location_p->column_end == column_end);
} /* compare_location */

int
main (void)
{
  TEST_INIT ();

  if (!jerry_is_feature_enabled (JERRY_FEATURE_ERROR_MESSAGES))
  {
    return 0;
  }

  jerry_init (JERRY_INIT_EMPTY);

  jerry_syntax_error_location_t error_location;

  jerry_value_t error_value = jerry_create_number (13);
  jerry_value_t resource_value = jerry_get_syntax_error_location (error_value, NULL);
  TEST_ASSERT (jerry_value_is_error (resource_value));
  jerry_release_value (resource_value);
  jerry_release_value (error_value);

  char *source_p = TEST_STRING_LITERAL ("new SyntaxError('Bad token!')");
  error_value = jerry_eval ((jerry_char_t *) source_p, strlen (source_p), JERRY_PARSE_NO_OPTS);
  TEST_ASSERT (jerry_get_error_type (error_value) == JERRY_ERROR_SYNTAX);
  error_location.line = 100;
  error_location.column_start = 200;
  error_location.column_end = 300;
  resource_value = jerry_get_syntax_error_location (error_value, &error_location);
  /* This SyntaxError is not generated by the parser. */
  TEST_ASSERT (jerry_value_is_error (resource_value));
  compare_location (&error_location, 100, 200, 300);
  jerry_release_value (resource_value);
  jerry_release_value (error_value);

  source_p = TEST_STRING_LITERAL ("\n\naa bb1 cc");
  error_value = jerry_parse ((jerry_char_t *) source_p, strlen (source_p), NULL);
  TEST_ASSERT (jerry_get_error_type (error_value) == JERRY_ERROR_SYNTAX);
  resource_value = jerry_get_syntax_error_location (error_value, NULL);
  compare_string (resource_value, "<anonymous>");
  jerry_release_value (resource_value);
  resource_value = jerry_get_syntax_error_location (error_value, &error_location);
  compare_string (resource_value, "<anonymous>");
  compare_location (&error_location, 3, 4, 7);
  jerry_release_value (resource_value);
  jerry_release_value (error_value);

  source_p = TEST_STRING_LITERAL ("var s = '1234567890'\n"
                                  "for (var i = 0; i < 6; i++) {\n"
                                  "  s += s\n"
                                  "}\n"
                                  "eval('aa \"' + s + '\"')");
  error_value = jerry_eval ((jerry_char_t *) source_p, strlen (source_p), JERRY_PARSE_NO_OPTS);
  TEST_ASSERT (jerry_get_error_type (error_value) == JERRY_ERROR_SYNTAX);
  error_value = jerry_get_value_from_error (error_value, true);
  TEST_ASSERT (!jerry_value_is_error (error_value));
  resource_value = jerry_get_syntax_error_location (error_value, &error_location);
  compare_string (resource_value, "<eval>");
  compare_location (&error_location, 1, 4, 646);
  jerry_release_value (resource_value);
  jerry_release_value (error_value);

  jerry_parse_options_t parse_options;
  parse_options.options = JERRY_PARSE_HAS_RESOURCE | JERRY_PARSE_HAS_START;
  parse_options.resource_name = jerry_create_string ((const jerry_char_t *) "[generated.js:1:2]");
  parse_options.start_line = 1234567890;
  parse_options.start_column = 1234567890;

  source_p = TEST_STRING_LITERAL ("aa(>>=2)");
  error_value = jerry_parse ((jerry_char_t *) source_p, strlen (source_p), &parse_options);
  TEST_ASSERT (jerry_get_error_type (error_value) == JERRY_ERROR_SYNTAX);
  resource_value = jerry_get_syntax_error_location (error_value, &error_location);
  compare_string (resource_value, "[generated.js:1:2]");
  compare_location (&error_location, 1234567890, 1234567893, 1234567896);
  jerry_release_value (resource_value);
  jerry_release_value (error_value);

  source_p = TEST_STRING_LITERAL ("\n\n\nabcd 'ab\\\ncd\\\ne'");
  error_value = jerry_parse ((jerry_char_t *) source_p, strlen (source_p), &parse_options);
  TEST_ASSERT (jerry_get_error_type (error_value) == JERRY_ERROR_SYNTAX);
  resource_value = jerry_get_syntax_error_location (error_value, &error_location);
  compare_string (resource_value, "[generated.js:1:2]");
  compare_location (&error_location, 1234567893, 6, 10);
  jerry_release_value (resource_value);
  jerry_release_value (error_value);

  jerry_release_value (parse_options.resource_name);

  jerry_cleanup ();
  return 0;
} /* main */
