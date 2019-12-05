#include <lib-common/core.h>
#include <lib-common/container.h>
#include <lib-common/parseopt.h>

typedef struct coords_t {
    int x;
    int y;
} coords_t;

static int ps_read_step(pstream_t *ps, char *dir, int *length)
{
    if (!ps_has(ps, 2)) {
        return -1;
    }

    *dir = __ps_getc(ps);
    *length = ps_geti(ps);

    if (ps_has(ps, 1) && ps_skipc(ps, ',')) {
        return -1;
    }

    return 0;
}

typedef void (*step_f)(coords_t *cursor, char dir, void *ctx);

static void execute_step(coords_t *cursor, char dir, int length,
                         step_f cb, void *ctx)
{
    int increment = 1;

    switch (dir) {
      case 'L':
        increment = -1;
        /* FALLTHROUGH */
      case 'R':
        for (int steps = 0; steps < length; steps++) {
            cursor->x += increment;
            cb(cursor, dir, ctx);
        }
        break;

      case 'D':
        increment = -1;
        /* FALLTHROUGH */
      case 'U':
        for (int steps = 0; steps < length; steps++) {
            cursor->y += increment;
            cb(cursor, dir, ctx);
        }
        break;
    }
}

typedef struct read_bounds_ctx_t {
    coords_t sw_corner;
    coords_t ne_corner;
} read_bounds_ctx_t;

GENERIC_FUNCTIONS(read_bounds_ctx_t, read_bounds_ctx);

static void read_bounds(coords_t *cursor, char dir, void *_ctx)
{
    read_bounds_ctx_t *ctx = _ctx;

    if (ctx->sw_corner.x > cursor->x) {
        ctx->sw_corner.x = cursor->x;
    }
    if (ctx->sw_corner.y > cursor->y) {
        ctx->sw_corner.y = cursor->y;
    }
    if (ctx->ne_corner.x < cursor->x) {
        ctx->ne_corner.x = cursor->x;
    }
    if (ctx->ne_corner.y < cursor->y) {
        ctx->ne_corner.y = cursor->y;
    }
}

typedef struct cell_t {
    char c;
    int wire1_steps;
} cell_t;

typedef struct map_t {
    int part1;
    int part2;
    int wire_number;
    int step_counter;
    int x0;
    int y0;
    int size_x;
    int size_y;
    cell_t* map;
} map_t;

static map_t *map_init(map_t *map)
{
    p_clear(map, 1);
    map->part1 = INT_MAX;
    map->part2 = INT_MAX;
    return map;
}
static void map_wipe(map_t *map)
{
    p_delete(&map->map);
}

static inline cell_t *map_ref(map_t *map, int x, int y) {
    if (x + map->x0 < 0 || x + map->x0 >= map->size_x
    ||  y + map->y0 < 0 || y + map->y0 >= map->size_y)
    {
        e_panic("out of bounds");
    }
    return map->map + x + map->x0 + (y + map->y0) * map->size_x;
}

static void detect_cross(coords_t *cursor, char dir, void *_ctx)
{
    map_t *map = _ctx;
    cell_t *c = map_ref(map, cursor->x, cursor->y);

    if (c->c != '\0' && c->c < map->wire_number + '0') {
        // cross detected
        int dist = abs(cursor->x) + abs(cursor->y);

        if (map->part1 > dist) {
            map->part1 = dist;
        }

        c->c = 'X';
    } else {
        c->c = map->wire_number + '0';
    }
}

static void detect_cross_part2_wire1(coords_t *cursor, char dir, void *_ctx)
{
    map_t *map = _ctx;
    cell_t *c = map_ref(map, cursor->x, cursor->y);

    map->step_counter++;

    c->c = map->wire_number + '0';
    if (c->wire1_steps < map->step_counter) {
        c->wire1_steps = map->step_counter;
    }
}

static void detect_cross_part2_wire2(coords_t *cursor, char dir, void *_ctx)
{
    map_t *map = _ctx;
    cell_t *c = map_ref(map, cursor->x, cursor->y);

    map->step_counter++;

    if (c->c != '\0' && c->c < map->wire_number + '0') {
        int cost = map->step_counter + c->wire1_steps;

        if (map->part2 > cost) {
            map->part2 = cost;
        }
        c->c = 'X';
    } else {
        c->c = map->wire_number + '0';
    }
}

