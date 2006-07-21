/*
 * (C) 2001 Clemson University and The University of Chicago
 *
 * See COPYING in top-level directory.
 */

/** \file
 *  \ingroup statecomp
 *
 * Code generation routines for statecomp.
 */

#include <stdio.h>
#include <stdlib.h>

#include "state-machine-values.h"

extern FILE *out_file;
extern int terminate_path_flag;
static char *current_machine;

void gen_init(
    void);
void gen_state_decl(
    char *state_name);
void gen_machine(
    char *machine_name,
    char *first_state_name);
void gen_state_start(
    char *state_name);
void gen_state_action(
    char *run_func,
    int flag);
void gen_return_code(
    char *return_code);
void gen_next_state(
    int flag,
    char *new_state);
void gen_state_end(
    void);

void gen_init(
    void)
{
    return;
}

void gen_state_decl(
    char *state_name)
{
    fprintf(out_file, "static union PINT_state_array_values ST_%s[];\n",
            state_name);
}

void gen_machine(
    char *machine_name,
    char *first_state_name)
{
    current_machine = machine_name;
    fprintf(out_file, "\nstruct PINT_state_machine_s %s = {\n", machine_name);
    fprintf(out_file, "\t.name = \"%s\",\n", machine_name);
    fprintf(out_file, "\t.state_machine = ST_%s\n", first_state_name);
    fprintf(out_file, "};\n\n");
}

void gen_state_start(
    char *state_name)
{
    fprintf(out_file,
            "static union PINT_state_array_values ST_%s[] = {\n"
            "\t{ .state_name = \"%s\" },\n"
            "\t{ .parent_machine = &%s },\n",
            state_name, state_name, current_machine);
}

/** generates first two lines in the state machine (I think),
 * the first one indicating what kind of action it is ("run"
 * or "jump") and the second being the action itself (either a
 * function or a nested state machine).
 */
void gen_state_action(
    char *run_func,
    int flag)
{
    switch (flag)
    {
    case SM_NONE:
        fprintf(out_file, "\t{ .flag = SM_NONE },\n");
        fprintf(out_file, "\t{ .state_action = %s }", run_func);
        break;
    case SM_JUMP:
        fprintf(out_file, "\t{ .flag = SM_JUMP },\n");
        fprintf(out_file, "\t{ .nested_machine = &%s }", run_func);
        break;
    default:
        fprintf(stderr, "invalid flag associated with action %s\n", run_func);
        exit(1);
    }
}

void gen_return_code(
    char *return_code)
{
    fprintf(out_file, ",\n\t{ .return_value = %s }", return_code);
}

void gen_next_state(
    int flag,
    char *new_state)
{
    switch (flag)
    {
    case SM_NEXT:
        fprintf(out_file, ",\n\t{ .next_state = ST_%s }", new_state);
        break;
    case SM_RETURN:
        terminate_path_flag = 1;
        fprintf(out_file, ",\n\t{ .flag = SM_RETURN }");
        break;
    case SM_TERMINATE:
        terminate_path_flag = 1;
        fprintf(out_file, ",\n\t{ .flag = SM_TERMINATE }");
        break;
    default:
        fprintf(stderr, "invalid flag associated with target (no more info)\n");
        exit(1);
    }
}

void gen_state_end(
    void)
{
    fprintf(out_file, "\n};\n\n");
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=8 sts=4 sw=4 expandtab
 */
