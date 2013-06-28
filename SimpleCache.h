#ifndef CACHE_SIMULATOR_H_
#define CACHE_SIMULATOR_H_

#include <stdint.h>

#define DEBUG_CACHE_SIMULATOR

#define INVALID_BLOCK (~(0UL))

#define INVALID_SUB_BLOCK_DIS 65

namespace BlSim {

    /*!
     *  @brief Computes floor(log2(n))
     *  Works by finding position of MSB set.
     *  @returns -1 if n == 0.
     */
    static inline uint32_t FloorLog2(uint32_t n)
    {
        uint32_t p = 0;

        if (n == 0) return 0;

        if (n & 0xffff0000) { p += 16; n >>= 16; }
        if (n & 0x0000ff00) { p +=  8; n >>=  8; }
        if (n & 0x000000f0) { p +=  4; n >>=  4; }
        if (n & 0x0000000c) { p +=  2; n >>=  2; }
        if (n & 0x00000002) { p +=  1; }

        return p;
    }
    enum Type {
        MEM_WRITE = 0,
        MEM_READ
    };
    /*

       class Transaction {
       public:
       Transaction(uint64_t addr, Type type) : m_addr(addr), m_type(type) {}
       uint64_t GetAddress() const {
       return m_addr; 
       }

       int GetType() const {
       return m_type;
       }
       private:
       uint64_t m_addr;
       Type m_type;
       };*/

    class CacheBlock {
        public:
            uint32_t m_block_size; //it is usual 64B
            uint64_t m_block_addr; //it is full addr, do not filter for it
            uint64_t m_block_tag;  //filter the inner-set addr and the set index
            uint32_t m_dirty; //to mark if this block is written
            CacheBlock* m_next;
            CacheBlock* m_prev;

        public:
            CacheBlock(uint32_t block_size) : m_block_size(block_size),
                                              m_block_addr(INVALID_BLOCK),
                                              m_block_tag(0),
                                              m_next(NULL),
                                              m_prev(NULL),
                                              m_dirty(0) { }

            void Replaced(uint32_t block_addr);  //Note: this only for lru cache block

            void hit_access(uint32_t mem_rw);
            void print_cache_block();
            void reset_block_access_distribution();
            void get_data_from_evicted(CacheBlock *p_evicted_block);

            void push_data_into_upper_block(CacheBlock *p_upper_block);


            uint32_t get_sub_block_distribution();

            int is_invalid_cache(){return m_block_addr == INVALID_BLOCK;}
    };

    class CacheSet {
        int write_back_mem_trace;

        protected:
        uint32_t m_way_count;  //the cache associaticity
        CacheBlock *m_p_mru_block;
        CacheBlock *m_p_lru_block;

        public:
        CacheSet(uint32_t way_count, uint32_t block_size);
        ~CacheSet();

        CacheBlock* FindBlockByTag(uint64_t mem_tag);
        void hit_access(CacheBlock **p_block);

        CacheBlock* evict_lru_block();
        void write_back_evicted_lru_block(CacheBlock *evicted_lru_block);
        void put_accessed_block_in_mru(CacheBlock *p_new_block);

        uint32_t load_new_block_in_LLC(uint64_t maddr, uint64_t mem_tag);
        CacheBlock *get_mru_block(){return m_p_mru_block;}
        CacheBlock *get_lru_block(){return m_p_lru_block;}

        void print_cache_set();
    };

    struct CacheAddress {
        uint64_t addr;
        uint32_t index;
        uint64_t tag;
        CacheAddress(uint64_t maddr, uint32_t set_index, uint64_t mtag) :
            addr(maddr), index(set_index), tag(mtag) {}
    }

    class Caches
    {
        protected:
            uint64_t m_cache_capacity;
            uint32_t m_cache_way_count;
            uint32_t m_block_size;

            uint32_t m_cache_set_capacity; //the size of each set
            uint32_t m_cache_set_count; //the count of cache set at each level cache

            uint32_t m_block_low_bits;  //the real low addr, 
            uint32_t m_set_index_bits;  //the cache block bits

            uint32_t m_block_low_mask;
            uint32_t m_set_index_mask;

            //some cache access statistics
            uint64_t m_mem_reads;
            uint64_t m_mem_reads_hit;
            uint64_t m_mem_reads_miss;

            uint64_t m_mem_writes;
            uint64_t m_mem_writes_hit;
            uint64_t m_mem_writes_miss;
            uint64_t m_hit_count;
            uint64_t m_miss_count;
            uint64_t m_total_count;
            uint32_t write_back_mem_trace ;


            int m_shared_LLC;  //whether the last level of cache is shared among cores, 1 for yes

            CacheSet **m_cache_sets;

            CacheAddress GetCacheAddress(uint64_t maddr);

            CacheSet* access_cache_at_level(uint64_t maddr,
                    uint32_t level,
                    uint64_t *mtag,
                    bool* hit);

            void cache_replaced(CacheSet *p_lower_set, CacheSet *p_upper_set,
                    uint32_t lower_level, uint64_t upper_mtag,
                    uint64_t maddr);

        public:
            Caches(unsigned int numCores);
            ~Caches();

            bool Access(uint64_t maddr, uint32_t mem_rw);

            void print_cache_config();
            void output_mem_reqs_statistics();
            void dump_statistic();
            bool writebackornot();
    };

}
#endif
