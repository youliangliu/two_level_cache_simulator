//========================================================//
//  cache.c                                               //
//  Source file for the Cache Simulator                   //
//                                                        //
//  Implement the I-cache, D-Cache and L2-cache as        //
//  described in the README                               //
//========================================================//

#include "cache.h"
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
//
// TODO:Student Information
//
const char *studentName = "Youliang Liu";
const char *studentID   = "A14003277";
const char *email       = "yol117@ucsd.edu";

//------------------------------------//
//        Cache Configuration         //
//------------------------------------//

uint32_t icacheSets;     // Number of sets in the I$
uint32_t icacheAssoc;    // Associativity of the I$
uint32_t icacheHitTime;  // Hit Time of the I$

uint32_t dcacheSets;     // Number of sets in the D$
uint32_t dcacheAssoc;    // Associativity of the D$
uint32_t dcacheHitTime;  // Hit Time of the D$

uint32_t l2cacheSets;    // Number of sets in the L2$
uint32_t l2cacheAssoc;   // Associativity of the L2$
uint32_t l2cacheHitTime; // Hit Time of the L2$
uint32_t inclusive;      // Indicates if the L2 is inclusive

uint32_t blocksize;      // Block/Line size
uint32_t memspeed;       // Latency of Main Memory

//------------------------------------//
//          Cache Statistics          //
//------------------------------------//

uint64_t icacheRefs;       // I$ references
uint64_t icacheMisses;     // I$ misses
uint64_t icachePenalties;  // I$ penalties

uint64_t dcacheRefs;       // D$ references
uint64_t dcacheMisses;     // D$ misses
uint64_t dcachePenalties;  // D$ penalties

uint64_t l2cacheRefs;      // L2$ references
uint64_t l2cacheMisses;    // L2$ misses
uint64_t l2cachePenalties; // L2$ penalties

//------------------------------------//
//        Cache Data Structures       //
//------------------------------------//

//
//TODO: Add your Cache data structures here
//

uint64_t icacheSize;
uint64_t dcacheSize;
uint64_t l2cacheSize;
uint64_t trackcacheSize;

uint64_t iLRUSize;
uint64_t dLRUSize;
uint64_t l2LRUSize;

uint64_t *icache;
uint64_t *dcache;
uint64_t *l2cache;
uint64_t *trackcache;

uint64_t *testcache;
uint64_t testcacheSize;


//------------------------------------//
//          Cache Functions           //
//------------------------------------//

uint64_t bits_needed(uint64_t num){
    uint64_t result = 0;
    while(num!= 0){
        num/=2;
        result++;
    }
    return result;
}

uint64_t get_index(uint64_t addr, uint64_t cacheSets, uint64_t assoc ,uint64_t blocksize){
    //printf("Addr now is: %x\n", addr);
    uint64_t blockOffsetBits = bits_needed(blocksize - 1);
    //printf("blockoffsetBits is: %d\n", blockOffsetBits);
    addr = addr >> blockOffsetBits;
    //printf("Addr now is: %x\n", addr);
    uint64_t indexBits = bits_needed(cacheSets - 1);
    //printf("index bits is: %d\n", indexBits);
    uint64_t temp = (1 << indexBits) - 1;
    addr &= temp;
    return addr;
}

uint64_t get_tag_from_addr(uint64_t addr, uint64_t cacheSets, uint64_t cacheAssoc, uint64_t blocksize){
    //printf("bits needed for addr: %d\n", bits_needed(addr));
    uint64_t blockOffset = bits_needed(blocksize - 1);
    //printf("blookBits is: %d\n", blockOffset);
    uint64_t indexBits = bits_needed(cacheSets- 1);
    //printf("indexBits is: %d\n", indexBits);
    addr = addr >> blockOffset;
    addr = addr >> indexBits;
    //uint32_t temp = (1 << tagBits) - 1;
    //addr &= temp;
    return addr;
}

