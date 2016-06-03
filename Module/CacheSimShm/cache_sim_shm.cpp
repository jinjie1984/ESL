#include "pin.H"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <set>
#include <list>
#include <sstream>
#include <sched.h>
#include <sys/time.h>

#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <string.h>


#define PIN_SC_SHARED_NUM 2
#define BUF_INST_NUM 10000

KNOB<bool>		KnobPhyAddr			(KNOB_MODE_WRITEONCE,	"pintool",	"phy",		"0",	"Use virtual to physical address translate");
KNOB<int>		KnobShareKey		(KNOB_MODE_WRITEONCE,	"pintool",	"key",		"1000",	"share memory key");


VOID fini(INT32, VOID*);

struct Inst
{
	uint64_t		m_fetch_addr;
	uint64_t		m_rd_addr;
	uint64_t		m_wr_addr;
	bool			m_rd_en;
	bool			m_wr_en;
};

struct Exchange
{
	bool			m_ready;
	bool			m_pin_complete;

	Inst			m_inst[BUF_INST_NUM];
	unsigned int	m_inst_cnt;
};

// Global parameters
Exchange*		g_shared[PIN_SC_SHARED_NUM];
int				g_shared_num		= 0;
uint64_t		g_page_miss_cnt		= 0;
uint64_t		g_page_ignore_cnt	= 0;
uint64_t		g_inst_cnt			= 0;

int				g_tmp_data			= 0;

bool			g_phy_en			= false;

//---------------Virtual to Physical----------------------------
#define GET_BIT(X,Y)        ((X & ((uint64_t)1<<Y))>>Y)
#define GET_PFN(X)          (X & 0x7FFFFFFFFFFFFF)
#define GET_PAGEOFFSET(X)   (X & 0xFFF)

ADDRINT virtualToPhysical(ADDRINT vaddr)
{
	FILE*		pm_file;
	ADDRINT		pfn 	= 0;
	ADDRINT		paddr 	= 0;
	uint64_t 	entry;
	uint64_t 	offset = (vaddr/getpagesize()*8);
	char 		byte_content;
	int 		again_cnt = 0;

again:
	if(!(pm_file = fopen("/proc/self/pagemap","rb")))
	{
		cout<<" Error, cannot open pagemap."<<endl;
	}

	if(fseek(pm_file,offset,SEEK_SET))
	{
		cout<<" Error, failed to fseek."<<endl;
	}

	if(fread(&entry,sizeof(uint64_t),1,pm_file))
	{
		//Get page map success
		if(GET_BIT(entry,63))
		{
			pfn = GET_PFN(entry);
		}
		else//Get page map fail
		{
			//cout<<" Warning, page not present 0x"<<hex<<vaddr<<endl;

			if(again_cnt == 0)
			{
				g_page_miss_cnt++;
			}
			
			again_cnt++;

			//Try N times, all fail, not return physical address, only return 0x0
			if(again_cnt >= 10)
			{
				g_page_ignore_cnt++;
				fclose(pm_file);

				return 0x0;
			}
			else
			{
				PIN_SafeCopy((VOID*)&byte_content,(VOID*)vaddr,sizeof(byte_content));
				fclose(pm_file);
				
				goto again;
			}

		}
	}

	fclose(pm_file);

	paddr = pfn*getpagesize()+GET_PAGEOFFSET(vaddr);

	return paddr;
}


/**
* Command line option to specify the name of the output file.
* Default is shellcode.out.
**/
//KNOB<string> outputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "shellcode.out", "specify trace file name");
/**
* Prints usage information.
**/
INT32 usage()
{
	cerr << "This tool produces cache simulation trace." << endl << endl;
	cerr << KNOB_BASE::StringKnobSummary() << endl;
	return -1;
}


/**
* Converts a PIN instruction object into a disassembled string.
**/
ADDRINT getFetchAddr(INS ins)
{
	ADDRINT t_addr = INS_Address(ins);
	
	if(g_phy_en)
	{
		t_addr = virtualToPhysical(t_addr);
	}
	
	return t_addr;
}
/**
* Callback function that is executed every time an instruction is identified
*  executed.
**/
/**
 * Record instruction do not access memory
**/
void RecordNoMem(	ADDRINT	fetch_addr,
			THREADID thread_id
		)
{
	g_inst_cnt++;

	Inst	t_inst;
	
	t_inst.m_rd_en		= false;
	t_inst.m_wr_en		= false;
	t_inst.m_fetch_addr	= fetch_addr;

	while(g_shared[g_shared_num]->m_ready)
	{
		//usleep(1);
		g_tmp_data = 0;
	}

	g_shared[g_shared_num]->m_inst[g_shared[g_shared_num]->m_inst_cnt] = t_inst;
	g_shared[g_shared_num]->m_inst_cnt++;

	if(g_shared[g_shared_num]->m_inst_cnt >= BUF_INST_NUM)
	{
		g_shared[g_shared_num]->m_ready = true;

		g_shared_num++;
		g_shared_num %= PIN_SC_SHARED_NUM;
	}
}

