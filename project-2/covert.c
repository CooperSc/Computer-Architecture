#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#ifdef _MSC_VER
#include <intrin.h> /* for rdtscp and clflush */
#pragma optimize("gt", on)
#else
#include <x86intrin.h> /* for rdtscp and clflush */
#endif

// Access hardware timestamp counter
#define RDTSC(cycles) __asm__ volatile("rdtsc" \
                                       : "=a"(cycles));

// Serialize execution
#define CPUID() asm volatile("CPUID" \
                             :       \
                             :       \
                             : "%rax", "%rbx", "%rcx", "%rdx");

// Intrinsic CLFLUSH for FLUSH+RELOAD attack
#define CLFLUSH(address) _mm_clflush(address);

#define SAMPLES 275 // TODO: CONFIGURE THIS

#define L1_CACHE_SIZE (32 * 1024)
#define LINE_SIZE 64
#define ASSOCIATIVITY 4
#define L1_NUM_SETS (L1_CACHE_SIZE / (LINE_SIZE * ASSOCIATIVITY)) // 64
#define NUM_OFFSET_BITS 6
#define NUM_INDEX_BITS 7
#define NUM_OFF_IND_BITS (NUM_OFFSET_BITS + NUM_INDEX_BITS)

uint64_t eviction_counts[L1_NUM_SETS] = {0};
__attribute__((aligned(64))) uint64_t trojan_array[32 * 4096];
__attribute__((aligned(64))) uint64_t spy_array[4096];

/* TODO:
 * This function provides an eviction set address, given the
 * base address of a trojan/spy array, the required cache
 * set ID, and way ID.
 *
 * Describe the algorithm used here.
 *
 * The first line of code right shifts the base address by the number
 * of tag and offset bits to create a variable of just the tag bits
 * of the base address.  The next line right shifts by the offset bits
 * and uses a 6-bit mask with an and to only keep the 6 bits that are 
 * the index bits.  Then the code checks if the index value is greater
 * than the set index for which it'll create a pointer with the same tag
 * and offset and a tag that is incremented by the way + 1.  For calls where
 * the index bit is not greater than the set, it increments by just the way. 
 *
 *
 *
 *
 */
uint64_t *get_eviction_set_address(uint64_t *base, int set, int way)
{
    uint64_t tag_bits = (((uint64_t)base) >> NUM_OFF_IND_BITS);  // off_ind_bits initially = 12, so 64-12 = 52 tag bits. tag_bits is shifted
                                                                 // by this amount so the right 52 bits is just the tag
    int idx_bits = (((uint64_t)base) >> NUM_OFFSET_BITS) & 0x7f; // shift right by 6, and logic AND it with 111111, get the index (0 - 2^6-1)

    if (idx_bits > set)
    {
        return (uint64_t *)((((tag_bits << NUM_INDEX_BITS) + (L1_NUM_SETS + set)) << NUM_OFFSET_BITS) + (L1_NUM_SETS * LINE_SIZE * way));
    }
    else
    {
        return (uint64_t *)((((tag_bits << NUM_INDEX_BITS) + set) << NUM_OFFSET_BITS) + (L1_NUM_SETS * LINE_SIZE * way));
    }
}

/* This function sets up a trojan/spy eviction set using the
 * function above.  The eviction set is essentially a linked
 * list that spans all ways of the conflicting cache set.
 *
 * i.e., way-0 -> way-1 -> ..... way-7 -> NULL
 *
 */
void setup(uint64_t *base, int assoc)
{
    uint64_t i, j;
    uint64_t *eviction_set_addr;

    // Prime the cache set by set (i.e., prime all lines in a set)
    for (i = 0; i < L1_NUM_SETS; i++)
    {
        eviction_set_addr = get_eviction_set_address(base, i, 0);
        for (j = 1; j < assoc; j++)
        {
            *eviction_set_addr = (uint64_t)get_eviction_set_address(base, i, j);
            eviction_set_addr = (uint64_t *)*eviction_set_addr;
        }
        *eviction_set_addr = 0;
    }
}

/* TODO:
 *
 * This function implements the trojan that sends a message
 * to the spy over the cache covert channel.  Note that the
 * message forgoes case sensitivity to maximize the covert
 * channel bandwidth.
 *
 * Your job is to use the right eviction set to mount an
 * appropriate PRIME+PROBE or FLUSH+RELOAD covert channel
 * attack.  Remember that in both these attacks, we only need
 * to time the spy and not the trojan.
 *
 * Note that you may need to serialize execution wherever
 * appropriate.
 */
