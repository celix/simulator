#ifndef SIMULATOR_H_
#define SIMULATOR_H_

#include "SimulatorIO.h"
#include "ClockDomain.h"
#include "MemorySystem.h"
#include "CacheSimulator.h"

using BlSim::Caches;

namespace DRAMSim
{
	class Simulator
	{
	public:
		Simulator(SimulatorIO *simIO) : simIO(simIO),
		                                memorySystem(NULL),
		                                myCache(NULL),
		                                trans(NULL),
		                                pendingTrace(true) {};
		~Simulator();

		void setup();
		void start();
		void update();
		void report();

		static ClockDomain* clockDomainCPU;
		static ClockDomain* clockDomainDRAM;
		static ClockDomain* clockDomainTREE;

	private:
		void setCPUClock(uint64_t cpuClkFreqHz);
		void setClockRatio(double ratio);

		SimulatorIO *simIO;
		MemorySystem *memorySystem;
		Caches *myCache;
		Transaction *trans;

		bool pendingTrace;

#ifdef RETURN_TRANSACTIONS
		TransactionReceiver *transReceiver;
#endif

	};
}

#endif /* SIMULATOR_H_ */