int main(int argc, char **argv)
{
    lstr_t input;
    pstream_t parser;
    read_bounds_ctx_t bounds;
    map_t map;
    bool print_map = false;
    bool help = false;
    const char *arg0 = NEXTARG(argc, argv);
    const char *input_file;

    popt_t options[] = {
        OPT_FLAG('h', "help", &help, "show help"),
        OPT_FLAG('p', "print", &print_map, "print the map"),
        OPT_END(),
    };

    argc = parseopt(argc, argv, options, 0);
    if (argc < 1 || help) {
        makeusage(!help, arg0, "<input file>", NULL, options);
    }

    read_bounds_ctx_init(&bounds);
    map_init(&map);

    input_file = NEXTARG(argc, argv);

    if (lstr_init_from_file(&input, input_file, PROT_READ, MAP_SHARED) < 0) {
        e_fatal("failed to open %s: %m", input_file);
    }

    parser = ps_initlstr(&input);

    // Read bounds
    while (!ps_done(&parser)) {
        pstream_t line;
        coords_t cursor;

        cursor.x = 0;
        cursor.y = 0;

        if (ps_get_ps_chr_and_skip(&parser, '\n', &line) < 0) {
            e_fatal("failed to read line");
        }

        while (!ps_done(&line)) {
            char dir;
            int length;

            if (ps_read_step(&line, &dir, &length) < 0) {
                e_fatal("failed to read step");
            }

            execute_step(&cursor, dir, length, read_bounds, &bounds);
        }
    }

    // Central port
    map.size_x = bounds.ne_corner.x - bounds.sw_corner.x + 1;
    map.x0 = -bounds.sw_corner.x;
    map.size_y = bounds.ne_corner.y - bounds.sw_corner.y + 1;
    map.y0 = -bounds.sw_corner.y;

    e_info("detected map of size %d x %d, centered at %d, %d",
           map.size_x, map.size_y, map.x0, map.y0);

    map.map = p_new(cell_t, map.size_x * map.size_y);

    map_ref(&map, 0, 0)->c = 'o';

    parser = ps_initlstr(&input);
    while (!ps_done(&parser)) {
        pstream_t line;
        coords_t cursor;

        cursor.x = 0;
        cursor.y = 0;

        if (ps_get_ps_chr_and_skip(&parser, '\n', &line) < 0) {
            e_fatal("failed to read line");
        }

        map.wire_number++;

        while (!ps_done(&line)) {
            char dir;
            int length;

            if (ps_read_step(&line, &dir, &length) < 0) {
                e_fatal("failed to read step");
            }

            execute_step(&cursor, dir, length, detect_cross, &map);
        }
    }

    if (print_map) {
        for (int j = bounds.ne_corner.y; j >= bounds.sw_corner.y; j--) {
            for (int i = bounds.sw_corner.x; i <= bounds.ne_corner.x; i++) {
                char c = map_ref(&map, i, j)->c;
                if (c == '\0') {
                    printf(" ");
                } else {
                    printf("%c", c);
                }
            }
            printf("\n");
        }
    }

    e_info("part1: %d", map.part1);

    p_clear(map.map, map.size_x * map.size_y);

    map_ref(&map, 0, 0)->c = 'o';
    map.wire_number = 0;

    parser = ps_initlstr(&input);
    while (!ps_done(&parser)) {
        pstream_t line;
        coords_t cursor;

        cursor.x = 0;
        cursor.y = 0;

        if (ps_get_ps_chr_and_skip(&parser, '\n', &line) < 0) {
            e_fatal("failed to read line");
        }

        map.wire_number++;
        map.step_counter = 0;

        while (!ps_done(&line)) {
            char dir;
            int length;

            if (ps_read_step(&line, &dir, &length) < 0) {
                e_fatal("failed to read step");
            }

            switch (map.wire_number) {
              case 1:
                execute_step(&cursor, dir, length, detect_cross_part2_wire1, &map);
                break;

              case 2:
                execute_step(&cursor, dir, length, detect_cross_part2_wire2, &map);
                break;
            }
        }
    }

    if (print_map) {
        for (int j = bounds.ne_corner.y; j >= bounds.sw_corner.y; j--) {
            for (int i = bounds.sw_corner.x; i <= bounds.ne_corner.x; i++) {
                char c = map_ref(&map, i, j)->c;
                if (c == '\0') {
                    printf(" ");
                } else {
                    printf("%c", c);
                }
            }
            printf("\n");
        }
    }

    e_info("part2: %d", map.part2);

    map_wipe(&map);
    read_bounds_ctx_wipe(&bounds);

    return 0;
}
