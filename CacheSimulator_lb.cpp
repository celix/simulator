#include "CacheSimulator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <iostream.h>
//#include <cstdlib.h>
#include <assert.h>
#include "Transaction.h"

using namespace std;
using namespace DRAMSim;

#define CACHE_WRITE_BACK_SIM
#define RECORD_UPPER_INS

#define max_mem_trace_gra_count (16UL<<20)

FILE * trace;
//Caches *myCaches;
//FILE *mtrace;
FILE *mtrace_gra;

uint64_t evicted_LLC_count = 0;

unsigned int non_mem_ins_count = 0;
unsigned long total_ins_count = 0;
unsigned long total_non_mem_ins_count = 0;
unsigned long total_mem_ins_count = 0;

unsigned long mem_read_requests = 0;
unsigned long mem_write_requests = 0;

void add_trace_to_memory(unsigned long maddr, unsigned int rw);

BlSim::CacheBlock::CacheBlock(uint32_t block_size)
{
    m_block_size = block_size;
    m_block_addr = INVALID_BLOCK;
    m_next_lru = NULL;
    m_prev_lru = NULL;
    m_parent_block_in_lower = NULL;
    m_block_in_upper_cache = 0;
    m_dirty = 0;
    m_block_tag =0 ;
}

void BlSim::CacheBlock::reset_block_access_distribution()
{
	m_dirty = 0;
}

void BlSim::CacheBlock::hit_access(uint32_t mem_rw)
{
	if(mem_rw == MEM_WRITE)
	{
		m_dirty = 1;
	}
}

void BlSim::CacheBlock::get_data_from_evicted(CacheBlock *p_evicted_block)
{
	//assert(m_block_addr == p_evicted_block->m_block_addr);
	assert(m_block_in_upper_cache == 1);
	//this block not in upper cache any more
	m_block_in_upper_cache = 0;
	p_evicted_block->m_parent_block_in_lower = NULL;
	m_dirty = p_evicted_block->m_dirty;
}

void BlSim::CacheBlock::push_data_into_upper_block(CacheBlock *p_upper_block)
{
        m_block_in_upper_cache = 1; //Yes, we put this block into upper cache

        //And, we just want to the access distribution info,

	p_upper_block->m_parent_block_in_lower = this;

	p_upper_block->m_dirty = m_dirty;
}

void BlSim::CacheBlock::print_cache_block()
{
	cout<<dec<<"size="<<m_block_size<<", block_addr=0x"<<hex<<m_block_addr<<dec<<endl;
}



BlSim::CacheSet::CacheSet(uint32_t way_count, uint32_t block_size):
	m_way_count(way_count)
{
	uint32_t i;
	CacheBlock *p_block = NULL;
	CacheBlock *p_next_block = NULL;
	p_block = new CacheBlock(block_size);
	m_p_mru_block = p_block;

	for(i = 1; i < way_count; i++)
	{
		p_next_block = new CacheBlock(block_size);
		p_block->m_next_lru = p_next_block;
		p_next_block->m_prev_lru = p_block;
		p_block = p_next_block;		
	}
	m_p_lru_block = p_next_block;

}

BlSim::CacheSet::~CacheSet()
{
	CacheBlock *p_block;
	CacheBlock *p_next_block;

	p_block = m_p_mru_block;

	while(p_block)
	{
		p_next_block = p_block->m_next_lru;
		delete p_block;
		p_block = p_next_block;
	}
}

void BlSim::CacheSet::hit_access(CacheBlock **pp_block)
{
    CacheBlock *p_block = *pp_block;
    assert(m_p_mru_block->m_prev_lru == NULL);
    assert(m_p_lru_block->m_next_lru == NULL);

	if(!p_block->m_prev_lru)
	{
		//it is the mru block, we do not need to move the block position
		assert(p_block->m_block_tag == m_p_mru_block->m_block_tag);
	}
	else
	{
		//get this block out of the link, and then put it in the mru position
		p_block->m_prev_lru->m_next_lru = p_block->m_next_lru;
		if(p_block->m_next_lru)
		{
			//this block is not the lru block
			p_block->m_next_lru->m_prev_lru = p_block->m_prev_lru;
		}
        	else
        	{
            		//this is the lru block
            		//Note: we need to update the lru pointer to its prev block, cause we move this block into mru
            		assert(p_block->m_block_tag == m_p_lru_block->m_block_tag);
            		m_p_lru_block = p_block->m_prev_lru;
        	}

		p_block->m_next_lru = m_p_mru_block;
		m_p_mru_block->m_prev_lru = p_block;

		p_block->m_prev_lru = NULL;  //now the p_block becomes the new mru block

		m_p_mru_block = p_block;
	}

	//p_block->hit_access(sub_block_index);
}

