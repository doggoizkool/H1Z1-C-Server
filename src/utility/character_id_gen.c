#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#define GUID_LENGTH 6
#define MAX_GUIDS 2048

u64 generate_guid() {
    u64 guid = 0;
    u64 random_num;
    srand((unsigned) time(NULL));
    for (int i = 0; i < GUID_LENGTH; i++) {
        random_num = (u64)rand() % 16;
        guid |= (random_num << (4 * (GUID_LENGTH - i - 1)));
    }
    return 0x0fuLL << 56 | guid & 0x7FFFFFFFFFFFFFFF; // add 0x0f to the most significant byte and "0x" to the return value
}

u64 get_guid() {
    // Initialize array to hold guids
    static u64 guids[MAX_GUIDS] = { 0 };
    // Find an empty slot to store guid
    int i = 0;
    while (guids[i] != 0 && i < MAX_GUIDS) {
        i++;
    }
    if (i >= MAX_GUIDS) {
        fprintf(stderr, "No more slots available to store guid\n");
        return 0;
    }
    // Generate guid and store it in the array
    if (guids[i] == 0) {
        guids[i] = generate_guid();
    }
    return 0x0fuLL << 56 | guids[i] & 0x7FFFFFFFFFFFFFFF; // add 0x0f to the most significant byte and "0x" to the returned value
}

void delete_guid(int delete_status) {
    // If delete_status is true, delete all stored guids
    if (delete_status == 1) {
        static u64 guids[MAX_GUIDS] = { 0 };
        memset(guids, 0, sizeof(guids)); // use memset to clear the array
    }
}