uint64_t get_addr_from_cacheLine(uint64_t cacheData, uint64_t cacheAssoc){
    uint64_t LRUBits = bits_needed(cacheAssoc);
    return cacheData >> LRUBits;
}

uint64_t get_tag_from_cacheLine(uint64_t cacheData, uint64_t cacheSets, uint64_t cacheAssoc, uint64_t blocksize){
    //printf("Old cache value is: %x\n", cacheData);
    uint64_t LRUbits = bits_needed(cacheAssoc);
    uint64_t addr = cacheData >> LRUbits;
    uint64_t tag = get_tag_from_addr(addr, cacheSets, cacheAssoc, blocksize);
    //printf("Tag value is: %x\n", tag);
    return tag;
}


uint64_t get_LRU_from_cacheLine(uint64_t cacheData, uint64_t cacheAssoc){
    uint64_t bitsNeeded = (1 << bits_needed(cacheAssoc)) - 1;
    return cacheData & bitsNeeded;
}


uint64_t LRU_modify(uint64_t oldCacheData, uint64_t LRUValue, uint64_t cacheAssoc){
    if(LRUValue > cacheAssoc){
        printf("Warning: LRU Value too big to be set\n");
    }
    //printf("Old cache value is: %x\n", oldCacheData);
    uint64_t addr = get_addr_from_cacheLine(oldCacheData, cacheAssoc);
    //printf("Tag value is: %x\n", tagValue);
    uint64_t LRUBits = bits_needed(cacheAssoc);
    //printf("LRUBits is: %x\n", LRUBits);
    uint64_t newCacheData= addr << LRUBits;
    newCacheData |= LRUValue;
    //printf("Tne new cache value is: %x\n", tagValue);
    return newCacheData;
}

uint64_t form_cacheLine_data(uint64_t addr, uint64_t LRUValue, uint64_t cacheAssoc){
    uint64_t bitsNeeded = bits_needed(cacheAssoc);
    uint64_t newCacheLine = addr;
    newCacheLine = newCacheLine << bitsNeeded;
    newCacheLine = newCacheLine | LRUValue;
    return newCacheLine;
}

/*
void set_new_LRU_values_hit(uint64_t* cache, uint32_t startingIndex, uint32_t cacheAssoc, uint32_t hitValue, uint32_t hitIndex){
    for(int i = 0; i < cacheAssoc; i++){
        uint32_t index = startingIndex + i;
        uint64_t cacheData = get_LRU_from_cacheLine(cache[index], cacheAssoc);
        if(get_LRU_from_cacheLine(cache[index], cacheAssoc) != 0 && get_LRU_from_cacheLine(cache[index], cacheAssoc) < hitValue && index != hitIndex){
            uint32_t oldLRU = get_LRU_from_cacheLine(cache[index], cacheAssoc);
            uint32_t newLRU = oldLRU + 1;
            cache[index] = form_cacheLine_data(cache[index], newLRU, cacheAssoc);
        }
    }
}
*/

uint64_t find_LRU_value_index(uint64_t* cache, uint64_t startingIndex, uint64_t cacheAssoc, uint64_t LRUtoFind){
    uint64_t result = -1;
    for(int i = 0; i < cacheAssoc; i++){
        uint64_t index = startingIndex + i;
        if(get_LRU_from_cacheLine(cache[index], cacheAssoc) == LRUtoFind){
            result = index ;
            break;
        }
    }
    return result;
}

void set_new_LRU_value(uint64_t* cache, uint64_t startingIndex, uint64_t cacheAssoc){
    for(uint64_t i = cacheAssoc; i > 1; i--){
        uint64_t big_index = find_LRU_value_index(cache, startingIndex, cacheAssoc, i);
        uint64_t small_index = find_LRU_value_index(cache, startingIndex, cacheAssoc, i-1);
        if (big_index == -1 && small_index != -1){
            cache[small_index] = LRU_modify(cache[small_index], i, cacheAssoc);
            //printf("new cacche[small_index] = %x\n", cache[small_index]);
        }
    }
}

