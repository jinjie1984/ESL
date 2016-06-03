#define _FILE_OFFSET_BITS 64
#define _LARGEFILE64_SOURCE

#include "pin.H"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <set>
#include <list>
#include <sstream>
#include <sched.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>

#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <string.h>
#include <pthread.h>

#define PIN_SC_SHARED_NUM 2
#define BUF_INST_NUM 10000

KNOB<int> KnobShareKey(KNOB_MODE_APPEND,"pintool","key","","share memory key");
KNOB<string> KnobSliceStart(KNOB_MODE_WRITEONCE,"pintool","inst","0","slice start instruction number");
KNOB<string> KnobSliceLen(KNOB_MODE_WRITEONCE,"pintool","len","0","slice length");
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
int				g_push_shm_id		= 0;
uint64_t		g_page_miss_cnt		= 0;
uint64_t		g_page_ignore_cnt	= 0;
uint64_t		g_inst_cnt			= 0;

bool			g_start_slice		= false;
uint64_t		g_slice_start		= 0;
uint64_t		g_slice_len			= 0;

int				g_tmp_data			= 0;
uint64_t		g_cr3;
int				g_pm_fd = -1;

//---------------Virtual to Physical----------------------------
static uint64_t get_cr3_reg_info()
{
	char*		buf;
    int			rd_size;
	uint64_t	cr3_value;

    int fd = open("/proc/sys_reg", O_RDONLY);
    if(fd < 0){
        printf("open failed\n");
        return 0;
    }

    buf = (char*)malloc(100);
    if(buf == NULL){
        printf("malloc failed\n");
        return 0;
    }

    rd_size = read(fd, buf, 100 - 1);
    if(rd_size < 0){
        printf("read failed\n");
        return 0;
    }

    buf[rd_size] = 0;

    //printf("%s", buf);

	sscanf(buf,"%lx",&cr3_value);

	free(buf);

    close(fd);

	return cr3_value;
}


static uint64_t read_mem_dword(int fd, uint64_t addr)//8 bytes
{
	addr = addr & 0xFFFFFFFFFFFFFFF8L;

	if(addr >    0x0fffffffffffffffL)
	{
		while(1){usleep(1);}
	}

	__off64_t off = lseek64(fd, (__off64_t)addr, SEEK_SET);
	if(off < 0)
	{
        cout<<"lseek fail, addr 0x"<<hex<<addr<<endl;
        return 0;
    }

	uint64_t data;
	int read_size = read(fd, (char*)&data, 8);
	if(read_size < 0)
	{
		cout<<"read failed, addr 0x"<<hex<<addr<<endl;
    	return 0;
    }

	return data;
}

static uint64_t virtualToPhysical(int fd, uint64_t cr3, uint64_t vir_addr)
{
	uint64_t map_addr,page_addr,phy_addr;
	//std::stringstream t_ss;

	//t_ss<<" CR3 = 0x"<<hex<<cr3<<endl;
	//t_ss<<" Virtual addr = 0x"<<hex<<vir_addr<<endl;
	//1 level
	map_addr = (vir_addr>>39)&0x1ff;
	page_addr = cr3 + map_addr*8;
	//t_ss<<" 1.1 addr = 0x"<<hex<<page_addr<<endl;

	//if(page_addr > 0x0fffffffffffffffL)
	//{
	//	cout<<" EEEEEEEEEEEEEEEEEE"<<endl;
	//	//return 0x0;
	//	cout<<t_ss.str()<<endl;
	//	while(1){usleep(1);}
	//}
	page_addr = read_mem_dword(fd,page_addr);
	//t_ss<<" 1.2 addr = 0x"<<hex<<page_addr<<endl;
	
	page_addr &= 0x7FFFFFFFFFFFFF00;
	//t_ss<<" 1.3 addr = 0x"<<hex<<page_addr<<endl;

	//2 level
	map_addr = (vir_addr>>30)&0x1ff;
	page_addr = page_addr + map_addr*8;
	//t_ss<<" 2.1 addr = 0x"<<hex<<page_addr<<endl;

	//if(page_addr > 0x0fffffffffffffffL)
	//{
	//	cout<<" EEEEEEEEEEEEEEEEEE"<<endl;
	//	//return 0x0;
	//	cout<<t_ss.str()<<endl;
	//	while(1){usleep(1);}
	//}
	page_addr = read_mem_dword(fd,page_addr);
	//t_ss<<" 2.2 addr = 0x"<<hex<<page_addr<<endl;
	
	page_addr &= 0x7FFFFFFFFFFFFF00;
	//t_ss<<" 2.3 addr = 0x"<<hex<<page_addr<<endl;

	//3 level
	map_addr = (vir_addr>>21)&0x1ff;
	page_addr = page_addr + map_addr*8;
	//t_ss<<" 3.1 addr = 0x"<<hex<<page_addr<<endl;

	//if(page_addr > 0x0fffffffffffffffL)
	//{
	//	cout<<" EEEEEEEEEEEEEEEEEE"<<endl;
	//	//return 0x0;
	//	cout<<t_ss.str()<<endl;
	//	while(1){usleep(1);}
	//}
	page_addr = read_mem_dword(fd,page_addr);
	//t_ss<<" 3.2 addr = 0x"<<hex<<page_addr<<endl;
	
	page_addr &= 0x7FFFFFFFFFFFFF00;
	//t_ss<<" 3.3 addr = 0x"<<hex<<page_addr<<endl;

	//4 level
	map_addr = (vir_addr>>12)&0x1ff;
	page_addr = page_addr + map_addr*8;
	//t_ss<<" 4.1 addr = 0x"<<hex<<page_addr<<endl;

	//if(page_addr > 0x0fffffffffffffffL)
	//{
	//	cout<<" EEEEEEEEEEEEEEEEEE"<<endl;
	//	//return 0x0;
	//	cout<<t_ss.str()<<endl;
	//	while(1){usleep(1);}
	//}
	page_addr = read_mem_dword(fd,page_addr);
	//t_ss<<" 4.2 addr = 0x"<<hex<<page_addr<<endl;
	
	page_addr &= 0x7FFFFFFFFFFFF000;
	//t_ss<<" 4.3 addr = 0x"<<hex<<page_addr<<endl;

	//phy addr
	phy_addr = page_addr + (vir_addr&0xFFF);

	//t_ss<<" Physical addr = 0x"<<hex<<phy_addr<<endl;

	return phy_addr;
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
	//t_addr = virtualToPhysical(g_pm_fd,g_cr3,t_addr);
	return t_addr;
}