void BlSim::CacheSet::print_cache_set()
{
	uint32_t i;

	cout<<"Set status: way_count="<<m_way_count<<endl;
	
	CacheBlock *p_block;
        CacheBlock *p_next_block;
	p_block = m_p_mru_block;
	i = 0;
	while(p_block)
	{
		p_next_block = p_block->m_next_lru;
		cout<<"Cache Block "<<i<<": ";
		p_block->print_cache_block();
		p_block = p_next_block;
		i++;
	}
	assert(i == m_way_count);
}

BlSim::CacheBlock* BlSim::CacheSet::find_block(uint64_t mem_tag)
{
	CacheBlock *p_block;
    //cout << "input tag: " << mem_tag << endl;;
    //cout << "exist tag: \n" << m_p_mru_block->m_block_tag;
	p_block = m_p_mru_block;
	while(p_block)
	{
		if(mem_tag == p_block->m_block_tag)
		{
			//find the cache block
			return p_block;
		}
		p_block = p_block->m_next_lru;
	}
	return NULL;  //not find cache block in the set
}

BlSim::CacheBlock* BlSim::CacheSet::evict_lru_block()
{
	//Just evict the lru block, out of list
        //Add 06/12/2012: can not evict the lru block with it is in the upper cache
    
	CacheBlock *p_block = m_p_lru_block;
    	assert(p_block != NULL);
    	if(!p_block->m_block_in_upper_cache)
    	{
        	//Well, the lru block is not in the upper cache, just evict
        	//cout<<"## evicted lru block:"<<p_block->m_block_addr<<endl;
	    	assert(p_block != NULL && p_block->m_prev_lru != NULL);

	    	m_p_lru_block = p_block->m_prev_lru;
	    	m_p_lru_block->m_next_lru = NULL;
	    	p_block->m_prev_lru = NULL;
    	}
    	else
    	{
        	//we need to find the first block not in the upper cache
#ifdef DEBUG_CACHE_SIMULATOR
        	//cout<<"## In evict lru block, the lru block is in upper cache";
#endif
        	uint32_t block_num = 0;
        	while(p_block != NULL && p_block->m_block_in_upper_cache)
        	{
            		p_block = p_block->m_prev_lru;
            		block_num++;
            		//assert(p_block != NULL);
        	}
#ifdef DEBUG_CACHE_SIMULATOR
        	//cout<<":"<<block_num<<endl;
#endif
        	//Got the block, evict it
        	//Is it possible the mru block, no way
        	assert(p_block->m_block_tag != m_p_mru_block->m_block_tag);
        	p_block->m_prev_lru->m_next_lru = p_block->m_next_lru;
        	p_block->m_next_lru->m_prev_lru = p_block->m_prev_lru;
        
        	p_block->m_prev_lru = NULL;
        	p_block->m_next_lru = NULL;
    	}
	
	return p_block;
}

void BlSim::CacheSet::put_accessed_block_in_mru(CacheBlock *p_new_block)
{
	p_new_block->m_next_lru = m_p_mru_block;
	m_p_mru_block->m_prev_lru = p_new_block;
	
	p_new_block->m_prev_lru = NULL;

	m_p_mru_block = p_new_block;
	
}