uint64_t find_max_LRU_index(uint64_t* cache, uint64_t startingIndex, uint64_t cacheAssoc){
    uint64_t maxLRU = 0;
    uint64_t maxIndex = startingIndex;
    for(int i=0; i<cacheAssoc; i++){
        uint64_t index = startingIndex + i;
        uint64_t currLRU = get_LRU_from_cacheLine(cache[index], cacheAssoc);
        if(currLRU > maxLRU){
            maxLRU = currLRU;
            maxIndex = index;
        }
    }
    return maxIndex;
}


void evict_l1_cache(uint64_t* cache, uint64_t cacheSets, uint64_t cacheAssoc, uint64_t blocksize, uint64_t addr){
    //Get the set index
    uint64_t setIndex = get_index(addr, cacheSets, cacheAssoc, blocksize);

    //Check the whole set to see if there is a hit.
    uint64_t setStartIndex = setIndex * cacheAssoc;
    uint64_t addrTag = get_tag_from_addr(addr, cacheSets, cacheAssoc, blocksize);
    for(int i = 0; i < cacheAssoc; i++){
        uint64_t index = setStartIndex + i;
        if (cache[index] != 0){
            uint64_t cacheLineTag = get_tag_from_cacheLine(cache[index], cacheSets, cacheAssoc, blocksize);
            if(addrTag == cacheLineTag){
                icache[index] = 0;
            }
        }
    }
}