void trojan(char byte)
{
    int set;
    uint64_t *eviction_set_addr;

    if (byte >= 'a' && byte <= 'z')
    {
        byte -= 32;
    }
    if (byte == 10 || byte == 13)
    { // encode a new line
        set = 63;
    }
    else if (byte >= 32 && byte < 96)
    {
        set = (byte - 32);
    }
    else
    {
        printf("pp trojan: unrecognized character %c\n", byte);
        exit(1);
    }
    // printf("set");
    // printf("%d", set);
    uint64_t i, j;
    // use get_eviction_set_address to get the start of the eviction_set_addr given set = set and way = 0
    eviction_set_addr = get_eviction_set_address(trojan_array, set, 0);
    // uint64_t base = (uint64_t)eviction_set_addr;
    // printf("%d", (int)((uint64_t)base));
    // printf("base");
    // printf("\n");
    // for (i = 1; i < ASSOCIATIVITY + 1; i++)
    // {
    //     // printf("%d", (int)((uint64_t)eviction_set_addr - base));
    //     // printf("\n");
    //     eviction_set_addr = (uint64_t *)*eviction_set_addr; //cur node = cur node. next?
    //     // *eviction_set_addr = eviction_set_addr;
    // }
    while(eviction_set_addr = (uint64_t *)*eviction_set_addr){

    }
}

/* TODO:
 *
 * This function implements the spy that receives a message
 * from the trojan over the cache covert channel.  Evictions
 * are timed using appropriate hardware timestamp counters
 * and recorded in the eviction_counts array.  In particular,
 * only record evictions to the set that incurred the maximum
 * penalty in terms of its access time.
 *
 * Your job is to use the right eviction set to mount an
 * appropriate PRIME+PROBE or FLUSH+RELOAD covert channel
 * attack.  Remember that in both these attacks, we only need
 * to time the spy and not the trojan.
 *
 * Note that you may need to serialize execution wherever
 * appropriate.
 */
char spy()
{
    int i, max_set;
    uint64_t *eviction_set_addr;
    int start, end;
    start = 0;
    end = 0;
    max_set = 0;
    int time_elapsed = 0;
    // Probe the cache line by line and take measurements
    for (i = 0; i < 64; i++)
    {
        CPUID();
        RDTSC(start);
        eviction_set_addr = get_eviction_set_address(spy_array, i, 0);
        // uint64_t base = (uint64_t)eviction_set_addr;
        // printf("%d", (int)((uint64_t)base));
        // printf("\n");
        // for (int j = 1; j < ASSOCIATIVITY; j++)
        // {
        //     // printf("%d", (int)((uint64_t)eviction_set_addr - base));
        //     // printf("\n");
        //     eviction_set_addr = (uint64_t *)*eviction_set_addr;
        // }
        while(eviction_set_addr = (uint64_t *)*eviction_set_addr){

        }
        CPUID();
        RDTSC(end);

        if (end - start > time_elapsed)
        {
            max_set = i;
            time_elapsed = end - start;
        }
        // printf("set ");
        // printf("%d", i);
        // printf(" end-start: ");
        // printf("%d", end - start);
        // printf("\n");
        // printf("max_set: ");
        // printf("%d", max_set);
        // printf("\n");
    }
    //printf("%d", max_set);
    eviction_counts[max_set]++; // records set with maximum access time.
}

int main()
{
    FILE *in, *out;
    in = fopen("transmitted-secret.txt", "r");
    out = fopen("received-secret.txt", "w");
    int j, k;
    int max_count = 0;
    int max_set = 0;

    // TODO: CONFIGURE THIS -- currently, 32*assoc to force eviction out of L2
    // configuring sample size and associativity will help decrease time

    setup(trojan_array, ASSOCIATIVITY * 32);

    setup(spy_array, ASSOCIATIVITY);

    for (;;)
    {
        max_count = 0;
        max_set = 0;
        char msg = fgetc(in);
        if (msg == EOF)
        {
            break;
        }
        for (k = 0; k < SAMPLES; k++)
        {
            trojan(msg);
            spy();
        }
        // for (int i = 0; i < 64; i++)
        // {
        //     printf("%d", i);
        //     printf(" ");
        //     printf("%d", (int)eviction_counts[i]);
        //     printf("\n");
        // }
        //for (j = 0; j < L1_NUM_SETS; j++)
        for (j = 0; j < 64; j++)
        {
            if (eviction_counts[j] > max_count)
            {
                max_count = eviction_counts[j];
                max_set = j;
            }
            eviction_counts[j] = 0;
        }
        // printf("%d", max_set);

        if (max_set >= 33 && max_set <= 59)
        {
            max_set += 32;
            // printf("%d",max_set);
            // printf("\n");
            // printf("\n");
        }
        else if (max_set == 63)
        {
            max_set = -22;
        }
        // printf("test ");
        // printf("%d",max_set);
        // printf("\n");
        fprintf(out, "%c", 32 + max_set);
        max_count = max_set = 0;
    }

    fclose(in);
    fclose(out);
}
