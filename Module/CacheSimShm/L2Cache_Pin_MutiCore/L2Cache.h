#ifndef L2_CACHE_FUNC_H
#define L2_CACHE_FUNC_H

#include <iostream>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include <deque>

#include "ComDef.h"

using namespace std;

class L1Cache;

class L2Cache
{
	public:
		L2Cache(unsigned int l1_num);
		
		void setL1Ptr(unsigned int l1_id,L1Cache* ptr){assert(l1_id<m_l1_num);m_l1_ptr[l1_id] = ptr;}
		
		bool load(unsigned int l1_id, uint64_t addr,CacheMESI& l1_mesi,CacheStatus& status);
		bool loadInv(unsigned int debug_id,unsigned int l1_id, uint64_t addr, CacheStatus& status);
		void evict(unsigned int l1_id, uint64_t addr, bool dirty);

		void dump(unsigned int set_index);
		void dump_addr(unsigned int addr);

		uint64_t get_bus_rd_cnt(){return m_l2_bus_rd_cnt;}
		uint64_t get_bus_wr_cnt(){return m_l2_bus_wr_cnt;}

		void set_bus_rd_cnt(uint64_t t){m_l2_bus_rd_cnt = t;}
		void set_bus_wr_cnt(uint64_t t){m_l2_bus_wr_cnt = t;}
	public:
		unsigned int		m_cache_size;
		unsigned int		m_way_num;
		unsigned int		m_line_size;
		
		struct L2Tag
		{
			L2Tag()
			: m_mesi(I)
			, m_last(NULL)
			, m_next(NULL)
			//, m_tag_used_time(SC_ZERO_TIME)
			{}
			
			CacheMESI		m_mesi;
			uint64_t			m_tag;
			
			std::deque<unsigned int>
							m_vld_l1_que;

			L2Tag*			m_last;
			L2Tag*			m_next;

			unsigned int	m_way_id;
			
			//sc_core::sc_time
			//				m_tag_used_time;
		};
		
		struct L2Set
		{
			L2Set(unsigned int way_num)
			: m_way_num(way_num)
			, m_lru_front(NULL)
			, m_lru_back(NULL)
			{
				m_tag = new L2Tag[way_num];
				for(unsigned int i=0;i<way_num;i++)
				{
					m_free_way_que.push_back(i);
					m_tag[i].m_way_id = i;
				}
			}

			void addLRUEntry(L2Tag* entry)
			{
				assert(entry->m_last == NULL);
				assert(entry->m_next == NULL);

				if(m_lru_front == NULL)
				{
					m_lru_front = entry;
					m_lru_back  = entry;
				}
				else
				{
					m_lru_back->m_next = entry;
					entry->m_last = m_lru_back;
					m_lru_back = entry;
				}
			}

			void deleteLRUEntry(L2Tag* entry)
			{
				if((entry == m_lru_back) && (entry == m_lru_front))
				{
					m_lru_back = NULL;
					m_lru_front = NULL;
					assert(entry->m_last == NULL);
					assert(entry->m_next == NULL);
				}	
				else if(entry == m_lru_back)//back
				{
					m_lru_back = entry->m_last;
					m_lru_back->m_next = NULL;

					entry->m_last = NULL;
					assert(entry->m_next == NULL);
				}
				else if(entry == m_lru_front)//front
				{
					m_lru_front = entry->m_next;
					m_lru_front->m_last = NULL;

					entry->m_next = NULL;
					assert(entry->m_last == NULL);
				}
				else
				{
					assert(entry->m_last != NULL);
					assert(entry->m_next != NULL);

					entry->m_last->m_next = entry->m_next;
					entry->m_next->m_last = entry->m_last;

					entry->m_last = NULL;
					entry->m_next = NULL;
				}
			}

			unsigned int getLRUFront()
			{
				assert(m_lru_front != NULL);
				return m_lru_front->m_way_id;
			}
			
			unsigned int 	m_way_num;
			L2Tag			*m_tag;
			
			std::deque<unsigned int>
							m_free_way_que;

			L2Tag			*m_lru_front;
			L2Tag			*m_lru_back;
		};
	private:
		unsigned int get_set(uint64_t addr);
		uint64_t get_tag(uint64_t addr);
		uint64_t get_addr(uint64_t tag, unsigned int set);
	public:
		unsigned int		m_l1_num;
		L1Cache				**m_l1_ptr;
	
		L2Set				**m_tag_array;
		unsigned int		m_set_num;
		unsigned int		m_set_msb;
		unsigned int		m_set_lsb;
		unsigned int		m_set_bits;
		
		unsigned int		m_line_bits;
		uint64_t				m_set_mask;

		uint64_t				m_l2_bus_wr_cnt;
		uint64_t				m_l2_bus_rd_cnt;

		//static
		bool				m_debug_en;
		//unsigned long long	*m_l2_set_total_cnt;
		//unsigned long long	*m_l2_set_miss_cnt;
		//unsigned long long	*m_l2_set_replace_cnt;
};

#endif