uint32_t BlSim::CacheSet::load_new_block_in_LLC(uint64_t maddr, uint64_t mem_tag)
{
#ifdef DEBUG_CACHE_SIMULATOR
    	//cout<<"## Evicted lru at level LLC"<<endl; 
#endif
	uint32_t accessed_sub_block = INVALID_SUB_BLOCK_DIS;
	CacheBlock *p_evicted_block = evict_lru_block();

	//uint32_t 
	//write_back_mem_trace = 0;

	//To DO: print this evicted block as the output trace
    	evicted_LLC_count++;
	//update the num of accessed sub block
	if(!p_evicted_block->is_invalid_cache() ) 
	{

#ifdef CACHE_WRITE_BACK_SIM
		if(p_evicted_block->m_dirty)
		{
			//the cache block is dirty, write back it into the memory
			add_trace_to_memory(maddr, (unsigned int)MEM_WRITE);
			write_back_mem_trace = 1;
			//issue to the memory libing 
			
		}
		else
			{
			write_back_mem_trace=0;
			}
#endif
	}


	//construct this evicted as the new mru block
	p_evicted_block->m_block_addr = maddr;
	p_evicted_block->m_block_tag = mem_tag;
	p_evicted_block->reset_block_access_distribution();
    	p_evicted_block->m_block_in_upper_cache = 0;



	//ok then, put this new block in the mru location
	put_accessed_block_in_mru(p_evicted_block);
	
	return accessed_sub_block;

}

BlSim::Caches::Caches(char *cache_config_fname, unsigned int numCores)
{
	if(cache_config_fname == NULL)
	{
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

	
	}
	else
	{
		cerr<<"#### Sorry, we have not supported read cache configs from file"<<endl;
		exit(-8);
	}

#ifdef DEBUG_CACHE_SIMULATOR
	//print_cache_config();
#endif
}

BlSim::Caches::~Caches()
{
	uint32_t i;
	uint32_t j;
    	cout<<"in ~Caches()"<<endl;

	for(i = 0; i < m_level; i++)
        {
        	for(j = 0; j < m_cache_set_count[i]; j++)
                {
			if(m_cache_sets[i][j])
			{
				delete m_cache_sets[i][j];
				m_cache_sets[i][j] = NULL;
			}
                }
		delete []m_cache_sets[i];
		m_cache_sets[i] = NULL;
       	}
}

BlSim::CacheSet* BlSim::Caches::access_cache_at_level(uint64_t maddr,
                                                          uint32_t level,
                                                          uint64_t *mtag,
                                                          bool* hit)
{
	//uint64_t mem_tag;
	uint32_t set_index;

	assert(level >= 0 && level < m_level);

	get_cache_addr_parts(maddr, mtag, &set_index, level);
    
    	assert(set_index >= 0 && set_index < m_cache_set_count[level] && (m_cache_sets[level][set_index] != NULL));

	CacheBlock *p_block = m_cache_sets[level][set_index]->find_block(*mtag);
	if(p_block)
	{
	    //cout << "find block" << endl;
		//Yeah, we find the cache block in this set. Hit it in the mru
		m_cache_sets[level][set_index]->hit_access(&p_block);
        	assert(m_cache_sets[level][set_index]->get_mru_block()->m_block_tag == *mtag);
		*hit = true;
#ifdef DEBUG_CACHE_SIMULATOR
		//cout<<"Cache Hit at Level "<<level <<": addr=0x"<<hex<<maddr<<", mtag="<<*mtag<<dec<<", set_index="<<set_index<<", sb_index="<<*sub_block_index<<endl;
#endif
	}
	return m_cache_sets[level][set_index];
}

/*void BlSim::Caches::output_mem_reqs_statistics()
{
	uint32_t i;
	cout<<endl<<endl<<"$$$$ Memory Request Cache Statistics:"<<endl;
	for(i = 0; i < m_level; i++)
	{
		cout<<i<<"th level cache:read="<<m_mem_reads[i]<<", read_hit="<<m_mem_reads_hit[i]<<", read_miss="<<m_mem_reads_miss[i]<<endl;
		cout<<"\t\twrite="<<m_mem_writes[i]<<", write_hit="<<m_mem_writes_hit[i]<<", write_miss="<<m_mem_writes_miss[i]<<endl;
	}

}*/


