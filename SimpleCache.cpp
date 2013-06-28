BlSim::CacheSet::CacheSet(uint32_t way_count, uint32_t block_size):
	m_way_count(way_count) {
	CacheBlock* p_block = NULL;
	CacheBlock* p_next_block = NULL;
	p_block = new CacheBlock(block_size);
	m_p_mru_block = p_block;

	for (int i = 1; i < way_count; i++) {
		p_next_block = new CacheBlock(block_size);
		p_block->m_next_lru = p_next_block;
		p_next_block->m_prev_lru = p_block;
		p_block = p_next_block;		
	}
	m_p_lru_block = p_next_block;
}

/*
 * 根据tag找到block
 */
bool BlSim::CacheSet::FindToDo(uint64_t tag) {
	CacheBlock *p_block = m_p_mru_block;
	while (p_block) {
		if (tag == p_block->m_block_tag) {
		    // 如果不是mru块
            if (p_block->m_prev) {
                p_block->m_prev->m_next = p_block->m_next;
                if (p_block->m_next) {
                    p_block->m_next->m_prev = p_block->m_prev;
                } else {
                    // 是lru块，需要更新lru块为prev
                    m_p_lru_block = p_block->m_prev;
                }
                // 更新mru块，设当前块为mru块
                p_block->m_next = m_p_mru_block;
                m_p_mru_block->m_prev = p_block;
                p_block->m_prev = NULL;
                m_p_mru_block = p_block;
                return true;
            }

		}
		p_block = p_block->m_next_lru;
	}
	return false;
}


/*
 * 先evict的lru块，再载入新的块，并设置为mru块
 */
void BlSim::CacheSet::LoadNewBlock(const CacheAddress& cache_addr) {
    // evict lru块
    CacheBlock* p_block = m_p_lru_block;
    m_p_lru_block = m_p_lru_block->m_prev;
    m_p_lru_block->m_next = NULL;
    // 把lru块写回到内存
    p_block->WriteBack();
    p_block->m_block_addr = cache_addr.addr;
    p_block->m_block_tag = cache_addr.tag;
    p_block->m_dirty = 0;
    // 设置当前块为mru块
    p_block->m_prev = NULL;
    p_block->m_next = m_p_mru_block;
    m_p_mru_block->m_prev = p_block;
    m_p_mru_block = p_block;
}

BlSim::Caches::Caches(unsigned int numCores) {
    m_cache_capacity = (8UL << 20) * numCores;
    m_cache_way_count = 4;
    m_block_size = 64;
    m_cache_set_capacity = m_block_size * m_cache_way_count;
    m_cache_set_count = m_cache_capacity / m_cache_set_capacity;

    m_mem_reads = 0;
    m_mem_reads_hit = 0;
    m_mem_reads_miss = 0;

    m_mem_writes = 0;
    m_mem_writes_hit = 0;
    m_mem_writes_miss = 0;

    m_block_low_bits = FloorLog2(m_block_size);
    m_set_index_bits = FloorLog2(m_cache_set_count);

    m_block_low_mask = (1UL << m_block_low_bits) - 1;
    m_set_index_mask = (1UL << m_set_index_bits) - 1;

    m_shared_LLC = 1;

    m_cache_sets = new CacheSet *[m_cache_set_count];
    for (int i = 0; i < m_cache_set_count; ++i) {
        m_cache_sets = new CacheSet(m_cache_way_count, m_block_size);
    }
}

/*
 * 计算组索引和tag
 * 先右移块位，得到块地址，再取模组数，获取组索引
 * 再右移组位，得到tag
 */
BlSim::CacheAddress BlSim::Caches::GetCacheAddress(uint64_t maddr) {
	uint64_t tmp = maddr;
	tmp >>= m_block_low_bits;
	uint32_t index = tmp & m_set_index_mask;

	tmp >>= m_set_index_bits;
	uint64_t tag = tmp;
	CacheAddress addr(maddr, index, tag);
	return addr;
}

bool BlSim::Caches::Access(uint64_t maddr, uint32_t memop) {
    m_total_count++;
    CacheAddress cache_addr = GetCacheAddress(maddr);
    bool is_hit = m_cache_sets[cache_addr.index]->FindBlockByTag(cache_addr.tag);
    if (is_hit) {
        cout << "hit" << endl;
        m_hit_count++;
    } else {
        m_cache_sets[cache_addr.index]->LoadNewBlock(cache_addr);
    }
    m_cache_sets[cache_addr.index]->GetMruBlock()->Access(memop);
    return is_hit;
}
