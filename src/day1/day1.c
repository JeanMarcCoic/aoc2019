#include <lib-common/core.h>

static int compute_fuel_req(int mass)
{
    return (mass / 3) - 2;
}

int main(int argc, char **argv)
{
    lstr_t input;
    pstream_t parser;
    int sum_fuel = 0;
    int sum_fuel_part2 = 0;
    int additional_fuel;

    if (argc < 2) {
        e_fatal("missing argument: path");
    }

    if (lstr_init_from_file(&input, argv[1], PROT_READ, MAP_SHARED) < 0) {
        e_fatal("failed to open %s: %m", argv[1]);
    }

    parser = ps_initlstr(&input);

    while (!ps_done(&parser)) {
        int mass = ps_geti(&parser);
        int fuel = compute_fuel_req(mass);
        int add_fuel_part2 = 0;

        if (ps_skip_afterchr(&parser, '\n') < 0) {
            e_fatal("missing end of line");
        }

        additional_fuel = compute_fuel_req(fuel);

        while (additional_fuel > 0) {
            add_fuel_part2 += additional_fuel;
            additional_fuel = compute_fuel_req(additional_fuel);
        }

        sum_fuel += fuel;
        sum_fuel_part2 += fuel + add_fuel_part2;
    }

    e_info("part1: %d", sum_fuel);
    e_info("part2: %d", sum_fuel_part2);

    lstr_wipe(&input);

    return 0;
}