// Initialize the Cache Hierarchy
//
void
init_cache()
{
    // Initialize cache stats
    icacheRefs        = 0;
    icacheMisses      = 0;
    icachePenalties   = 0;
    dcacheRefs        = 0;
    dcacheMisses      = 0;
    dcachePenalties   = 0;
    l2cacheRefs       = 0;
    l2cacheMisses     = 0;
    l2cachePenalties  = 0;

    //
    //TODO: Initialize Cache Simulator Data Structures
    //

    iLRUSize = bits_needed(icacheAssoc);
    dLRUSize = bits_needed(dcacheAssoc);
    l2LRUSize = bits_needed(l2cacheAssoc);

    icacheSize = (icacheSets * icacheAssoc) * sizeof(uint64_t);
    dcacheSize = (dcacheSets * dcacheAssoc) * sizeof(uint64_t);
    l2cacheSize = (l2cacheSets * l2cacheAssoc) * sizeof(uint64_t);
    trackcacheSize = l2cacheSize;

    icache = malloc(icacheSize);
    dcache = malloc(dcacheSize);
    l2cache = malloc(l2cacheSize);
    trackcache = malloc(trackcacheSize);

    memset(icache, 0, icacheSize);
    memset(dcache, 0, dcacheSize);
    memset(l2cache, 0, l2cacheSize);
    memset(trackcache, 0, trackcacheSize);

    testcacheSize = 8 * sizeof(uint32_t);
    testcache = malloc(testcacheSize);
    memset(testcache, 0, testcacheSize);

    //testcache[0] = 0xFF1;
    //testcache[1] = 0xFF4;
    //testcache[2] = 0xFF2;
    //testcache[3] = 0xFF3;
    //set_new_LRU_values(testcache, 0, 4, 3, 2);
    //testcache[2] = LRU_modify(testcache[2], 0, 4);
    //printf("After modification: %d\n", bits_needed(15));
    //set_new_LRU_value(testcache, 0, 4);
    //testcache[2] = LRU_modify(testcache[2], 1, 4);
    /*
    printf("The LRU index is: %d\n", find_LRU_value_index(testcache, 0, 15, 1));
    printf("The LRU index is: %d\n", find_LRU_value_index(testcache, 0, 15, 2));
    printf("The LRU index is: %d\n", find_LRU_value_index(testcache, 0, 15, 0));
    printf("The LRU index is: %d\n", find_LRU_value_index(testcache, 0, 15, 4));
     */
    //printf("LRU value is: %x\n", get_LRU_from_cacheLine(testcache[3], 15));
    //printf("The new cache value is: %x\n", LRU_modify(testcache[0], 8, 1, 15, 0));


/*
  printf("%x\n", testcache[0]);
  printf("%x\n", testcache[1]);
  printf("%x\n", testcache[2]);
  printf("%x\n", testcache[3]);
  //printf("The index value is: %x\n", get_tag_from_addr(0b1101, 4, 2, 0));
  //print_binary(0xF0);
    printf("The index is: %x\n", get_index(0xF0, 15, 15, 0));
    printf("The tag is: %x\n", get_tag_from_addr(0xF0, 15, 15, 0));
    testcache_access(0xF0, testcache, 4, 4, 0);
    testcache_access(0xF0, testcache, 4, 4, 0);
    testcache_access(0x80, testcache, 4, 4, 0);
    testcache_access(0x80, testcache, 4, 4, 0);
    //testcache_access(0xF0, testcache, 4, 4, 0);
    testcache_access(0x70, testcache, 4, 4, 0);
    testcache_access(0x60, testcache, 4, 4, 0);
    testcache_access(0xF0, testcache, 4, 4, 0);
    testcache_access(0x60, testcache, 4, 4, 0);
    testcache_access(0x50, testcache, 4, 4, 0);
    testcache_access(0x50, testcache, 4, 4, 0);
    //testcache_access(0b00100, testcache, 15, 2, 0);
    //testcache_access(0b01000, testcache, 15, 2, 0);
    //testcache_access(0b01001, testcache, 15, 2, 0);
    //testcache_access(0b00000, testcache, 15, 2, 0);
    //testcache_access(0b01000, testcache, 15, 2, 0);
    printf("%x\n", testcache[0]);
    printf("%x\n", testcache[1]);
    printf("%x\n", testcache[2]);
    printf("%x\n", testcache[3]);


    printf("Get tag from cachedata: %x\n", get_tag_from_cacheLine(0x381, 4, 4, 0));
    printf("Get tag from addr: %x\n", get_tag_from_addr(0x70, 4, 4, 0));
*/
    //printf("Get tag from addr: %x\n", get_tag_from_addr(0x70, 256, 256, 0));
    //printf("The index is: %x\n", get_index(0xF0, 256, 256, 256));
    //printf("The cache line is: %x\n", 0x7ffeffe4);
    //printf("The new cache line is: %llx\n", (long long)form_cacheLine_data(0x7ffeffe4, 7, 15));
    //printf("bits needed: %d\n", bits_needed(0b11));
    //printf("new data formed is: %x\n", form_cacheLine_data(0xF, 2, 15));
    //printf("Max Index is: %d\n", find_max_LRU_index(testcache, 0, 4));
    //printf("bits need for 16: %d\n", bits_needed(16));
    //printf("Test get LRU data: %x\n", get_LRU_from_cacheLine(0b00101110, 4));
    //printf("the value is: %x\n", get_index(0xFFFF86FF, 4, 4, 256));
    //LRU_modify(0xFFF8, 16, 16);
    //get_tag_from_cacheLine(0xFFF8, 16);

}






