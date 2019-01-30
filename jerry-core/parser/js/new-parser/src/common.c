/* Copyright 2015 University of Szeged.
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

#include "common.h"

/**
 * Checks whether the next UTF8 character is a valid identifier start.
 *
 * @return non-zero if it is.
 */
int
util_is_identifier_start (const uint8_t *src_p) /* pointer to a vaild UTF8 character */
{
  if (*src_p > 127)
  {
    return 0;
  }

  return util_is_identifier_start_character (*src_p);
} /* util_is_identifier_start */

/**
 * Checks whether the next UTF8 character is a valid identifier part.
 *
 * @return non-zero if it is.
 */
int
util_is_identifier_part (const uint8_t *src_p) /* pointer to a vaild UTF8 character */
{
  if (*src_p > 127)
  {
    return 0;
  }

  return util_is_identifier_part_character (*src_p);
} /* util_is_identifier_part */

/**
 * Checks whether the character is a valid identifier start.
 *
 * @return non-zero if it is.
 */
int
util_is_identifier_start_character (uint16_t chr) /**< EcmaScript character */
{
  return (((chr | 0x20) >= 'a' && (chr | 0x20) <= 'z')
          || chr == '$' || chr == '_');
} /* util_is_identifier_start_character */

/**
 * Checks whether the character is a valid identifier part.
 *
 * @return non-zero if it is.
 */
int
util_is_identifier_part_character (uint16_t chr) /**< EcmaScript character */
{
  return (((chr | 0x20) >= 'a' && (chr | 0x20) <= 'z')
          || (chr >= '0' && chr <= '9')
          || chr == '$' || chr == '_');
} /* util_is_identifier_part_character */

/**
 * Converts a character to UTF8 bytes.
 *
 * @return length of the UTF8 representation.
 */
size_t
util_to_utf8_bytes (uint8_t *dst_p, /**< destination buffer */
                    uint16_t chr) /**< EcmaScript character */
{
  if (!(chr & ~0x007f))
  {
    /* 00000000 0xxxxxxx -> 0xxxxxxx */
    *dst_p = (uint8_t) chr;
    return 1;
  }

  if (!(chr & ~0x07ff))
  {
    /* 00000yyy yyxxxxxx -> 110yyyyy 10xxxxxx */
    *(dst_p++) = (uint8_t) (0xc0 | ((chr >> 6) & 0x1f));
    *dst_p = (uint8_t) (0x80 | (chr & 0x3f));
    return 2;
  }

  /* zzzzyyyy yyxxxxxx -> 1110zzzz 10yyyyyy 10xxxxxx */
  *(dst_p++) = (uint8_t) (0xe0 | ((chr >> 12) & 0x0f));
  *(dst_p++) = (uint8_t) (0x80 | ((chr >> 6) & 0x3f));
  *dst_p = (uint8_t) (0x80 | (chr & 0x3f));
  return 3;
} /* util_to_utf8_bytes */

/**
 * Returns the length of the UTF8 representation of a character.
 *
 * @return length of the UTF8 representation.
 */
size_t
util_get_utf8_length (uint16_t chr) /**< EcmaScript character */
{
  if (!(chr & ~0x007f))
  {
    /* 00000000 0xxxxxxx */
    return 1;
  }

  if (!(chr & ~0x07ff))
  {
    /* 00000yyy yyxxxxxx */
    return 2;
  }

  /* zzzzyyyy yyxxxxxx */
  return 3;
} /* util_get_utf8_length */

/**
 * Compares a string literal with a character array.
 *
 * @return non-zero if they match.
 */
int
util_compare_char_literals (lexer_literal_t *literal_p, /**< literal */
                            const uint8_t *char_p) /**< character array */
{
  PARSER_ASSERT (literal_p->type == LEXER_IDENT_LITERAL
                 || literal_p->type == LEXER_STRING_LITERAL);
  return memcmp (literal_p->value.char_p, char_p, literal_p->prop.length) == 0;
} /* util_compare_char_literals */

/**
 * Get number value.
 *
 * @return non-zero if they match.
 */
