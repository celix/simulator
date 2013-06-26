#ifndef CACHE_SIMULATOR_H_
#define CACHE_SIMULATOR_H_

#define DEBUG_CACHE_SIMULATOR

#define INVALID_BLOCK (~(0UL))

#define INVALID_SUB_BLOCK_DIS 65

namespace BlSim
{
    typedef unsigned int uint32_t;
    typedef unsigned long uint64_t;

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

    class CacheBlock
    {
        public:
            uint32_t m_block_size; //it is usual 64B
            uint64_t m_block_addr; //it is full addr, do not filter for it
            uint64_t m_block_tag;  //filter the inner-set addr and the set index
            uint32_t m_block_in_upper_cache;




            uint32_t m_dirty; //to mark if this block is written
        public:
            class CacheBlock* m_next_lru;
            class CacheBlock* m_prev_lru;

            //Add this to quick find its parent block in the lower level cache, Inclusive
            class CacheBlock *m_parent_block_in_lower;

        public:
            CacheBlock(uint32_t block_size);

            void Replaced(uint32_t block_addr);  //Note: this only for lru cache block

            void hit_access(uint32_t mem_rw);
            void print_cache_block();
            void reset_block_access_distribution();
            void get_data_from_evicted(CacheBlock *p_evicted_block);

            void push_data_into_upper_block(CacheBlock *p_upper_block);


            uint32_t get_sub_block_distribution();

            int is_invalid_cache(){return m_block_addr == INVALID_BLOCK;}
    };

    class CacheSet
    {
	int write_back_mem_trace;

protected:
            uint32_t m_way_count;  //the cache associaticity
            class CacheBlock *m_p_mru_block;
            class CacheBlock *m_p_lru_block;

        public:
            CacheSet(uint32_t way_count, uint32_t block_size);
            ~CacheSet();

            CacheBlock *find_block(uint64_t mem_tag); //if not in set, return NULL		
            void hit_access(CacheBlock **p_block);

            CacheBlock* evict_lru_block();
            void write_back_evicted_lru_block(CacheBlock *evicted_lru_block);
            void put_accessed_block_in_mru(CacheBlock *p_new_block);

            uint32_t load_new_block_in_LLC(uint64_t maddr, uint64_t mem_tag);
            CacheBlock *get_mru_block(){return m_p_mru_block;}
            CacheBlock *get_lru_block(){return m_p_lru_block;}

            void print_cache_set();
    };

    class Caches
    {
        protected:
            enum Cache_Config{MAX_CACHE_LEVEL=8};

            uint32_t m_level; //3 level cache
            uint64_t m_cache_capacity[MAX_CACHE_LEVEL]; //the capacity of each level cahce
            uint32_t m_cache_way_count[MAX_CACHE_LEVEL]; //the way of each level cache
            uint32_t m_block_size[MAX_CACHE_LEVEL];

            uint32_t m_cache_set_capacity[MAX_CACHE_LEVEL]; //the size of each set
            uint32_t m_cache_set_count[MAX_CACHE_LEVEL]; //the count of cache set at each level cache

            uint32_t m_block_low_bits[MAX_CACHE_LEVEL];  //the real low addr, 
            uint32_t m_set_index_bits[MAX_CACHE_LEVEL];  //the cache block bits
            //uint32_t m_tag_bits[MAX_CACHE_LEVEL];

            uint32_t m_block_low_mask[MAX_CACHE_LEVEL];
            uint32_t m_set_index_mask[MAX_CACHE_LEVEL];
            //uint64_t m_tag_mask[MASK_CACHE_LEVEL];

            //some cache access statistics
            uint64_t m_mem_reads[MAX_CACHE_LEVEL];
            uint64_t m_mem_reads_hit[MAX_CACHE_LEVEL];
            uint64_t m_mem_reads_miss[MAX_CACHE_LEVEL];

            uint64_t m_mem_writes[MAX_CACHE_LEVEL];
            uint64_t m_mem_writes_hit[MAX_CACHE_LEVEL];
            uint64_t m_mem_writes_miss[MAX_CACHE_LEVEL];
            uint64_t m_hit_count;
			uint64_t m_miss_count;
            uint64_t m_total_count;
            uint32_t write_back_mem_trace ;


            int m_shared_LLC;  //whether the last level of cache is shared among cores, 1 for yes

            char *m_cache_config_fname;

            CacheSet **m_cache_sets[MAX_CACHE_LEVEL];

            void get_cache_addr_parts(uint64_t maddr, uint64_t *mem_tag,
                                      uint32_t *set_index, uint32_t level);

            CacheSet* access_cache_at_level(uint64_t maddr,
                                            uint32_t level,
                                            uint64_t *mtag,
                                            bool* hit);

            void cache_replaced(CacheSet *p_lower_set, CacheSet *p_upper_set,
                                uint32_t lower_level, uint64_t upper_mtag,
                                uint64_t maddr);

        public:
            Caches(char *cache_config_fname, unsigned int numCores);
            ~Caches();

            bool access_cache(uint64_t maddr, uint32_t mem_rw);

            void print_cache_config();
            void output_mem_reqs_statistics();
            void dump_statistic();
            bool writebackornot();
    };

}
#endif