bool BlSim::Caches::access_cache(uint64_t maddr, uint32_t memop)
{
	uint32_t i;
	CacheSet *access_cache_sets[MAX_CACHE_LEVEL];
	uint64_t mtags[MAX_CACHE_LEVEL];
   	 bool hit = false;
   	 m_total_count++;
	for(i = 0; i < m_level; i++)
	{
        access_cache_sets[i] = access_cache_at_level(maddr, i, &mtags[i], &hit);
        assert(access_cache_sets[i] != NULL);
        if(memop == MEM_READ)
		{
			m_mem_reads[i]++;
			if(hit)
			{
				m_mem_reads_hit[i]++;
			}
			else
			{
				m_mem_reads_miss[i]++;
			}
		}
		else if(memop == MEM_WRITE)
		{
			m_mem_writes[i]++;
			if(hit)
			{
				m_mem_writes_hit[i]++;
			}
			else
			{
				m_mem_writes_miss[i]++;
			}
		}
		if(hit)
		{
		    //cout << "hit" << endl;
			//we got the cache block hit in this cache level
			m_hit_count++;
			add_trace_to_memory(maddr, (unsigned int)memop);
			break;
		}
		
	}
	
	if(!hit)
	{
	   //cout << "miss" << endl;
	   m_miss_count++;
		//the cache block miss in all level of caches, we put it in the 
		assert(i == m_level);
		uint32_t accessed_sub_block;

		//for memory write request, first send read, write when the cache block is written back
		accessed_sub_block = access_cache_sets[m_level-1]->load_new_block_in_LLC(maddr, mtags[m_level-1]);
		i--;
		/*if(write_back_mem_trace){
			hit=false;
		}*/
	}

#ifdef DEBUG_CACHE_SIMULATOR
    	//cout<<"## Enter in Cache replaced: "<<i<<endl;
#endif

	//Now we need to put this new block into the upper cache until to the L1 cache
	while(i > 0)
	{
		//replace the upper lru block with the lower new mru block
        	assert(i > 0);
        	assert(access_cache_sets[i] && access_cache_sets[i-1]);
        	assert(access_cache_sets[i]->get_mru_block()->m_block_tag == mtags[i]);
#ifdef DEBUG_CACHE_SIMULATOR
        	//cout<<"## Before cache replaced at level " << i <<endl;
#endif
		cache_replaced(access_cache_sets[i], access_cache_sets[i-1], i, mtags[i-1], maddr);
		i--;
	}

#ifdef DEBUG_CACHE_SIMULATOR
    	//cout<<"## After Cache replaced"<<endl;
#endif


	assert(access_cache_sets[0]->get_mru_block()->m_block_tag == mtags[0]);
	access_cache_sets[0]->get_mru_block()->hit_access(memop);
	return hit;
}

void BlSim::add_trace_to_memory(unsigned long maddr, unsigned int rw)
{
	Transaction *trans = new Trannsaction();
	if(mem_trace_buf_next_index == mem_trace_next_committing_index)
	{
		//no space for new mem trace
		//we need to write back ready committed mem trace into file to free some space
		commit_mem_trace_buf_to_file();
		assert(mem_trace_buf_next_index != mem_trace_next_committing_index);
	}

	if(mem_trace_buf_next_index == max_mem_trace_gra_count)
	{
		mem_trace_buf_next_index = 0;
	}

	assert(mem_trace_buf_next_index < max_mem_trace_gra_count);
	p_mem_trace_buf[mem_trace_buf_next_index].m_addr = maddr;
	p_mem_trace_buf[mem_trace_buf_next_index].m_rw = rw;
	p_mem_trace_buf[mem_trace_buf_next_index].m_committed = 0;
	p_mem_trace_buf[mem_trace_buf_next_index].m_sb_count = 0;

	p_mem_trace_buf[mem_trace_buf_next_index].m_skipped = 0;

	p_mem_trace_buf[mem_trace_buf_next_index].m_threadid = thdid;


	p_mem_trace_buf[mem_trace_buf_next_index].m_upper_ins_count = non_mem_ins_count; //////////
	total_non_mem_ins_count += non_mem_ins_count;

	non_mem_ins_count = 0;

	//assert(p_mem_trace_buf[mem_trace_buf_next_index].m_p_block_LLC->m_block_addr == maddr);

	mem_trace_buf_next_index++;

	if(rw == MEM_READ)
	{
		mem_read_traces++;
	}
	else
	{
		mem_write_traces++;
	}
}
void BlSim::Caches::cache_replaced(CacheSet *p_lower_set, 
                                     CacheSet *p_upper_set,
                                     uint32_t lower_level,
                                     uint64_t upper_mtag,
                                     uint64_t maddr)
{
	CacheBlock *p_evicted_block;
	CacheSet *p_wb_set;
	CacheBlock *p_wb_block;

	CacheBlock *p_new_mru_block;	

	uint64_t wb_maddr;
	uint64_t wb_mem_tag;
    	uint32_t wb_set_index;

    	assert(lower_level >= 1 && lower_level < m_level);

#ifdef DEBUG_CACHE_SIMULATOR
     //cout<<"## Evicted lru at level "<<lower_level-1<<endl;
#endif

	p_evicted_block = p_upper_set->evict_lru_block();
    	assert(p_evicted_block);
	wb_maddr = p_evicted_block->m_block_addr;


	if(wb_maddr != INVALID_BLOCK)
	{
		//we only write back the block with valid addr and tag,
		//Attention: we can not use the cache tag form upper set, cause it is different with the lower set
		//Thus, we need to recalculate the tag
        	get_cache_addr_parts(wb_maddr, &wb_mem_tag, &wb_set_index, lower_level);

		p_wb_set = m_cache_sets[lower_level][wb_set_index];
        	p_wb_block = p_wb_set->find_block(wb_mem_tag);

		assert(p_wb_block != NULL);
		assert(p_evicted_block->m_parent_block_in_lower->m_block_tag == p_wb_block->m_block_tag);

		//write back the access distribution into the lower wb cache block
		p_wb_block->get_data_from_evicted(p_evicted_block);
	}

	
	//Now load the new block into upper set mru position
	p_new_mru_block = p_lower_set->get_mru_block();
	
	p_evicted_block->m_block_addr = maddr;
	p_evicted_block->m_block_tag = upper_mtag;
	
	p_new_mru_block->push_data_into_upper_block(p_evicted_block);

	//Finally, put this new block into the mru position of the upper cache
	p_upper_set->put_accessed_block_in_mru(p_evicted_block);	
}