// Perform a memory access through the icache interface for the address 'addr'
// Return the access time for the memory operation
//
uint32_t icache_access(uint32_t addr){
    addr = (uint64_t)addr;
    icacheSets = (uint64_t)icacheSets;
    icacheAssoc = (uint64_t)icacheAssoc;
    blocksize = (uint64_t)blocksize;

    icacheRefs++;
    if(icacheSets == 0) {
        return l2cache_access(addr, 1);
    }

    //Get the set index
    uint64_t setIndex = get_index(addr, icacheSets, icacheAssoc, blocksize);

    //Check the whole set to see if there is a hit.
    uint64_t setStartIndex = setIndex * icacheAssoc;
    uint64_t addrTag = get_tag_from_addr(addr, icacheSets, icacheAssoc, blocksize);
    bool slotFlag = false;
    uint64_t openSlotIndex;
    for(int i = 0; i < icacheAssoc; i++){
        uint64_t index = setStartIndex + i;
        if (icache[index] != 0){
            uint64_t cacheLineTag = get_tag_from_cacheLine(icache[index], icacheSets, icacheAssoc, blocksize);
            if(addrTag == cacheLineTag){
                //The data is found, a hit took place
                // Set the new LRU here
                //uint32_t LRUHitValue = get_LRU_from_cacheLine(icache[index], icacheAssoc);
                icache[index] = LRU_modify(icache[index], 0, icacheAssoc);
                set_new_LRU_value(icache, setStartIndex, icacheAssoc);
                icache[index] = LRU_modify(icache[index], 1, icacheAssoc);
                return icacheHitTime;
            }
        }
        else{
            slotFlag = true;
            openSlotIndex = index;
        }
    }

    //No hit was found.
    icacheMisses++;

    //Handle Compulsory miss
    if(slotFlag){
        set_new_LRU_value(icache, setStartIndex,icacheAssoc);
        icache[openSlotIndex] = form_cacheLine_data(addr, 1, icacheAssoc);
        uint32_t penalty = l2cache_access(addr, 1);
        icachePenalties += penalty;
        return penalty + icacheHitTime;
    }
        //Handle capacity miss
    else{
        uint64_t maxIndex = find_max_LRU_index(icache, setStartIndex, icacheAssoc);
        icache[maxIndex] = 0;
        set_new_LRU_value(icache, setStartIndex, icacheAssoc);
        icache[maxIndex] = form_cacheLine_data(addr, 1, icacheAssoc);
        uint32_t penalty = l2cache_access(addr, 1);
        icachePenalties += penalty;
        return penalty + icacheHitTime;
    }


    return memspeed;
}

// Perform a memory access through the dcache interface for the address 'addr'
// Return the access time for the memory operation
//
uint32_t
dcache_access(uint32_t addr)
{
    //
    //TODO: Implement D$
    //
    addr = (uint64_t)addr;
    dcacheSets = (uint64_t)dcacheSets;
    dcacheAssoc = (uint64_t)dcacheAssoc;
    blocksize = (uint64_t)blocksize;

    dcacheRefs++;
    if(dcacheSets == 0) {
        return l2cache_access(addr, 2);
    }

    //Get the set index
    uint64_t setIndex = get_index(addr, dcacheSets, dcacheAssoc, blocksize);

    //Check the whole set to see if there is a hit.
    uint64_t setStartIndex = setIndex * dcacheAssoc;
    uint64_t addrTag = get_tag_from_addr(addr, dcacheSets, dcacheAssoc, blocksize);
    bool slotFlag = false;
    uint64_t openSlotIndex;
    for(int i = 0; i < dcacheAssoc; i++){
        uint64_t index = setStartIndex + i;
        if (dcache[index] != 0){
            uint64_t cacheLineTag = get_tag_from_cacheLine(dcache[index], dcacheSets, dcacheAssoc, blocksize);
            if(addrTag == cacheLineTag){
                //The data is found, a hit took place
                // Set the new LRU here
                //uint32_t LRUHitValue = get_LRU_from_cacheLine(icache[index], icacheAssoc);
                dcache[index] = LRU_modify(dcache[index], 0, dcacheAssoc);
                set_new_LRU_value(dcache, setStartIndex, dcacheAssoc);
                dcache[index] = LRU_modify(dcache[index], 1, dcacheAssoc);
                return dcacheHitTime;
            }
        }
        else{
            slotFlag = true;
            openSlotIndex = index;
        }
    }

    //No hit was found.
    dcacheMisses++;

    //Handle Compulsory miss
    if(slotFlag){
        set_new_LRU_value(dcache, setStartIndex,dcacheAssoc);
        dcache[openSlotIndex] = form_cacheLine_data(addr, 1, dcacheAssoc);
        uint32_t penalty = l2cache_access(addr, 2);
        dcachePenalties += penalty;
        return penalty + dcacheHitTime;
    }
        //Handle capacity miss
    else{
        uint64_t maxIndex = find_max_LRU_index(dcache, setStartIndex, dcacheAssoc);
        dcache[maxIndex] = 0;
        set_new_LRU_value(dcache, setStartIndex, dcacheAssoc);
        dcache[maxIndex] = form_cacheLine_data(addr, 1, dcacheAssoc);
        uint32_t penalty = l2cache_access(addr, 2);
        dcachePenalties += penalty;
        return penalty + dcacheHitTime;
    }


    return memspeed;
}

