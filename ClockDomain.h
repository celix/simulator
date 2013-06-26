#ifndef CLOCKDOMAIN_H
#define CLOCKDOMAIN_H

#include <iostream>
#include <cmath>
#include <stdint.h>
#include "Callback.h"


namespace DRAMSim
{

	class ClockDomain
	{
	public:
		uint64_t clockcycle;
		ClockUpdateCB *callback;
		uint64_t clock;
		uint64_t counter;
		ClockDomain *previousDomain;
		ClockDomain *nextDomain;

		ClockDomain(ClockUpdateCB *callback, uint64_t clock = 0);

		void tick();
	};
}

#endif
