#ifndef L1_CACHE_FUNC_H
#define L1_CACHE_FUNC_H

#include <iostream>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include <deque>

#include "ComDef.h"

using namespace std;

class L2Cache;

class L1Cache
{
	public:
		L1Cache(unsigned int id);
		
		void setL2Ptr(L2Cache* ptr){m_l2_ptr = ptr;}
	
		void load(uint64_t addr,CacheStatus& status);
		void store(uint64_t addr,CacheStatus& status);
	
		CacheMESI query(uint64_t addr);
		CacheMESI queryInv(uint64_t addr);

		void dump(unsigned int set_index);
		void dump_addr(unsigned int addr);
		void dump_free_way_que(unsigned int set_index);

	public:
		unsigned int		m_cache_size;
		unsigned int		m_way_num;
		unsigned int		m_line_size;
		
		struct L1Tag
		{
			L1Tag()
			: m_mesi(I)
			, m_last(NULL)
			, m_next(NULL)
			//, m_tag_used_time(SC_ZERO_TIME)
			{}
			
			CacheMESI		m_mesi;

			uint64_t		m_tag;

			L1Tag*			m_last;
			L1Tag*			m_next;

			unsigned int	m_way_id;
			
			//sc_core::sc_time
			//				m_tag_used_time;
		};
		
		struct L1Set
		{
			L1Set(unsigned int way_num)
			: m_way_num(way_num)
			, m_lru_front(NULL)
			, m_lru_back(NULL)
			{
				m_tag = new L1Tag[way_num];
				for(unsigned int i=0;i<way_num;i++)
				{
					m_tag[i].m_way_id = i;
					m_free_way_que.push_back(i);
				}
			}

			void addLRUEntry(L1Tag* entry)
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

			void deleteLRUEntry(L1Tag* entry)
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
			L1Tag			*m_tag;
			
			std::deque<unsigned int>
							m_free_way_que;

			L1Tag			*m_lru_front;
			L1Tag			*m_lru_back;
		};
	private:
		unsigned int get_set(uint64_t addr);
		uint64_t get_tag(uint64_t addr);
		uint64_t get_addr(uint64_t tag, unsigned int set);
	private:
		unsigned int		m_id;

		L2Cache				*m_l2_ptr;
	
		L1Set				**m_tag_array;
		unsigned int		m_set_num;
		unsigned int		m_set_msb;
		unsigned int		m_set_lsb;
		unsigned int		m_set_bits;
		
		unsigned int		m_line_bits;
		uint64_t				m_set_mask;
};

#endif

