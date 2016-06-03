#ifndef CACHE_COM_DEF_H
#define CACHE_COM_DEF_H

enum CacheMESI{M,E,S,I};

struct CacheStatus
{
	CacheStatus()
	: m_l1_hit(false)
	, m_l2_hit(false)
	, m_l1_victim(false)
	, m_l1_victim_clean(true)
	, m_l2_query_l1(false)
	, m_l2_query_l1_clean(true)
	, m_llc_victim(false)
	, m_llc_victim_query_l1(false)
	, m_llc_victim_query_l1_clean(true)
	, m_llc_victim_clean(true)
	, m_llc_victim_addr(0x0)
	{}

	bool						m_l1_hit;
	bool						m_l2_hit;//when l1 hit = true, l2 hit = true means need loadInv L2

	bool						m_l1_victim;
	bool						m_l1_victim_clean;
	
	bool						m_l2_query_l1;
	bool						m_l2_query_l1_clean;

	bool						m_llc_victim;
	bool						m_llc_victim_query_l1;
	bool						m_llc_victim_query_l1_clean;
	bool						m_llc_victim_clean;
	uint64_t					m_llc_victim_addr;
};


#endif