void PushInstToShareMem(Inst& inst)
{
	while(g_shared[g_push_shm_id]->m_ready)
	{
		//usleep(1);
		g_tmp_data = 0;
	}

	g_shared[g_push_shm_id]->m_inst[g_shared[g_push_shm_id]->m_inst_cnt] = inst;
	g_shared[g_push_shm_id]->m_inst_cnt++;

	if(g_shared[g_push_shm_id]->m_inst_cnt >= BUF_INST_NUM)
	{
		//-- virtual address to physical address
		for(int i=0;i<BUF_INST_NUM;i++)
		{
			g_shared[g_push_shm_id]->m_inst[i].m_fetch_addr = 
				virtualToPhysical(	g_pm_fd,
									g_cr3,
									g_shared[g_push_shm_id]->m_inst[i].m_fetch_addr);

			if(g_shared[g_push_shm_id]->m_inst[i].m_rd_en)
			{
				g_shared[g_push_shm_id]->m_inst[i].m_rd_addr = 
					virtualToPhysical(	g_pm_fd,
										g_cr3,
										g_shared[g_push_shm_id]->m_inst[i].m_rd_addr);
			}

			if(g_shared[g_push_shm_id]->m_inst[i].m_wr_en)
			{
				g_shared[g_push_shm_id]->m_inst[i].m_wr_addr = 
					virtualToPhysical(	g_pm_fd,
										g_cr3,
										g_shared[g_push_shm_id]->m_inst[i].m_wr_addr);
			}
		}
		//-- address convert complete

		//
		g_shared[g_push_shm_id]->m_ready = true;

		g_push_shm_id++;
		g_push_shm_id %= PIN_SC_SHARED_NUM;
	}
}

