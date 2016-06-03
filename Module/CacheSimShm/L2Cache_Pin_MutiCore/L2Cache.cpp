#include "L2Cache.h"
#include "L1Cache.h"
#include <math.h>

L2Cache::
L2Cache(unsigned int l1_num)
: m_cache_size(3*1024*1024)
, m_way_num(12)
, m_line_size(64)
, m_l1_num(l1_num)
, m_l2_bus_wr_cnt(0)
, m_l2_bus_rd_cnt(0)
{
	m_l1_ptr = new L1Cache* [l1_num];
	for(unsigned int i=0;i<l1_num;i++)
	{
		m_l1_ptr[i] = NULL;
	}
	
	m_set_num = m_cache_size/(m_way_num*m_line_size);

	//m_l2_set_total_cnt   = new unsigned long long [m_set_num];
	//m_l2_set_miss_cnt    = new unsigned long long [m_set_num];
	//m_l2_set_replace_cnt = new unsigned long long [m_set_num];

	//for(unsigned int i=0;i<m_set_num;i++)
	//{
	//	m_l2_set_total_cnt[i]   = 0;
	//	m_l2_set_miss_cnt[i]    = 0;
	//	m_l2_set_replace_cnt[i] = 0;
	//}
	
	m_line_bits = (unsigned int)(log(m_line_size)/log(2));
	m_set_bits  = (unsigned int)(log(m_set_num)/log(2));
	
	m_set_lsb = m_line_bits;
	m_set_msb = m_set_lsb+m_set_bits-1;
	
	uint64_t t_mask = 0x1;	
	m_set_mask = ((t_mask<<m_set_bits)-1)<<m_set_lsb;
	
	
	cout<<" L2"<<"\n"
		<<"        cache size "<<dec<<m_cache_size<<" bytes\n"
		<<"           way num "<<dec<<m_way_num<<"\n"
		<<"           set num "<<dec<<m_set_num<<"\n"
		<<"           set bits - addr["<<dec<<m_set_msb<<":"<<m_set_lsb<<"]\n"
		<<"           set mask 0x"<<hex<<m_set_mask
		<<endl;

	m_tag_array = new L2Set* [m_set_num];
	for(unsigned int i=0;i<m_set_num;i++)
	{
		m_tag_array[i] = new L2Set (m_way_num);
	}

	//SC_THREAD(testProcess);
}

void L2Cache::
dump(unsigned int set_index)
{
	L2Set* t_set = m_tag_array[set_index];
	
	cout<<" set["<<dec<<set_index<<"]: ";
	for(unsigned int i=0;i<m_way_num;i++)
	{
		cout<<"0x"<<hex<<t_set->m_tag[i].m_tag<<"|"<<((t_set->m_tag[i].m_mesi==I)?"0":"1")
			<<((i == m_way_num-1)?" ":", ");
	}
	cout<<endl;
}
void L2Cache::
dump_addr(unsigned int addr)
{
	unsigned int set_index = get_set(addr);
	L2Set* t_set = m_tag_array[set_index];
	
	cout<<"---- "<<" set["<<dec<<set_index<<"]: ";
	for(unsigned int i=0;i<m_way_num;i++)
	{
		cout<<"0x"<<hex<<t_set->m_tag[i].m_tag<<"|"<<((t_set->m_tag[i].m_mesi==I)?"0":"1")
			<<((i == m_way_num-1)?" ":", ");
	}
	cout<<endl;
}