/**
 * Record instruction do both memory read and memory write
**/
void RecordMemReadWrite(ADDRINT fetch_addr,
			ADDRINT rd_addr,
			ADDRINT wr_addr,
			THREADID thread_id
			)
{
	g_inst_cnt++;

	Inst	t_inst;
	
	t_inst.m_rd_en		= true;
	t_inst.m_wr_en		= true;
	t_inst.m_fetch_addr	= fetch_addr;
	if(g_phy_en)
	{
		t_inst.m_rd_addr	= virtualToPhysical(rd_addr);
		t_inst.m_wr_addr	= virtualToPhysical(wr_addr);
	}
	else
	{
		t_inst.m_rd_addr	= rd_addr;
		t_inst.m_wr_addr	= wr_addr;
	}

	while(g_shared[g_shared_num]->m_ready)
	{
		//usleep(1);
		g_tmp_data = 0;
	}

	g_shared[g_shared_num]->m_inst[g_shared[g_shared_num]->m_inst_cnt] = t_inst;
	g_shared[g_shared_num]->m_inst_cnt++;

	if(g_shared[g_shared_num]->m_inst_cnt >= BUF_INST_NUM)
	{
		g_shared[g_shared_num]->m_ready = true;

		g_shared_num++;
		g_shared_num %= PIN_SC_SHARED_NUM;
	}
}
/**
 * Record instruction do memory read
**/
void RecordMemRead(	ADDRINT fetch_addr,
			ADDRINT rd_addr,
			THREADID thread_id)
{
	g_inst_cnt++;

	Inst	t_inst;
	
	t_inst.m_rd_en		= true;
	t_inst.m_wr_en		= false;
	t_inst.m_fetch_addr	= fetch_addr;
	if(g_phy_en)
	{
		t_inst.m_rd_addr	= virtualToPhysical(rd_addr);
	}
	else
	{
		t_inst.m_rd_addr	= rd_addr;
	}

	while(g_shared[g_shared_num]->m_ready)
	{
		//usleep(1);
		g_tmp_data = 0;
	}

	g_shared[g_shared_num]->m_inst[g_shared[g_shared_num]->m_inst_cnt] = t_inst;
	g_shared[g_shared_num]->m_inst_cnt++;

	if(g_shared[g_shared_num]->m_inst_cnt >= BUF_INST_NUM)
	{
		g_shared[g_shared_num]->m_ready = true;

		g_shared_num++;
		g_shared_num %= PIN_SC_SHARED_NUM;
	}
}
/**
 * Record instrucion do memory write
**/
void RecordMemWrite(	ADDRINT fetch_addr,
			ADDRINT wr_addr,
			THREADID thread_id)
{
	g_inst_cnt++;

	Inst	t_inst;
	
	t_inst.m_rd_en		= false;
	t_inst.m_wr_en		= true;
	t_inst.m_fetch_addr	= fetch_addr;
	if(g_phy_en)
	{
		t_inst.m_wr_addr	= virtualToPhysical(wr_addr);
	}
	else
	{
		t_inst.m_wr_addr	= wr_addr;
	}

	while(g_shared[g_shared_num]->m_ready)
	{
		//usleep(1);
		g_tmp_data = 0;
	}

	g_shared[g_shared_num]->m_inst[g_shared[g_shared_num]->m_inst_cnt] = t_inst;
	g_shared[g_shared_num]->m_inst_cnt++;

	if(g_shared[g_shared_num]->m_inst_cnt >= BUF_INST_NUM)
	{
		g_shared[g_shared_num]->m_ready = true;

		g_shared_num++;
		g_shared_num %= PIN_SC_SHARED_NUM;
	}
}

