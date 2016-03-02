/*
 * Copyright (c) 2015, 2016 Jonas 'Sortie' Termansen.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * interactive.h
 * Interactive utility functions.
 */

#ifndef INTERACTIVE_H
#define INTERACTIVE_H

extern const char* prompt_man_section;
extern const char* prompt_man_page;

void shlvl(void);
void text(const char* str);
__attribute__((format(printf, 1, 2)))
void textf(const char* format, ...);
void prompt(char* buffer,
            size_t buffer_size,
            const char* question,
            const char* answer);
void password(char* buffer,
              size_t buffer_size,
              const char* question);
bool missing_program(const char* program);

#endif