bool L2Cache::
load(unsigned int l1_id, uint64_t addr, CacheMESI& l1_mesi, CacheStatus& status)
{
	unsigned int t_set_index = get_set(addr);
	L2Set* t_set;
	assert(t_set_index<m_set_num);
	
	t_set = m_tag_array[t_set_index];

	//check whether hit in L2
	for(unsigned int i=0;i<m_way_num;i++)
	{
		if(t_set->m_tag[i].m_mesi == I)
		{
			continue;
		}
		
		if(t_set->m_tag[i].m_tag == get_tag(addr))//hit
		{
			if(t_set->m_tag[i].m_vld_l1_que.empty())//no l1 has copy
			{
				t_set->m_tag[i].m_vld_l1_que.push_back(l1_id);
				l1_mesi = E;

				status.m_l2_hit						= true;
				status.m_l2_query_l1				= false;
				status.m_llc_victim					= false;
			}
			else if(t_set->m_tag[i].m_vld_l1_que.size() == 1)//only 1 L1 has copy
			{
				unsigned int t_query_l1_id = t_set->m_tag[i].m_vld_l1_que.front();
				CacheMESI t_mesi;

				t_mesi = m_l1_ptr[t_query_l1_id]->query(addr);

				if(t_mesi == M)//dirty
				{
					t_set->m_tag[i].m_mesi = M;
				}
				
				t_set->m_tag[i].m_vld_l1_que.push_back(l1_id);
				l1_mesi = S;

				status.m_l2_hit						= true;
				status.m_l2_query_l1				= true;
				status.m_l2_query_l1_clean			= (t_mesi != M);
				status.m_llc_victim					= false;
 
			}
			else// more than 1 L1s has copy
			{
				t_set->m_tag[i].m_vld_l1_que.push_back(l1_id);
				l1_mesi = S;

				status.m_l2_hit						= true;
				status.m_l2_query_l1				= false;
				status.m_llc_victim					= false;
			}
			
			t_set->deleteLRUEntry(&t_set->m_tag[i]);
			t_set->addLRUEntry(&t_set->m_tag[i]);

			return true;//l2 hit
		}
	}
	
	//l2 miss
	//find one way to fill this line
	if(!t_set->m_free_way_que.empty())//has free entry
	{
		unsigned int t_way_id = t_set->m_free_way_que.front();
		t_set->m_free_way_que.pop_front();
		
		assert(t_set->m_tag[t_way_id].m_mesi == I);
		
		t_set->m_tag[t_way_id].m_tag = get_tag(addr);
		t_set->m_tag[t_way_id].m_mesi = E;
		t_set->m_tag[t_way_id].m_vld_l1_que.push_back(l1_id);
		t_set->addLRUEntry(&t_set->m_tag[t_way_id]);

		m_l2_bus_rd_cnt++;

		status.m_l2_hit					= false;
		status.m_l2_query_l1			= false;
		status.m_llc_victim				= false;
	}
	else//no free entry, need replace
	{
		//find the oldest way
		uint64_t t_replace_addr;
		unsigned int t_replace_way = 0;
		t_replace_way = t_set->getLRUFront();
		
		t_replace_addr = get_addr(t_set->m_tag[t_replace_way].m_tag,t_set_index);
		
		//query all l1 for the replace line
		if(t_set->m_tag[t_replace_way].m_vld_l1_que.size() == 1)
		{
			CacheMESI t_mesi;

			t_mesi = m_l1_ptr[t_set->m_tag[t_replace_way].m_vld_l1_que[0]]->queryInv(t_replace_addr);

			if((t_mesi == M)||(t_set->m_tag[t_replace_way].m_mesi == M))//query dirty or L2 dirty
			{
				//todo: write back dirty data to memory
				m_l2_bus_wr_cnt++;

				status.m_l2_hit						= false;
				status.m_l2_query_l1				= false;
				status.m_llc_victim					= true;
				status.m_llc_victim_query_l1		= true;
				status.m_llc_victim_query_l1_clean	= (t_mesi != M);
				status.m_llc_victim_clean			= false;
				status.m_llc_victim_addr			= t_replace_addr;
			}
			else
			{
				status.m_l2_hit						= false;
				status.m_l2_query_l1				= false;
				status.m_llc_victim					= true;
				status.m_llc_victim_query_l1		= true;
				status.m_llc_victim_query_l1_clean	= true;
				status.m_llc_victim_clean			= true;
			}
		}
		else
		{
			CacheMESI t_mesi = I;

			for(unsigned int i=0;i<t_set->m_tag[t_replace_way].m_vld_l1_que.size();i++)
			{
				t_mesi = m_l1_ptr[t_set->m_tag[t_replace_way].m_vld_l1_que[i]]->queryInv(t_replace_addr);

				assert(t_mesi == S);
			}
			if(t_set->m_tag[t_replace_way].m_mesi == M)
			{
				//todo: write back dirty data to memory
				m_l2_bus_wr_cnt++;

				status.m_l2_hit						= false;
				status.m_l2_query_l1				= false;
				status.m_llc_victim					= true;
				status.m_llc_victim_query_l1		= (t_mesi == S);
				status.m_llc_victim_query_l1_clean	= true;
				status.m_llc_victim_clean			= false;
				status.m_llc_victim_addr			= t_replace_addr;
			}
			else
			{
				status.m_l2_hit						= false;
				status.m_l2_query_l1				= false;
				status.m_llc_victim					= true;
				status.m_llc_victim_query_l1		= (t_mesi == S);
				status.m_llc_victim_query_l1_clean	= true;
				status.m_llc_victim_clean			= true;
			}
		}
		//fill one new line
		t_set->m_tag[t_replace_way].m_tag = get_tag(addr);
		t_set->m_tag[t_replace_way].m_mesi = E;
		t_set->m_tag[t_replace_way].m_vld_l1_que.clear();
		t_set->m_tag[t_replace_way].m_vld_l1_que.push_back(l1_id);
		t_set->deleteLRUEntry(&t_set->m_tag[t_replace_way]);
		t_set->addLRUEntry(&t_set->m_tag[t_replace_way]);

		m_l2_bus_rd_cnt++;
	}
	
	l1_mesi = E;

	return false;//l2 miss
}


