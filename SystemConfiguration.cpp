#include "SystemConfiguration.h"

namespace DRAMSim
{

	using namespace std;

	// device storage
	uint64_t TOTAL_STORAGE;

	unsigned NUM_CHANS;
	unsigned NUM_RANKS;
	unsigned NUM_BANKS;
	unsigned NUM_ROWS;
	unsigned NUM_COLS;

	unsigned NUM_DEVICES;
	unsigned DEVICE_WIDTH;

	unsigned TRANS_DATA_BYTES;
	unsigned SUBARRAY_DATA_BYTES;

	// device timing
	unsigned REFRESH_PERIOD;
	float tCK;
	float Vdd;
	unsigned CL;
	unsigned AL;
	unsigned BL;
	unsigned tRAS;
	unsigned tRCD;
	unsigned tRRD;
	unsigned tRC;
	unsigned tRP;
	unsigned tCCD;
	unsigned tRTP;
	unsigned tWTR;
	unsigned tWR;
	unsigned tRTRS;
	unsigned tRFC;
	unsigned tFAW;
	unsigned tCKE;
	unsigned tXP;
	unsigned tCMD;

	unsigned IDD0;
	unsigned IDD1;
	unsigned IDD2P;
	unsigned IDD2Q;
	unsigned IDD2N;
	unsigned IDD3Pf;
	unsigned IDD3Ps;
	unsigned IDD3N;
	unsigned IDD4W;
	unsigned IDD4R;
	unsigned IDD5;
	unsigned IDD6;
	unsigned IDD6L;
	unsigned IDD7;

	// memory system
	unsigned ECC_DATA_BUS_BITS;
	unsigned JEDEC_DATA_BUS_BITS;

	//Memory Controller parameters
	unsigned TRANS_QUEUE_DEPTH;
	unsigned CMD_QUEUE_DEPTH;

	//cycles within an epoch
	uint64_t EPOCH_LENGTH;
	unsigned HISTOGRAM_BIN_SIZE;

	//row accesses allowed before closing (open page)
	unsigned TOTAL_ROW_ACCESSES;

	// strings and their associated enums
	string ROW_BUFFER_POLICY;
	string SCHEDULING_POLICY;
	string ADDRESS_MAPPING_SCHEME;
	string QUEUING_STRUCTURE;

	RowBufferPolicy rowBufferPolicy;
	SchedulingPolicy schedulingPolicy;
	AddressMappingScheme addressMappingScheme;
	QueuingStructure queuingStructure;


	bool DEBUG_TRANS_Q;
	bool DEBUG_CMD_Q;
	bool DEBUG_ADDR_MAP;
	bool DEBUG_BANKSTATE;
	bool DEBUG_BUS;
	bool DEBUG_BANKS;
	bool DEBUG_POWER;
	bool USE_LOW_POWER;
	bool VIS_FILE_OUTPUT;

	bool VERIFICATION_OUTPUT;

	bool DEBUG_INI_READER = false;
	bool SHOW_SIM_OUTPUT = true;

}