void CompleteShareMem()
{
	unsigned int t_inst_cnt = g_shared[g_push_shm_id]->m_inst_cnt;

	while(g_shared[g_push_shm_id]->m_ready)
	{
		//usleep(1);
		g_tmp_data = 0;
	}

	for(unsigned int i=0;i<t_inst_cnt;i++)
	{
		g_shared[g_push_shm_id]->m_inst[i].m_fetch_addr = 
			virtualToPhysical(	g_pm_fd,
								g_cr3,
								g_shared[g_push_shm_id]->m_inst[i].m_fetch_addr);

		if(g_shared[g_push_shm_id]->m_inst[i].m_rd_en)
		{
			g_shared[g_push_shm_id]->m_inst[i].m_rd_addr = 
				virtualToPhysical(	g_pm_fd,
									g_cr3,
									g_shared[g_push_shm_id]->m_inst[i].m_rd_addr);
		}

		if(g_shared[g_push_shm_id]->m_inst[i].m_wr_en)
		{
			g_shared[g_push_shm_id]->m_inst[i].m_wr_addr = 
				virtualToPhysical(	g_pm_fd,
									g_cr3,
									g_shared[g_push_shm_id]->m_inst[i].m_wr_addr);
		}
	}

	g_shared[g_push_shm_id]->m_pin_complete = true;
	g_shared[g_push_shm_id]->m_ready = true;
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

	if(g_slice_len != 0)
	{
		if(g_inst_cnt > (g_slice_start+g_slice_len))
		{
			fini(0,NULL);
			cout<<" Complete slice to "<<dec<<g_inst_cnt<<endl;
			exit(EXIT_FAILURE);
		}

		if(g_inst_cnt < g_slice_start) 
		{
			return;
		}
	}

	if(!g_start_slice)
	{
		cout<<" Start slice from "<<dec<<g_inst_cnt<<endl;
		g_start_slice = true;
	}

	Inst	t_inst;
	
	t_inst.m_rd_en		= false;
	t_inst.m_wr_en		= false;
	t_inst.m_fetch_addr	= fetch_addr;

	PushInstToShareMem(t_inst);
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

	if(g_slice_len != 0)
	{
		if(g_inst_cnt > (g_slice_start+g_slice_len))
		{
			fini(0,NULL);
			cout<<" Complete slice to "<<dec<<g_inst_cnt<<endl;
			exit(EXIT_FAILURE);
		}

		if(g_inst_cnt < g_slice_start) 
		{
			return;
		}
	}

	if(!g_start_slice)
	{
		cout<<" Start slice from "<<dec<<g_inst_cnt<<endl;
		g_start_slice = true;
	}


	Inst	t_inst;
	
	t_inst.m_rd_en		= true;
	t_inst.m_wr_en		= true;
	t_inst.m_fetch_addr	= fetch_addr;
	//t_inst.m_rd_addr	= virtualToPhysical(g_pm_fd,g_cr3,rd_addr);
	//t_inst.m_wr_addr	= virtualToPhysical(g_pm_fd,g_cr3,wr_addr);
	t_inst.m_rd_addr	= rd_addr;
	t_inst.m_wr_addr	= wr_addr;

	PushInstToShareMem(t_inst);
}
/**
 * Record instruction do memory read
**/
void RecordMemRead(	ADDRINT fetch_addr,
			ADDRINT rd_addr,
			THREADID thread_id)
{
	g_inst_cnt++;

	if(g_slice_len != 0)
	{
		if(g_inst_cnt > (g_slice_start+g_slice_len))
		{
			fini(0,NULL);
			cout<<" Complete slice to "<<dec<<g_inst_cnt<<endl;
			exit(EXIT_FAILURE);
		}

		if(g_inst_cnt < g_slice_start) 
		{
			return;
		}
	}

	if(!g_start_slice)
	{
		cout<<" Start slice from "<<dec<<g_inst_cnt<<endl;
		g_start_slice = true;
	}


	Inst	t_inst;
	
	t_inst.m_rd_en		= true;
	t_inst.m_wr_en		= false;
	t_inst.m_fetch_addr	= fetch_addr;
	//t_inst.m_rd_addr	= virtualToPhysical(g_pm_fd,g_cr3,rd_addr);
	t_inst.m_rd_addr	= rd_addr;

	PushInstToShareMem(t_inst);
}
/**
 * Record instrucion do memory write
**/
void RecordMemWrite(	ADDRINT fetch_addr,
			ADDRINT wr_addr,
			THREADID thread_id)
{
	g_inst_cnt++;

	if(g_slice_len != 0)
	{
		if(g_inst_cnt > (g_slice_start+g_slice_len))
		{
			fini(0,NULL);
			cout<<" Complete slice to "<<dec<<g_inst_cnt<<endl;
			exit(EXIT_FAILURE);
		}

		if(g_inst_cnt < g_slice_start) 
		{
			return;
		}
	}

	if(!g_start_slice)
	{
		cout<<" Start slice from "<<dec<<g_inst_cnt<<endl;
		g_start_slice = true;
	}


	Inst	t_inst;
	
	t_inst.m_rd_en		= false;
	t_inst.m_wr_en		= true;
	t_inst.m_fetch_addr	= fetch_addr;
	//t_inst.m_wr_addr	= virtualToPhysical(g_pm_fd,g_cr3,wr_addr);
	t_inst.m_wr_addr	= wr_addr;

	PushInstToShareMem(t_inst);
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
	CompleteShareMem();

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

	close(g_pm_fd);
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

	if(KnobShareKey.NumberOfValues() != PIN_SC_SHARED_NUM)
	{
		cout<<" Number of key value must be "<<(PIN_SC_SHARED_NUM)<<endl;
		exit(EXIT_FAILURE);
	}

	string t_str = KnobSliceStart;
	sscanf(t_str.c_str(),"%ld",&g_slice_start);
	
	t_str = KnobSliceLen;
	sscanf(t_str.c_str(),"%ld",&g_slice_len);

	cout<<"------------ START ----------------"<<endl;
	//
	for(int i=0;i<PIN_SC_SHARED_NUM;i++)
	{
		int t_key = KnobShareKey.Value(i);
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
	if(g_pm_fd<0)
	{
		g_cr3 = get_cr3_reg_info();

		g_pm_fd = open("/dev/phy_mem", O_RDONLY);
		if(g_pm_fd<0)
		{
			cout<<" Error, phy_mem open fail."<<endl;
			exit(-1);
		}

		cout<<" CR3 = 0x"<<hex<<g_cr3<<endl;
	}

	INS_AddInstrumentFunction(traceInst, 0);
	PIN_AddFiniFunction(fini, 0);

	// Never returns
	PIN_StartProgram();

	return 0;
}