/**
 * error execute
**/
void RecordInvalid(	ADDRINT fetch_addr,
			THREADID threadid)
{
	cout<<" Error, inst 0x"<<hex<<fetch_addr<<" record fail."<<endl;
}

/**
* Finalizer function that is called at the end of the trace process.
* In this script, the finalizer function is responsible for closing
* the shellcode output file.
**/
VOID fini(INT32, VOID*)
{
	while(g_shared[g_shared_num]->m_ready)
	{
		//usleep(1);
		g_tmp_data = 0;
	}

	g_shared[g_shared_num]->m_pin_complete = true;
	g_shared[g_shared_num]->m_ready = true;

	//
	for(int i=0;i<PIN_SC_SHARED_NUM;i++)
	{
		if(shmdt((void*)g_shared[i]) == -1)
		{
			cout<<" Pin shmdt fail"<<endl;
			exit(EXIT_FAILURE);
		}
	}
	cout<<"------------ END ----------------"<<endl;
	cout<<"* Page miss count:   "<<dec<<g_page_miss_cnt<<endl;
	cout<<"* Page ignore count: "<<dec<<g_page_ignore_cnt<<endl;
	cout<<"---------------------------------"<<endl;
}

/**
* This function is called
**/
void traceInst(INS ins, VOID*)
{
	UINT32 memOperands = INS_MemoryOperandCount(ins);
        
	if(!memOperands)
	{
		INS_InsertCall(	ins,
				IPOINT_BEFORE,
				AFUNPTR(RecordNoMem),
				IARG_PTR,
				getFetchAddr(ins), 
				IARG_THREAD_ID,
				IARG_END
                     		);
	}
	else
	{
		if(INS_IsMemoryRead(ins)&&INS_IsMemoryWrite(ins))
		{
			INS_InsertPredicatedCall(	ins,
							IPOINT_BEFORE,
							(AFUNPTR)RecordMemReadWrite,
							IARG_PTR,
							getFetchAddr(ins),
							IARG_MEMORYREAD_EA,
							IARG_MEMORYWRITE_EA,
							IARG_THREAD_ID,
							IARG_END
						);
		}
		else
		{
			if (INS_IsMemoryRead(ins))
			{
				INS_InsertPredicatedCall(	ins,
								IPOINT_BEFORE,
								(AFUNPTR)RecordMemRead,
								IARG_PTR,
								getFetchAddr(ins),
								IARG_MEMORYREAD_EA,
								IARG_THREAD_ID,
								IARG_END
							);
			}
			else if (INS_IsMemoryWrite(ins))
			{
				INS_InsertPredicatedCall(	ins,
								IPOINT_BEFORE,
								(AFUNPTR)RecordMemWrite,
								IARG_PTR,
								getFetchAddr(ins),
								IARG_MEMORYWRITE_EA,
								IARG_THREAD_ID,
								IARG_END
							);
			}
			else
			{
				INS_InsertCall(			ins,
								IPOINT_BEFORE,
								AFUNPTR(RecordInvalid),
								IARG_PTR,
								getFetchAddr(ins),
								IARG_THREAD_ID,
								IARG_END
							);
			}
		}
	}
}




int main(int argc, char *argv[])
{
	PIN_InitSymbols();
	if( PIN_Init(argc, argv))
	{
		return usage();
	}

	g_phy_en = KnobPhyAddr;


	cout<<"------------ START ----------------"<<endl;
	//
	for(int i=0;i<PIN_SC_SHARED_NUM;i++)
	{
		int t_key = KnobShareKey+i;
		void *shm = NULL;
		int shmid;
	
		cout<<" key id "<<dec<<t_key<<endl;

		shmid = shmget((key_t)t_key,sizeof(Exchange),0666|IPC_CREAT);
		if(shmid == -1)
		{
			cout<<" Pin create share memory failed"<<endl;
			exit(EXIT_FAILURE);
		}

		shm = shmat(shmid,(void*)0,0);
		if(shm == (void*)-1)
		{
			cout<<" Pin map share memory failed"<<endl;
			exit(EXIT_FAILURE);
		}
		g_shared[i] = (Exchange*)shm;

		g_shared[i]->m_ready 		= false;
		g_shared[i]->m_pin_complete	= false;
		g_shared[i]->m_inst_cnt 	= 0;
	}
	//

	INS_AddInstrumentFunction(traceInst, 0);
	PIN_AddFiniFunction(fini, 0);

	// Never returns
	PIN_StartProgram();

	return 0;
}