bool L2Cache::
loadInv(unsigned int debug_id,unsigned int l1_id, uint64_t addr, CacheStatus& status)
{
	unsigned int t_set_index = get_set(addr);
	L2Set* t_set;
	assert(t_set_index<m_set_num);
	
	t_set = m_tag_array[t_set_index];

	//check whether hit in L2
	for(unsigned int i=0;i<m_way_num;i++)
	{
		if(t_set->m_tag[i].m_mesi == I)
		{
			continue;
		}
		
		if(t_set->m_tag[i].m_tag == get_tag(addr))//hit
		{
			if(t_set->m_tag[i].m_vld_l1_que.empty())//no l1 has copy
			{
				//no need to invalid
				t_set->m_tag[i].m_vld_l1_que.push_back(l1_id);

				status.m_l2_hit					= true;
				status.m_l2_query_l1			= false;
				status.m_llc_victim				= false;
			}
			else if(t_set->m_tag[i].m_vld_l1_que.size() == 1)//only 1 L1 has copy
			{
				unsigned int t_query_l1_id = t_set->m_tag[i].m_vld_l1_que.front();
				CacheMESI t_mesi;
				if(l1_id != t_query_l1_id)
				{
					t_mesi = m_l1_ptr[t_query_l1_id]->queryInv(addr);

					if(t_mesi == M)//dirty
					{
						t_set->m_tag[i].m_mesi = M;
					}
					
					t_set->m_tag[i].m_vld_l1_que.clear();
					t_set->m_tag[i].m_vld_l1_que.push_back(l1_id);

					status.m_l2_hit						= true;
					status.m_l2_query_l1				= true;
					status.m_l2_query_l1_clean			= (t_mesi != M);
					status.m_llc_victim					= false;
				}
				else
				{
					status.m_l2_hit						= true;
					status.m_l2_query_l1				= false;
					status.m_l2_query_l1_clean			= false;
					status.m_llc_victim					= false;
				}
			}
			else //at least 1 L1 has copy
			{
				bool t_need_query = !t_set->m_tag[i].m_vld_l1_que.empty();
				for(unsigned int j=0;j<t_set->m_tag[i].m_vld_l1_que.size();j++)
				{
					if(t_set->m_tag[i].m_vld_l1_que[j] != l1_id)
					{
						m_l1_ptr[t_set->m_tag[i].m_vld_l1_que[j]]->queryInv(addr);
					}
				}
								
				t_set->m_tag[i].m_vld_l1_que.clear();
				t_set->m_tag[i].m_vld_l1_que.push_back(l1_id);

				status.m_l2_hit						= true;
				status.m_l2_query_l1				= t_need_query;
				status.m_l2_query_l1_clean			= true;
				status.m_llc_victim					= false;
			}
			t_set->deleteLRUEntry(&t_set->m_tag[i]);
			t_set->addLRUEntry(&t_set->m_tag[i]);

			return true;//l2 hit
		}
	}
	
	//l2 miss
	//find one way to fill this line
	if(!t_set->m_free_way_que.empty())//has free entry
	{
		unsigned int t_way_id = t_set->m_free_way_que.front();
		t_set->m_free_way_que.pop_front();
		
		assert(t_set->m_tag[t_way_id].m_mesi == I);
		
		t_set->m_tag[t_way_id].m_tag = get_tag(addr);
		t_set->m_tag[t_way_id].m_mesi = E;
		t_set->m_tag[t_way_id].m_vld_l1_que.push_back(l1_id);
		t_set->addLRUEntry(&t_set->m_tag[t_way_id]);

		m_l2_bus_rd_cnt++;

		status.m_l2_hit					= false;
		status.m_l2_query_l1			= false;
		status.m_llc_victim				= false;
	}
	else//no free entry, need replace
	{
		//find the oldest way
		uint64_t t_replace_addr;
		unsigned int t_replace_way = 0;

		t_replace_way = t_set->getLRUFront();
		
		t_replace_addr = get_addr(t_set->m_tag[t_replace_way].m_tag,t_set_index);
		
		//query all l1 for the replace line
		if(t_set->m_tag[t_replace_way].m_vld_l1_que.size() == 1)
		{
			CacheMESI t_mesi;

			t_mesi = m_l1_ptr[t_set->m_tag[t_replace_way].m_vld_l1_que[0]]->queryInv(t_replace_addr);

			if((t_mesi == M)||(t_set->m_tag[t_replace_way].m_mesi == M))//query dirty or L2 dirty
			{
				//todo: write back dirty data to memory
				m_l2_bus_wr_cnt++;

				status.m_l2_hit						= false;
				status.m_l2_query_l1				= false;
				status.m_llc_victim					= true;
				status.m_llc_victim_query_l1		= true;
				status.m_llc_victim_query_l1_clean	= (t_mesi != M);
				status.m_llc_victim_clean			= false;
				status.m_llc_victim_addr			= t_replace_addr;
			}
			else
			{
				status.m_l2_hit						= false;
				status.m_l2_query_l1				= false;
				status.m_llc_victim					= true;
				status.m_llc_victim_query_l1		= true;
				status.m_llc_victim_query_l1_clean	= true;
				status.m_llc_victim_clean			= true;
			}
		}
		else
		{
			CacheMESI t_mesi = I;

			for(unsigned int i=0;i<t_set->m_tag[t_replace_way].m_vld_l1_que.size();i++)
			{
				t_mesi = m_l1_ptr[t_set->m_tag[t_replace_way].m_vld_l1_que[i]]->queryInv(t_replace_addr);

				assert(t_mesi == S);
			}
			if(t_set->m_tag[t_replace_way].m_mesi == M)
			{
				//todo: write back dirty data to memory
				m_l2_bus_wr_cnt++;
				
				status.m_l2_hit						= false;
				status.m_l2_query_l1				= false;
				status.m_llc_victim					= true;
				status.m_llc_victim_query_l1		= (t_mesi == S);
				status.m_llc_victim_query_l1_clean	= true;
				status.m_llc_victim_clean			= false;
				status.m_llc_victim_addr			= t_replace_addr;
			}
			else
			{
				status.m_l2_hit						= false;
				status.m_l2_query_l1				= false;
				status.m_llc_victim					= true;
				status.m_llc_victim_query_l1		= (t_mesi == S);
				status.m_llc_victim_query_l1_clean	= true;
				status.m_llc_victim_clean			= true;
			}
		}
		
		//fill one new line
		t_set->m_tag[t_replace_way].m_tag = get_tag(addr);
		t_set->m_tag[t_replace_way].m_mesi = E;
		t_set->m_tag[t_replace_way].m_vld_l1_que.clear();
		t_set->m_tag[t_replace_way].m_vld_l1_que.push_back(l1_id);
		t_set->deleteLRUEntry(&t_set->m_tag[t_replace_way]);
		t_set->addLRUEntry(&t_set->m_tag[t_replace_way]);

		m_l2_bus_rd_cnt++;
	}
	return false;//l2 miss
}

