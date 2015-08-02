#ifndef __KOBJ_H_
#define __KOBJ_H_

typedef struct KProcess KProcess;

typedef struct KMemInfo
{
	void* addr;
	uint32_t total_pages;
	uint32_t binfo_count;		// KBlockInfo count for section
	void* binfo_first;			// Pointer to KLinkedListNode that holds a pointer to the first KBlockInfo object for that section
	void* binfo_last;			// Pointer to KLinkedListNode that holds a pointer to the last KBlockInfo object for that section
} KMemInfo;

typedef struct __attribute__((__packed__)) KCodeSet
{
	void* vtable;
	uint32_t ref_count;
	KMemInfo text;
	KMemInfo rodata;
	KMemInfo data;
	uint32_t text_pages;
	uint32_t ro_pages;
	uint32_t rw_pages;
	uint32_t namelo;
	uint32_t namehi;
	uint32_t unk;
	uint32_t titleid[2];
} KCodeSet;

typedef struct KLinkedListNode KLinkedListNode;

typedef struct KLinkedListNode
{
	KLinkedListNode* next;
	KLinkedListNode* prev;
	void* node;
} KLinkedListNode;

typedef struct KDebugThread
{

} KDebugThread;

typedef struct KThread KThread;
typedef struct KThread
{
	void* vtable;
	uint32_t ref_count;
	uint32_t node_count;
	KLinkedListNode* first_thread;
	KLinkedListNode* last_thread;

	uint32_t unk0[9];
	KDebugThread* debug_thread;
	uint32_t base_thread_priority;

	void* wait_obj;
	uint32_t pad0[2];
	void* arb_addr;

	uint32_t pad1[4];

	uint32_t mutex_count;
	KLinkedListNode* first_mutex;
	KLinkedListNode* last_mutex;
	int32_t dynamic_thread_priority;

	int32_t creator_cpu;
	uint32_t pad2[3];

	KProcess* parent;
	uint32_t thread_id;
	void* svc_reg_storage;
	void* page_end;

	int32_t ideal_cpu;
	void* tls;
	void* tls2;
	uint32_t pad3;

	KThread* prev;
	KThread* next;
	void* owner;
} KThread;

typedef struct KHandleData
{
	uint32_t handle_info;
	void* ptr;
} KHandleData;

typedef struct KProcessHandleTable
{
	KHandleData* data;
	uint16_t max_handles;
	uint16_t highest_usage; // The highest count of handles that have been open at once
	KHandleData* next_open;
	uint16_t total_handles;
	uint16_t total_in_use; // ?

	KHandleData table[0x28];
} KProcessHandleTable;

// http://www.3dbrew.org/wiki/KProcess
typedef struct KProcess_4
{
	void* vtable;
	uint32_t ref_count;
	uint32_t node_count;
	KLinkedListNode* first_thread;

	KLinkedListNode* last_thread;
	uint32_t pad0[2];
	KThread* owner; // 0 or pointer to the thread the currently does something with the process object.

	uint32_t pad1[4];

	KLinkedListNode* first_KMB;
	KLinkedListNode* last_KMB;
	uint32_t pad2[2];

	uint32_t trans_table_base;
	uint32_t context_id;
	uint32_t pad3[2];

	uint32_t mmu_table_size;// 0x50
	uint32_t mmu_table;		// 0x54
	uint32_t total_context_size; // Total size of all thread context pages (0xFF4xxxxx) owned by threads that belong to this process
	uint32_t thread_local_page_count; // Number of KThreadLocalPages used by this KProcess

	KLinkedListNode* first_KTLP;
	KLinkedListNode* last_KTLP;
	uint32_t pad4;
	int32_t ideal_processor;

	uint32_t pad5;
	void* limits;
	uint8_t pad6;
	uint8_t proc_affinity_mask;
	uint8_t pad7[2];
	uint32_t thread_count;

	uint32_t svc_access_mask[4];
	uint32_t interrupt_flags[4];

	uint32_t kernel_flags0;
	uint16_t handle_table_size;
	uint16_t kern_release_version;
	KCodeSet* kcodeset;		// 0xA8
	uint32_t pid;			// 0xAC

	uint32_t kernel_flags1;
	uint32_t pad8;
	KThread* main_thread;
	uint32_t pad9;

	uint32_t pad10[3];
	KProcessHandleTable table;
} KProcess_4;

typedef struct KProcess_8
{
	void* vtable;
	uint32_t ref_count;
	uint32_t node_count;
	KLinkedListNode* first_thread;

	KLinkedListNode* last_thread;
	uint32_t pad0[6];

	uint32_t KMB_count;

	KLinkedListNode* first_KMB;
	KLinkedListNode* last_KMB;
	uint32_t pad1[2];

	uint32_t trans_table_base;
	uint32_t context_id;
	uint32_t pad2;
	void* vmem_end;

	void* linear_start;
	uint32_t pad3;
	uint32_t mmu_table_size;// 0x58
	uint32_t mmu_table;		// 0x5C

	uint32_t total_context_size; // Total size of all thread context pages (0xFF4xxxxx) owned by threads that belong to this process
	uint32_t thread_local_page_count; // Number of KThreadLocalPages used by this KProcess
	KLinkedListNode* first_KTLP;
	KLinkedListNode* last_KTLP;

	uint32_t pad4;
	int32_t ideal_processor;
	uint32_t pad5;
	void* limits;

	uint32_t pad6;
	uint32_t thread_count;

	uint32_t svc_access_mask[4];
	uint32_t interrupt_available_flags[4];
	uint32_t kernel_flags;
	uint16_t handle_table_size;
	uint16_t kern_release_version;

	KCodeSet* kcodeset;		// 0xB0
	uint32_t pid;			// 0xB4
	uint32_t pad7[2];

	KThread* main_thread;
	uint32_t interrupt_enabled_flags[4];
	KProcessHandleTable table;
} KProcess_8;

typedef struct KServerPort KServerPort;
typedef struct KClientPort KClientPort;

typedef struct KPort
{
	void* vtable;
	uint32_t ref_count;
	KServerPort* server;
	uint32_t pad0[8];
	KClientPort* client;
} KPort;

typedef struct KServerPort
{
	void* vtable;
	uint32_t ref_count;
	uint32_t node_count;
	KLinkedListNode* first_thread;
	KLinkedListNode* last_thread;

	uint32_t session_count;
	KLinkedListNode* first_session;
	KLinkedListNode* last_session;
	KPort* parent;
} KServerPort;

typedef struct KClientPort
{
	void* vtable;
	uint32_t ref_count;
	uint32_t node_count;
	KLinkedListNode* first_thread;
	KLinkedListNode* last_thread;
	uint16_t conn_count;
	uint16_t max_conn;
	KPort* parent;
} KClientPort;

typedef struct KServerSession KServerSession;
typedef struct KClientSession KClientSession;

typedef struct KSession
{
	void* vtable;
	uint32_t ref_count;
	KServerSession* server;
	uint32_t pad0[8];
	KClientSession* client;
} KSession;

typedef struct KServerSession
{
	void* vtable;
	uint32_t ref_count;
	uint32_t node_count;
	KLinkedListNode* first_thread;
	KLinkedListNode* last_thread;
	KSession* parent;
	KThread* unk0;
	KThread* unk1;
	KThread* origin;
} KServerSession;

typedef struct KClientSession
{
	void* vtable;
	uint32_t ref_count;
	uint32_t node_count;
	KLinkedListNode* first_thread;
	KLinkedListNode* last_thread;
	KSession* parent;
	uint32_t status;
	KClientPort* port;
} KClientSession;

#endif
