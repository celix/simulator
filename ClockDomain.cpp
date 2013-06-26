#include "ClockDomain.h"
#include "Callback.h"

using namespace std;



namespace DRAMSim
{

	ClockDomain::ClockDomain(ClockUpdateCB *callback, uint64_t clock) :
			callback(callback),
			clock(clock),
			clockcycle(0),
			counter(0),
			previousDomain(NULL),
			nextDomain(NULL){};


	void ClockDomain::tick()
	{
		// update current clock domain
		(*callback)();

		// update counter
		counter += clock;


		if (nextDomain == NULL)						// no more next Clock Domain
		{
			clockcycle++;
			return;
		}
		else if (clock == nextDomain->clock)		//short circuit case for 1:1 ratios
		{
			nextDomain->tick();
			clockcycle++;
		}
		else
		{
			while (counter > nextDomain->counter)
			{
				nextDomain->counter += nextDomain->clock;
				nextDomain->tick();
			}
			clockcycle++;
		}

		// reset counters when all clock domain counters are equal
		if (previousDomain == NULL && nextDomain != NULL)
		{
			bool counterReset = true;
			ClockDomain *p;

			for (p=this; p->nextDomain != NULL; p = p->nextDomain)
			{
				if (p->counter != p->nextDomain->counter)
				{
					counterReset = false;
					break;
				}
			}
			if (counterReset)
			{
				for (p=this; p != NULL; p = p->nextDomain)
				{
					p->counter = 0;
				}
			}
		}
	}

} // end of namespace DRAMSim
