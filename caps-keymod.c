#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/input.h>
#include <fcntl.h>
#include <stdbool.h>
#include <pthread.h>

#include "uthash.h"

int mappings[][2] = {
    {KEY_H, KEY_LEFT},
    {KEY_J, KEY_DOWN},
    {KEY_K, KEY_UP},
    {KEY_L, KEY_RIGHT},
    {KEY_C, KEY_INSERT},
    {KEY_BACKSPACE, KEY_DELETE},
    {KEY_F, KEY_PAGEUP}
};

typedef struct map {
    int key;
    int value;
    UT_hash_handle hh;
} map_t;

struct state {
    bool caps_pressed;
};

__u16 capslock_just_released = 0;

#ifdef DEBUG
#include <signal.h>
int g_fd = -1;

void close_event(int sig)
{
    (void)(sig);
    if (g_fd >= 0)
        close(g_fd);
    exit(0);
}
#endif

void *thread_capslock_just_released(void *arg)
{
    (void)(arg);
    usleep(100000); // 500 milliseconds

    capslock_just_released = 0;
    return NULL;
}

void create_keymap(map_t **map)
{
    for (long unsigned int i = 0; i < sizeof(mappings); ++i) {
        map_t *entry = calloc(1, sizeof *entry);
        if (!entry) {
            return;
        }

        entry->key = mappings[i][0];
        entry->value = mappings[i][1];
        HASH_ADD_INT(*map, key, entry);
    }
}

int read_event(struct input_event *event)
{
#ifdef DEBUG
    return read(g_fd, event, sizeof *event);
#endif
    return fread(event, sizeof(struct input_event), 1, stdin) == 1;
}

void write_event(const struct input_event *event)
{
#ifdef DEBUG
    if (event->type == EV_KEY)
        printf("event out: type=%d  code=%d  value=%d\n", event->type, event->code, event->value);
#else
    if (fwrite(event, sizeof(struct input_event), 1, stdout) != 1)
        exit(EXIT_FAILURE);
#endif
}

void write_translated_event(map_t *keymap, struct input_event *input)
{
    
    int code = input->code;
    map_t *keymap_entry = NULL;

    HASH_FIND_INT(keymap, &code, keymap_entry);
    if (keymap_entry) {
        input->code = keymap_entry->value;
    }
#ifdef DEBUG
    if (input->type == EV_KEY)
        printf("event out: type=%d  code=%d  value=%d\n", input->type, input->code, input->value);
#else
    if (fwrite(input, sizeof(struct input_event), 1, stdout) != 1)
        exit(EXIT_FAILURE);
#endif
}

enum _state {
    MSC,
    KEY_CAPS,
    KEY_NO_CAPS,
    SYN
};

int main(int argc, char **argv)
{
    (void)(argc);
    (void)(argv);

    pthread_t thread;
    struct input_event input;
    map_t *keymap = NULL;
    __u16 capslock_pressed = 0;

    #ifdef DEBUG
    signal(SIGINT, close_event);
    if (argc != 2) {
        printf("Please specify a /dev/input/eventX\n");
        return 1;
    }
    g_fd = open(argv[1], O_RDONLY);
    #endif

    setbuf(stdin, NULL), setbuf(stdout, NULL);

    create_keymap(&keymap);

    while (read_event(&input)) {
        if (input.type == EV_KEY) {
#ifdef DEBUG
            printf("event in: type=%d  code=%d  value=%d\n", input.type, input.code, input.value);
#endif
            if (input.code == KEY_CAPSLOCK) {
                if (input.value == 1 || input.value == 2) {
                    capslock_pressed = 1;
                }

                if (input.value == 0) {
                    capslock_pressed = 0;
                    capslock_just_released = 1;
                    pthread_create(&thread, NULL, thread_capslock_just_released, NULL);
                }

            } else if (capslock_pressed || capslock_just_released) {
                write_translated_event(keymap, &input);
                    capslock_just_released = 0;


            } else {
                write_event(&input);
            }

        } else {
            write_event(&input);
        }
    }

    #ifdef DEBUG
    close_event(0);
    #endif

    pthread_join(thread, NULL);
    return 0;
}