void BlSim::Caches::get_cache_addr_parts(uint64_t maddr, uint64_t *mem_tag, uint32_t *set_index, uint32_t level)
{
	uint64_t tmp = maddr;

	assert(level < m_level);		
    //cout << "low bit" << m_block_low_bits[level] << endl;
    //cout << "mask" << m_set_index_mask[level] << endl;
    //cout << "index" << m_set_index_bits[level] << endl;
	tmp >>= m_block_low_bits[level];
	*set_index = (tmp & m_set_index_mask[level]);

	tmp >>= m_set_index_bits[level];
	*mem_tag = tmp;

}

void BlSim::Caches::dump_statistic()
{
	float hit_rate = (float)(m_hit_count) / m_total_count;

	cout << "hit: " << m_hit_count
		<< " miss: " << m_miss_count
		<< "\t total: " << m_total_count
 	      << "\t hit rate: " << hit_rate
 	      << "\t evicted LLC count: " << evicted_LLC_count << endl;

}


bool BlSim::Caches::writebackornot()
{
	if(write_back_mem_trace!=0){
		cout << "write back to memory!\t" << std::endl;
		return true;
	}
	else{
		cout << "not write back to memory \t" << std::endl;
		write_back_mem_trace=0;
		return false;
	}
	
}

void BlSim::Caches::print_cache_config()
{
	uint32_t i;
	uint32_t j;

	cout<<"$$$$ The cache config details:"<<endl;
	cout<<"\t level of cache = " <<m_level<<endl;
	for(i = 0; i < m_level; i++)
	{
		cout<<"\t"<<i<<"th level cache:capacity="<<m_cache_capacity[i];
		cout<<", way_count="<<m_cache_way_count[i];
		cout<<", block_size="<<m_block_size[i];
		cout<<", set_capacity="<<m_cache_set_capacity[i];
		cout<<", set_count="<<m_cache_set_count[i]<<endl;

		cout<<"\t"<<"Some bits masks info:";
		cout<<"<set_index_bits,low_bits>=<"<<m_set_index_bits[i]<<","<<m_block_low_bits[i]<<">"<<endl;
		cout<<"mask:"<<hex<<m_set_index_mask[i]<<","<<m_block_low_mask[i]<<dec<<endl;
	}	

	cout<<endl<<"LLC shared is "<<m_shared_LLC<<endl;

	cout<<endl<<endl<<"Cache Sets status:"<<endl;
	for(i = 0; i < m_level; i++)
        {
                for(j = 0; j < m_cache_set_count[i]; j++)
                {
                        if(m_cache_sets[i][j])
                        {
				cout<<"Cache Level "<<i<<", Set "<<j<<": ";
                                m_cache_sets[i][j]->print_cache_set();
                        }
                }
                
	}
}