int32_t
util_get_number (const uint8_t *char_p, /**< start of the number */
                 size_t size) /**< length of the number */
{
  int32_t value = 0;

  while (size > 0)
  {
    if (*char_p < '0' || *char_p > '9')
    {
      return 32768;
    }
    value = value * 10 + (*char_p++) - '0';

    if (value > 32767)
    {
      return 32768;
    }
    size--;
  }
  return value;
} /* util_get_number */

/**
 * Initializes a string literal from a character array.
 *
 * @return non-zero in an error is occured.
 */
int
util_set_char_literal (lexer_literal_t *literal_p, /**< literal */
                       const uint8_t *char_p) /**< character array */
{
  uint8_t *new_char_p;

  if (literal_p->prop.length == 0)
  {
    literal_p->value.char_p = NULL;
    return 0;
  }

  if (literal_p->status_flags & LEXER_FLAG_SOURCE_PTR)
  {
    literal_p->value.char_p = char_p;
    return 0;
  }

  new_char_p = (uint8_t *) malloc (literal_p->prop.length);
  if (new_char_p == NULL)
  {
    return 1;
  }

  memcpy (new_char_p, char_p, literal_p->prop.length);
  literal_p->value.char_p = new_char_p;
  return 0;
} /* util_set_char_literal */

/**
 * Initializes a number literal from a character array.
 *
 * @return non-zero in an error is occured.
 */
int
util_set_number_literal (lexer_literal_t *literal_p, /**< literal */
                         const uint8_t *source_p) /**< starting source code position */
{
  literal_p->value.char_p = (uint8_t *) source_p;
  return 0;
} /* util_set_number_literal */

/**
 * Initializes a regexp literal from a character array.
 *
 * @return NULL if an error is occured.
 *         otherwise returns with end position of the number
 */
int
util_set_regexp_literal (lexer_literal_t *literal_p, /**< literal */
                         const uint8_t *source_p) /**< starting source code position */
{
  literal_p->value.char_p = (uint8_t *) source_p;
  return 0;
} /* util_set_regexp_literal */

/**
 * Initializes a function literal from the argument.
 */
void
util_set_function_literal (lexer_literal_t *literal_p, /**< literal */
                           void *function_p) /**< function */
{
  literal_p->value.compiled_code_p = function_p;
} /* util_set_char_literal */

/**
 * Free literal.
 */
void
util_free_literal (lexer_literal_t *literal_p) /**< literal */
{
  if (literal_p->type == LEXER_IDENT_LITERAL
      || literal_p->type == LEXER_STRING_LITERAL)
  {
    if (literal_p->prop.length > 0
        && !(literal_p->status_flags & LEXER_FLAG_SOURCE_PTR))
    {
      free ((uint8_t *) literal_p->value.char_p);
    }
  }
  else if (literal_p->type == LEXER_FUNCTION_LITERAL)
  {
    free (literal_p->value.compiled_code_p);
  }
} /* util_free_literal */

#ifdef PARSER_DUMP_BYTE_CODE

/**
 * Utility to print a character sequence.
 */
static void
util_print_string (const uint8_t *char_p, /**< character pointer */
                   size_t size) /**< size */
{
  while (size > 0)
  {
    printf ("%c", *char_p++);
    size--;
  }
}

/**
 * Print literal.
 */
void
util_print_literal (lexer_literal_t *literal_p) /**< literal */
{
  if (literal_p->type == LEXER_IDENT_LITERAL)
  {
    if (literal_p->status_flags & LEXER_FLAG_VAR)
    {
      printf ("var_ident(");
    }
    else
    {
      printf ("ident(");
    }
  }
  else if (literal_p->type == LEXER_FUNCTION_LITERAL)
  {
    printf ("function");
    return;
  }
  else if (literal_p->type == LEXER_STRING_LITERAL)
  {
    printf ("string(");
  }
  else if (literal_p->type == LEXER_NUMBER_LITERAL)
  {
    printf ("number(");

    if (literal_p->prop.length == 0)
    {
      printf ("%d)", (int) (((intptr_t) literal_p->value.char_p) - 256));
      return;
    }
  }
  else if (literal_p->type == LEXER_REGEXP_LITERAL)
  {
    printf ("regexp(");
  }
  else
  {
    printf ("unknown");
    return;
  }

  util_print_string (literal_p->value.char_p, literal_p->prop.length);
  printf (")");
} /* util_print_literal */

#endif /* PARSER_DUMP_BYTE_CODE */
