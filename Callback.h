/*********************************************************************************
*  Copyright (c) 2010-2011, Elliott Cooper-Balis
*                             Paul Rosenfeld
*                             Bruce Jacob
*                             University of Maryland 
*                             dramninjas [at] gmail [dot] com
*  All rights reserved.
*  
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are met:
*  
*     * Redistributions of source code must retain the above copyright notice,
*        this list of conditions and the following disclaimer.
*  
*     * Redistributions in binary form must reproduce the above copyright notice,
*        this list of conditions and the following disclaimer in the documentation
*        and/or other materials provided with the distribution.
*  
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
*  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
*  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
*  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
*  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
*  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
*  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
*  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*********************************************************************************/



#include <stdint.h> // uint64_t

#ifndef CALLBACK_H
#define CALLBACK_H

namespace DRAMSim
{

	template <typename ReturnT>
	class CallbackBaseP0
	{
		public:
		virtual ReturnT operator()() = 0;
		virtual ~CallbackBaseP0() {};
	};


	template <typename ConsumerT, typename ReturnT>
	class CallbackP0: public CallbackBaseP0<ReturnT>
	{
	private:
		typedef ReturnT (ConsumerT::*PtrMember)();

		public:
		CallbackP0(ConsumerT* const object, PtrMember member) : object(object), member(member) {}

		CallbackP0(const CallbackP0<ConsumerT,ReturnT> &e) : object(e.object), member(e.member) {}

		ReturnT operator()()
		{
			return (const_cast<ConsumerT*>(object)->*member)();
		}

	private:
		ConsumerT* const object;
		const PtrMember  member;
	};


	template <typename ReturnT, typename Param1T, typename Param2T,	typename Param3T>
	class CallbackBaseP3
	{
	public:
		virtual ReturnT operator()(Param1T, Param2T, Param3T) = 0;
		virtual ~CallbackBaseP3() {};
	};

	template <typename ConsumerT, typename ReturnT,	typename Param1T, typename Param2T, typename Param3T >
	class CallbackP3: public CallbackBaseP3<ReturnT,Param1T,Param2T,Param3T>
	{
	private:
		typedef ReturnT (ConsumerT::*PtrMember)(Param1T,Param2T,Param3T);

	public:
		CallbackP3( ConsumerT* const object, PtrMember member) : object(object), member(member) {}

		CallbackP3( const CallbackP3<ConsumerT,ReturnT,Param1T,Param2T,Param3T>& e ) : object(e.object), member(e.member) {}

		ReturnT operator()(Param1T param1, Param2T param2, Param3T param3)
		{
			return (const_cast<ConsumerT*>(object)->*member)(param1,param2,param3);
		}

	private:
		ConsumerT* const object;
		const PtrMember  member;
	};


	typedef CallbackBaseP0<void> ClockUpdateCB;
	typedef CallbackBaseP3<void, unsigned, uint64_t, uint64_t> TransactionCompleteCB;

	typedef void (*ReturnCB)(unsigned id, uint64_t addr, uint64_t clockcycle);
	typedef void (*PowerCB)(double bgpower, double burstpower, double refreshpower, double actprepower);
} // namespace DRAMSim

#endif