void L2Cache::
evict(unsigned int l1_id, uint64_t addr, bool dirty)
{
	unsigned int t_set_index = get_set(addr);
	L2Set* t_set;
	assert(t_set_index<m_set_num);
	
	t_set = m_tag_array[t_set_index];
	
	for(unsigned int i=0;i<m_way_num;i++)
	{
		if(t_set->m_tag[i].m_mesi == I)
		{
			continue;
		}
		
		if(t_set->m_tag[i].m_tag == get_tag(addr))//hit
		{
			if(!dirty)//clean
			{
				for(unsigned int j=0;j<t_set->m_tag[i].m_vld_l1_que.size();j++)
				{
					if(t_set->m_tag[i].m_vld_l1_que[j] == l1_id)
					{
						t_set->m_tag[i].m_vld_l1_que.erase(
								t_set->m_tag[i].m_vld_l1_que.begin()+j);
						return;
					}
				}
				assert(false);
			}
			else//dirty
			{
				assert(t_set->m_tag[i].m_vld_l1_que.size() == 1);
				t_set->m_tag[i].m_vld_l1_que.clear();

				t_set->m_tag[i].m_mesi = M;

				return;
			}
		}
	}

	cout<<" Error, evict addr 0x"<<hex<<addr
		<<", in L2 set["<<dec<<t_set_index<<"]"
		<<endl;

	assert(false);//can not miss
}


unsigned int L2Cache::
get_set(uint64_t addr)
{
	return (unsigned int)((addr&m_set_mask)>>m_set_lsb);
}

uint64_t L2Cache::
get_tag(uint64_t addr)
{
	return (addr>>(m_line_bits+m_set_bits)<<(m_line_bits+m_set_bits));
}
uint64_t L2Cache::
get_addr(uint64_t tag, unsigned int set)
{
	uint64_t addr = 0x0;
	
	addr = tag | (set<<m_set_lsb);
	return addr;
}