// Perform a memory access to the l2cache for the address 'addr'
// Return the access time for the memory operation
//
uint32_t
l2cache_access(uint32_t addr, uint64_t info)
{
    addr = (uint64_t)addr;
    l2cacheSets = (uint64_t)l2cacheSets;
    l2cacheAssoc = (uint64_t)l2cacheAssoc;
    blocksize = (uint64_t)blocksize;

    l2cacheRefs++;
    if(l2cacheSets == 0) {
        return memspeed;
    }

    //Get the set index
    uint64_t setIndex = get_index(addr, l2cacheSets, l2cacheAssoc, blocksize);

    //Check the whole set to see if there is a hit.
    uint64_t setStartIndex = setIndex * l2cacheAssoc;
    uint64_t addrTag = get_tag_from_addr(addr, l2cacheSets, l2cacheAssoc, blocksize);
    bool slotFlag = false;
    uint64_t openSlotIndex;
    for(int i = 0; i < l2cacheAssoc; i++){
        uint64_t index = setStartIndex + i;
        if (l2cache[index] != 0){
            uint64_t cacheLineTag = get_tag_from_cacheLine(l2cache[index], l2cacheSets, l2cacheAssoc, blocksize);
            if(addrTag == cacheLineTag){
                //The data is found, a hit took place
                // Set the new LRU here
                //uint32_t LRUHitValue = get_LRU_from_cacheLine(icache[index], icacheAssoc);
                l2cache[index] = LRU_modify(l2cache[index], 0, l2cacheAssoc);
                set_new_LRU_value(l2cache, setStartIndex, l2cacheAssoc);
                l2cache[index] = LRU_modify(l2cache[index], 1, l2cacheAssoc);
                trackcache[index] = info;
                return l2cacheHitTime;
            }
        }
        else{
            slotFlag = true;
            openSlotIndex = index;
        }
    }

    //No hit was found.
    l2cacheMisses++;
    //Handle Compulsory miss
    if(slotFlag){
        set_new_LRU_value(l2cache, setStartIndex, l2cacheAssoc);
        l2cache[openSlotIndex] = form_cacheLine_data(addr, 1, l2cacheAssoc);
        trackcache[openSlotIndex] = info;
        l2cachePenalties += memspeed;
        return memspeed + l2cacheHitTime;
    }
        //Handle capacity miss
    else{
        uint64_t maxIndex = find_max_LRU_index(l2cache, setStartIndex, l2cacheAssoc);
        uint64_t l1_addr = get_addr_from_cacheLine(l2cache[maxIndex], l2cacheAssoc);
        if(inclusive == 1){
            if(trackcache[maxIndex] == 1){
                icacheSets = (uint64_t)icacheSets;
                icacheAssoc = (uint64_t)icacheAssoc;
                evict_l1_cache(icache, icacheSets, icacheAssoc, blocksize, addr);
            }
            if(trackcache[maxIndex] == 2){
                dcacheSets = (uint64_t)dcacheSets;
                dcacheAssoc = (uint64_t)dcacheAssoc;
                evict_l1_cache(dcache, dcacheSets, dcacheAssoc, blocksize, addr);
            }
        }
        l2cache[maxIndex] = 0;
        set_new_LRU_value(l2cache, setStartIndex, l2cacheAssoc);
        l2cache[maxIndex] = form_cacheLine_data(addr, 1, l2cacheAssoc);
        trackcache[maxIndex] = info;
        l2cachePenalties += memspeed;
        return memspeed + l2cacheHitTime;
    }


    return memspeed;
}

