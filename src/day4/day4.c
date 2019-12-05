#include <lib-common/core.h>
#include <lib-common/container.h>
#include <lib-common/parseopt.h>

static void is_code_valid(char buffer[7], bool *part1, bool *part2)
{
    bool has_same_adjacent = false;
    bool always_increase = true;
    bool has_one_2adj = false;

    for (int i = 0; i < 5; i++) {
        if (buffer[i] == buffer[i + 1]) {
            has_same_adjacent = true;

            if ((i == 0 || buffer[i - 1] != buffer[i])
            &&  (i == 4 || buffer[i] != buffer[i + 2]))
            {
                has_one_2adj = true;
            }
        }
        if (buffer[i] > buffer[i + 1]) {
            always_increase = false;
        }
    }

    *part1 = has_same_adjacent && always_increase;
    *part2 = *part1 && has_one_2adj;
}

static void count_codes(int range_min, int range_max, int *part1, int *part2)
{
    int count1 = 0;
    int count2 = 0;
    char buffer[7];

    for (int i = range_min; i <= range_max; i++) {
        bool valid1, valid2;
        snprintf(buffer, sizeof(buffer), "%06d", i);

        is_code_valid(buffer, &valid1, &valid2);

        if (valid1) {
            count1++;
        }
        if (valid2) {
            count2++;
        }
    }

    *part1 = count1;
    *part2 = count2;
}

int main(int argc, char **argv)
{
    pstream_t parser;
    int part1, part2;
    bool help = false;
    const char *arg0 = NEXTARG(argc, argv);
    const char *input;
    int range_min, range_max;

    popt_t options[] = {
        OPT_FLAG('h', "help", &help, "show help"),
        OPT_END(),
    };

    argc = parseopt(argc, argv, options, 0);
    if (argc < 1 || help) {
        makeusage(!help, arg0, "<input range>", NULL, options);
    }

    input = NEXTARG(argc, argv);

    parser = ps_initstr(input);

    if (ps_len(&parser) != strlen("XXXXXX-XXXXXX")) {
        e_fatal("invalid input range length");
    }

    range_min = ps_geti(&parser);

    if (!ps_has(&parser, 1) || ps_skipc(&parser, '-') < 0) {
        e_fatal("invalid input range format");
    }

    range_max = ps_geti(&parser);

    count_codes(range_min, range_max, &part1, &part2);

    e_info("part1: %d", part1);
    e_info("part2: %d", part2);

    return 0;
}
