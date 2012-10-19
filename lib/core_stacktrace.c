/*
    core_stacktrace.c

    Copyright (C) 2012  Red Hat, Inc.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#include "core_stacktrace.h"
#include "core_frame.h"
#include "core_thread.h"
#include "gdb_stacktrace.h"
#include "gdb_frame.h"
#include "gdb_thread.h"
#include "location.h"
#include "normalize.h"
#include "utils.h"
#include "strbuf.h"
#include "unstrip.h"
#include <ctype.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

struct btp_core_stacktrace *
btp_core_stacktrace_new()
{
    struct btp_core_stacktrace *stacktrace =
        btp_malloc(sizeof(struct btp_core_stacktrace));

    btp_core_stacktrace_init(stacktrace);
    return stacktrace;
}

void
btp_core_stacktrace_init(struct btp_core_stacktrace *stacktrace)
{
    stacktrace->threads = NULL;
}

void
btp_core_stacktrace_free(struct btp_core_stacktrace *stacktrace)
{
    if (!stacktrace)
        return;

    while (stacktrace->threads)
    {
        struct btp_core_thread *thread = stacktrace->threads;
        stacktrace->threads = thread->next;
        btp_core_thread_free(thread);
    }

    free(stacktrace);
}

struct btp_core_stacktrace *
btp_core_stacktrace_dup(struct btp_core_stacktrace *stacktrace)
{
    struct btp_core_stacktrace *result = btp_core_stacktrace_new();
    memcpy(result, stacktrace, sizeof(struct btp_core_stacktrace));

    if (stacktrace->threads)
        result->threads = btp_core_thread_dup(stacktrace->threads, true);

    return result;
}

int
btp_core_stacktrace_get_thread_count(struct btp_core_stacktrace *stacktrace)
{
    struct btp_core_thread *thread = stacktrace->threads;
    int count = 0;
    while (thread)
    {
        thread = thread->next;
        ++count;
    }
    return count;
}

struct btp_core_stacktrace *
btp_core_stacktrace_parse(const char **input,
                         struct btp_location *location)
{
    // TODO
    return NULL;
}

char *
btp_core_stacktrace_to_json(struct btp_core_stacktrace *stacktrace)
{
    struct btp_strbuf *strbuf = btp_strbuf_new();
    btp_strbuf_append_strf(strbuf,
                           "{   \"signal\": %"PRIu8",\n",
                           stacktrace->signal);

    btp_strbuf_append_str(strbuf, "    \"threads\": [\n");

    struct btp_core_thread *thread = stacktrace->threads;
    while (thread)
    {
        char *thread_json = btp_core_thread_to_json(thread);
        char *indented_thread_json = btp_indent(thread_json, 8);
        btp_strbuf_append_str(strbuf, indented_thread_json);
        free(indented_thread_json);
        free(thread_json);
        thread = thread->next;
        if (thread)
            btp_strbuf_append_str(strbuf, ",\n");
    }

    btp_strbuf_append_str(strbuf, "    ]\n");
    btp_strbuf_append_str(strbuf, "}");
    return btp_strbuf_free_nobuf(strbuf);
}

struct btp_core_stacktrace *
btp_core_stacktrace_create(const char *gdb_stacktrace_text,
                          const char *unstrip_text,
                          const char *executable_path)
{
    // Parse the GDB stacktrace.
    struct btp_location location;
    btp_location_init(&location);

    struct btp_gdb_stacktrace *gdb_stacktrace =
        btp_gdb_stacktrace_parse(&gdb_stacktrace_text, &location);

    if (!gdb_stacktrace)
    {
        if (btp_debug_parser)
        {
            fprintf(stderr, "Unable to parse stacktrace: %d:%d: %s\n",
                    location.line, location.column, location.message);
        }

        return NULL;
    }

    // Parse the unstrip output.
    struct btp_unstrip_entry *unstrip = btp_unstrip_parse(unstrip_text);
    if (!unstrip)
    {
        if (btp_debug_parser)
            fprintf(stderr, "Unable to parse unstrip output.");

        return NULL;
    }

    // Create the core stacktrace
    struct btp_core_stacktrace *core_stacktrace =
        btp_core_stacktrace_new();

    struct btp_gdb_thread *gdb_thread = gdb_stacktrace->threads;
    while (gdb_thread)
    {
        struct btp_core_thread *core_thread = btp_core_thread_new();

        struct btp_gdb_frame *gdb_frame = gdb_thread->frames;
        while (gdb_frame)
        {
            gdb_frame = gdb_frame->next;

            struct btp_core_frame *core_frame = btp_core_frame_new();
            core_frame->address = gdb_frame->address;

            struct btp_unstrip_entry *unstrip_entry =
                btp_unstrip_find_address(unstrip, gdb_frame->address);

            if (unstrip_entry)
            {
                core_frame->build_id = btp_strdup(unstrip_entry->build_id);
                core_frame->build_id_offset = core_frame->address - unstrip_entry->start;
                core_frame->file_name = btp_strdup(unstrip_entry->file_name);
            }

            if (gdb_frame->function_name &&
                0 != strcmp(gdb_frame->function_name, "??"))
            {
                core_frame->function_name =
                    btp_strdup(gdb_frame->function_name);
            }
        }

        core_stacktrace->threads =
            btp_core_thread_append(core_stacktrace->threads,
                                   core_thread);

        gdb_thread = gdb_thread->next;
    }

    return core_stacktrace;
}