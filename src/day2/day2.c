#include <lib-common/core.h>
#include <lib-common/container.h>

static int32_t prog_execute(qv_t(i32) *prog, int noun, int verb)
{
    int result;
    qv_t(i32) positions;

    qv_init(&positions);

    qv_copy(&positions, prog);
#define POS(i)  (positions.tab[(i)])
    POS(1) = noun;
    POS(2) = verb;

    for (int pos = 0; pos < positions.len; pos += 4) {
        switch (positions.tab[pos]) {
          case 1:
            if (pos + 3 >= positions.len) {
                e_panic("missing opcodes for op 1");
            }
            POS(POS(pos + 3)) = POS(POS(pos + 1)) + POS(POS(pos + 2));
            break;

          case 2:
            if (pos + 3 >= positions.len) {
                e_panic("missing opcodes for op 2");
            }
            POS(POS(pos + 3)) = POS(POS(pos + 1)) * POS(POS(pos + 2));
            break;

          case 99:
            goto prog_end;
        }
    }
  prog_end:

    result = POS(0);

    qv_wipe(&positions);

    return result;
}

int main(int argc, char **argv)
{
    lstr_t input;
    pstream_t parser;
    qv_t(i32) program;
    int part1;

    qv_init(&program);

    if (argc < 2) {
        e_fatal("missing argument: path");
    }

    if (lstr_init_from_file(&input, argv[1], PROT_READ, MAP_SHARED) < 0) {
        e_fatal("failed to open %s: %m", argv[1]);
    }

    parser = ps_initlstr(&input);

    while (!ps_done(&parser)) {
        int cell = ps_geti(&parser);

        qv_append(&program, cell);

        if (ps_skip_afterchr(&parser, ',') < 0) {
            break;
        }
    }

    part1 = prog_execute(&program, 12, 2);
    e_info("part1: %d", part1);

    for (int noun = 0; noun < 100; noun++) {
        for (int verb = 0; verb < 100; verb++) {
            int part2 = prog_execute(&program, noun, verb);

            if (part2 == 19690720) {
                e_info("part2: %d", noun * 100 + verb);
                goto end;
            }
        }
    }

  end:
    qv_wipe(&program);

    return 0;
}
