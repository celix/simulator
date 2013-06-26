BlSim::Caches::Caches(unsigned int numCores) {
		uint32_t i;
		uint32_t j;
		//we use the default cache config of core i7
		m_level = 1;
		if(m_level > MAX_CACHE_LEVEL)
		{
			cerr<<"Invalid cache level:"<<m_level<<", max cache level is "<<MAX_CACHE_LEVEL<<endl;
			exit(-8);	
		}		


		m_cache_capacity[0] = (32UL << 20) * numCores;  //L1 cache 32KB
		m_cache_capacity[1] = (256UL << 10) * numCores; //L2 cahce 256KB
		m_cache_capacity[2] = (1UL << 20) * numCores;   //L3 cache 8MB, shared
		if(numCores < 4)
		{
			m_cache_capacity[2] = (4UL << 20);  //we use the 4MB for less cores
		}

		m_cache_way_count[0] = 4;  //L1 cache 4-way (Associativity)
		m_cache_way_count[1] = 8;
		m_cache_way_count[2] = 16;

		m_block_size[0] = 64;
		m_block_size[1] = 64;
		m_block_size[2] = 64;



		for(i = 0; i < m_level; i++)
		{
			m_cache_set_capacity[i] = m_block_size[i] * m_cache_way_count[i];
			m_cache_set_count[i] = m_cache_capacity[i] / m_cache_set_capacity[i];

			m_mem_reads[i] = 0;
			m_mem_reads_hit[i] = 0;
			m_mem_reads_miss[i] = 0;

			m_mem_writes[i] = 0;
			m_mem_writes_hit[i] = 0;
			m_mem_writes_miss[i] = 0;
		}

		for(i = 0; i < m_level; i++)
		{
			m_block_low_bits[i] = FloorLog2(m_block_size[i]);
			m_set_index_bits[i] = FloorLog2(m_cache_set_count[i]);
	
			m_block_low_mask[i] = (1UL << m_block_low_bits[i]) - 1;
			m_set_index_mask[i] = (1UL << m_set_index_bits[i]) - 1;
		}


		m_shared_LLC = 1;
		
		m_cache_config_fname = NULL;

		//alloc memoryu for real cache sets
		for(i = 0; i < MAX_CACHE_LEVEL; i++)
		{
			m_cache_sets[i] = NULL;
		}

		for(i = 0; i < m_level; i++)
		{
			m_cache_sets[i] = new CacheSet *[m_cache_set_count[i]];
			for(j = 0; j < m_cache_set_count[i]; j++)
			{
				m_cache_sets[i][j] = new CacheSet(m_cache_way_count[i], m_block_size[i]);
			}
		}
#ifdef DEBUG_CACHE_SIMULATOR
	//print_cache_config();
#endif
}
