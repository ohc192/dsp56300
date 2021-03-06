// DSP 56300 family 24-bit DSP emulator

#include "dsp.h"

#include <iomanip>

#include "registers.h"
#include "types.h"
#include "memory.h"
#include "error.h"
#include "agu.h"
#include "disasm.h"
#include "opcodes.h"

#include "dsp_ops.inl"

#if 0
#	define LOGSC(F)	logSC(F)
#else
#	define LOGSC(F)	{}
#endif

//

namespace dsp56k
{
	constexpr bool g_dumpPC = false;
//	constexpr TWord g_dumpPCictrMin = 0x153000;
	constexpr TWord g_dumpPCictrMin = 0x0;
	constexpr TWord g_dumpPCictrMax = 0x10d00;

	using TInstructionFunc = void (DSP::*)(TWord op);

	constexpr TInstructionFunc g_jumpTable[InstructionCount] =
	{
		&DSP::op_Abs,							// Abs Abs 
		&DSP::op_ADC,							// ADC ADC 
		&DSP::op_Add_SD,						// Add_SD Add_SD 
		&DSP::op_Add_xx,						// Add_xx Add_xx 
		&DSP::op_Add_xxxx,						// Add_xxxx 
		&DSP::op_Addl,							// Addl 
		&DSP::op_Addr,							// Addr 
		&DSP::op_And_SD,						// And_SD 
		&DSP::op_And_xx,						// And_xx 
		&DSP::op_And_xxxx,						// And_xxxx 
		&DSP::op_Andi,							// Andi 
		&DSP::op_Asl_D,							// Asl_D 
		&DSP::op_Asl_ii,						// Asl_ii 
		&DSP::op_Asl_S1S2D,						// Asl_S1S2D 
		&DSP::op_Asr_D,							// Asr_D 
		&DSP::op_Asr_ii,						// Asr_ii 
		&DSP::op_Asr_S1S2D,						// Asr_S1S2D 
		&DSP::op_Bcc_xxxx,						// Bcc_xxxx 
		&DSP::op_Bcc_xxx,						// Bcc_xxx 
		&DSP::op_Bcc_Rn,						// Bcc_Rn 
		&DSP::op_Bchg_ea,						// Bchg_ea 
		&DSP::op_Bchg_aa,						// Bchg_aa 
		&DSP::op_Bchg_pp,						// Bchg_pp 
		&DSP::op_Bchg_qq,						// Bchg_qq 
		&DSP::op_Bchg_D,						// Bchg_D 
		&DSP::op_Bclr_ea,						// Bclr_ea 
		&DSP::op_Bclr_aa,						// Bclr_aa 
		&DSP::op_Bclr_pp,						// Bclr_pp 
		&DSP::op_Bclr_qq,						// Bclr_qq 
		&DSP::op_Bclr_D,						// Bclr_D 
		&DSP::op_Bra_xxxx,						// Bra_xxxx 
		&DSP::op_Bra_xxx,						// Bra_xxx 
		&DSP::op_Bra_Rn,						// Bra_Rn 
		&DSP::op_Brclr_ea,						// Brclr_ea 
		&DSP::op_Brclr_aa,						// Brclr_aa 
		&DSP::op_Brclr_pp,						// Brclr_pp 
		&DSP::op_Brclr_qq,						// Brclr_qq 
		&DSP::op_Brclr_S,						// Brclr_S 
		&DSP::op_BRKcc,							// BRKcc 
		&DSP::op_Brset_ea,						// Brset_ea 
		&DSP::op_Brset_aa,						// Brset_aa 
		&DSP::op_Brset_pp,						// Brset_pp 
		&DSP::op_Brset_qq,						// Brset_qq 
		&DSP::op_Brset_S,						// Brset_S 
		&DSP::op_BScc_xxxx,						// BScc_xxxx 
		&DSP::op_BScc_xxx,						// BScc_xxx 
		&DSP::op_BScc_Rn,						// BScc_Rn 
		&DSP::op_Bsclr_ea,						// Bsclr_ea 
		&DSP::op_Bsclr_aa,						// Bsclr_aa 
		&DSP::op_Bsclr_pp,						// Bsclr_pp 
		&DSP::op_Bsclr_qq,						// Bsclr_qq 
		&DSP::op_Bsclr_S,						// Bsclr_S 
		&DSP::op_Bset_ea,						// Bset_ea 
		&DSP::op_Bset_aa,						// Bset_aa 
		&DSP::op_Bset_pp,						// Bset_pp 
		&DSP::op_Bset_qq,						// Bset_qq 
		&DSP::op_Bset_D,						// Bset_D 
		&DSP::op_Bsr_xxxx,						// Bsr_xxxx 
		&DSP::op_Bsr_xxx,						// Bsr_xxx 
		&DSP::op_Bsr_Rn,						// Bsr_Rn 
		&DSP::op_Bsset_ea,						// Bsset_ea 
		&DSP::op_Bsset_aa,						// Bsset_aa 
		&DSP::op_Bsset_pp,						// Bsset_pp 
		&DSP::op_Bsset_qq,						// Bsset_qq 
		&DSP::op_Bsset_S,						// Bsset_S 
		&DSP::op_Btst_ea,						// Btst_ea 
		&DSP::op_Btst_aa,						// Btst_aa 
		&DSP::op_Btst_pp,						// Btst_pp 
		&DSP::op_Btst_qq,						// Btst_qq 
		&DSP::op_Btst_D,						// Btst_D 
		&DSP::op_Clb,							// Clb 
		&DSP::op_Clr,							// Clr 
		&DSP::op_Cmp_S1S2,						// Cmp_S1S2 
		&DSP::op_Cmp_xxS2,						// Cmp_xxS2 
		&DSP::op_Cmp_xxxxS2,					// Cmp_xxxxS2 
		&DSP::op_Cmpm_S1S2,						// Cmpm_S1S2 
		&DSP::op_Cmpu_S1S2,						// Cmpu_S1S2 
		&DSP::op_Debug,							// Debug 
		&DSP::op_Debugcc,						// Debugcc 
		&DSP::op_Dec,							// Dec 
		&DSP::op_Div,							// Div 
		&DSP::op_Dmac,							// Dmac 
		&DSP::op_Do_ea,							// Do_ea 
		&DSP::op_Do_aa,							// Do_aa 
		&DSP::op_Do_xxx,						// Do_xxx 
		&DSP::op_Do_S,							// Do_S 
		&DSP::op_DoForever,						// DoForever 
		&DSP::op_Dor_ea,						// Dor_ea 
		&DSP::op_Dor_aa,						// Dor_aa 
		&DSP::op_Dor_xxx,						// Dor_xxx 
		&DSP::op_Dor_S,							// Dor_S 
		&DSP::op_DorForever,					// DorForever 
		&DSP::op_Enddo,							// Enddo 
		&DSP::op_Eor_SD,						// Eor_SD 
		&DSP::op_Eor_xx,						// Eor_xx 
		&DSP::op_Eor_xxxx,						// Eor_xxxx 
		&DSP::op_Extract_S1S2,					// Extract_S1S2 
		&DSP::op_Extract_CoS2,					// Extract_CoS2 
		&DSP::op_Extractu_S1S2,					// Extractu_S1S2 
		&DSP::op_Extractu_CoS2,					// Extractu_CoS2 
		&DSP::op_Ifcc,							// Ifcc 
		&DSP::op_Ifcc_U,						// Ifcc_U 
		&DSP::op_Illegal,						// Illegal 
		&DSP::op_Inc,							// Inc 
		&DSP::op_Insert_S1S2,					// Insert_S1S2 
		&DSP::op_Insert_CoS2,					// Insert_CoS2 
		&DSP::op_Jcc_xxx,						// Jcc_xxx 
		&DSP::op_Jcc_ea,						// Jcc_ea 
		&DSP::op_Jclr_ea,						// Jclr_ea 
		&DSP::op_Jclr_aa,						// Jclr_aa 
		&DSP::op_Jclr_pp,						// Jclr_pp 
		&DSP::op_Jclr_qq,						// Jclr_qq 
		&DSP::op_Jclr_S,						// Jclr_S 
		&DSP::op_Jmp_ea,						// Jmp_ea 
		&DSP::op_Jmp_xxx,						// Jmp_xxx 
		&DSP::op_Jscc_xxx,						// Jscc_xxx 
		&DSP::op_Jscc_ea,						// Jscc_ea 
		&DSP::op_Jsclr_ea,						// Jsclr_ea 
		&DSP::op_Jsclr_aa,						// Jsclr_aa 
		&DSP::op_Jsclr_pp,						// Jsclr_pp 
		&DSP::op_Jsclr_qq,						// Jsclr_qq 
		&DSP::op_Jsclr_S,						// Jsclr_S 
		&DSP::op_Jset_ea,						// Jset_ea 
		&DSP::op_Jset_aa,						// Jset_aa 
		&DSP::op_Jset_pp,						// Jset_pp 
		&DSP::op_Jset_qq,						// Jset_qq 
		&DSP::op_Jset_S,						// Jset_S 
		&DSP::op_Jsr_ea,						// Jsr_ea 
		&DSP::op_Jsr_xxx,						// Jsr_xxx 
		&DSP::op_Jsset_ea,						// Jsset_ea 
		&DSP::op_Jsset_aa,						// Jsset_aa 
		&DSP::op_Jsset_pp,						// Jsset_pp 
		&DSP::op_Jsset_qq,						// Jsset_qq 
		&DSP::op_Jsset_S,						// Jsset_S 
		&DSP::op_Lra_Rn,						// Lra_Rn 
		&DSP::op_Lra_xxxx,						// Lra_xxxx 
		&DSP::op_Lsl_D,							// Lsl_D 
		&DSP::op_Lsl_ii,						// Lsl_ii 
		&DSP::op_Lsl_SD,						// Lsl_SD 
		&DSP::op_Lsr_D,							// Lsr_D 
		&DSP::op_Lsr_ii,						// Lsr_ii 
		&DSP::op_Lsr_SD,						// Lsr_SD 
		&DSP::op_Lua_ea,						// Lua_ea 
		&DSP::op_Lua_Rn,						// Lua_Rn 
		&DSP::op_Mac_S1S2,						// Mac_S1S2 
		&DSP::op_Mac_S,							// Mac_S 
		&DSP::op_Maci_xxxx,						// Maci_xxxx 
		&DSP::op_Macsu,							// Macsu 
		&DSP::op_Macr_S1S2,						// Macr_S1S2 
		&DSP::op_Macr_S,						// Macr_S 
		&DSP::op_Macri_xxxx,					// Macri_xxxx 
		&DSP::op_Max,							// Max 
		&DSP::op_Maxm,							// Maxm 
		&DSP::op_Merge,							// Merge 
		&DSP::op_Move_Nop,						// Move_Nop 
		&DSP::op_Move_xx,						// Move_xx 
		&DSP::op_Mover,							// Mover 
		&DSP::op_Move_ea,						// Move_ea 
		&DSP::op_Movex_ea,						// Movex_ea 
		&DSP::op_Movex_aa,						// Movex_aa 
		&DSP::op_Movex_Rnxxxx,					// Movex_Rnxxxx 
		&DSP::op_Movex_Rnxxx,					// Movex_Rnxxx 
		&DSP::op_Movexr_ea,						// Movexr_ea 
		&DSP::op_Movexr_A,						// Movexr_A 
		&DSP::op_Movey_ea,						// Movey_ea 
		&DSP::op_Movey_aa,						// Movey_aa 
		&DSP::op_Movey_Rnxxxx,					// Movey_Rnxxxx 
		&DSP::op_Movey_Rnxxx,					// Movey_Rnxxx 
		&DSP::op_Moveyr_ea,						// Moveyr_ea 
		&DSP::op_Moveyr_A,						// Moveyr_A 
		&DSP::op_Movel_ea,						// Movel_ea 
		&DSP::op_Movel_aa,						// Movel_aa 
		&DSP::op_Movexy,						// Movexy 
		&DSP::op_Movec_ea,						// Movec_ea 
		&DSP::op_Movec_aa,						// Movec_aa 
		&DSP::op_Movec_S1D2,					// Movec_S1D2 
		&DSP::op_Movec_xx,						// Movec_xx 
		&DSP::op_Movem_ea,						// Movem_ea 
		&DSP::op_Movem_aa,						// Movem_aa 
		&DSP::op_Movep_ppea,					// Movep_ppea 
		&DSP::op_Movep_Xqqea,					// Movep_Xqqea 
		&DSP::op_Movep_Yqqea,					// Movep_Yqqea 
		&DSP::op_Movep_eapp,					// Movep_eapp 
		&DSP::op_Movep_eaqq,					// Movep_eaqq 
		&DSP::op_Movep_Spp,						// Movep_Spp 
		&DSP::op_Movep_SXqq,					// Movep_SXqq 
		&DSP::op_Movep_SYqq,					// Movep_SYqq 
		&DSP::op_Mpy_S1S2D,						// Mpy_S1S2D 
		&DSP::op_Mpy_SD,						// Mpy_SD 
		&DSP::op_Mpy_su,						// Mpy_su 
		&DSP::op_Mpyi,							// Mpyi 
		&DSP::op_Mpyr_S1S2D,					// Mpyr_S1S2D 
		&DSP::op_Mpyr_SD,						// Mpyr_SD 
		&DSP::op_Mpyri,							// Mpyri 
		&DSP::op_Neg,							// Neg 
		&DSP::op_Nop,							// Nop 
		&DSP::op_Norm,							// Norm 
		&DSP::op_Normf,							// Normf 
		&DSP::op_Not,							// Not 
		&DSP::op_Or_SD,							// Or_SD 
		&DSP::op_Or_xx,							// Or_xx 
		&DSP::op_Or_xxxx,						// Or_xxxx 
		&DSP::op_Ori,							// Ori 
		&DSP::op_Pflush,						// Pflush 
		&DSP::op_Pflushun,						// Pflushun 
		&DSP::op_Pfree,							// Pfree 
		&DSP::op_Plock,							// Plock 
		&DSP::op_Plockr,						// Plockr 
		&DSP::op_Punlock,						// Punlock 
		&DSP::op_Punlockr,						// Punlockr 
		&DSP::op_Rep_ea,						// Rep_ea 
		&DSP::op_Rep_aa,						// Rep_aa 
		&DSP::op_Rep_xxx,						// Rep_xxx 
		&DSP::op_Rep_S,							// Rep_S 
		&DSP::op_Reset,							// Reset 
		&DSP::op_Rnd,							// Rnd 
		&DSP::op_Rol,							// Rol 
		&DSP::op_Ror,							// Ror 
		&DSP::op_Rti,							// Rti 
		&DSP::op_Rts,							// Rts 
		&DSP::op_Sbc,							// Sbc 
		&DSP::op_Stop,							// Stop 
		&DSP::op_Sub_SD,						// Sub_SD 
		&DSP::op_Sub_xx,						// Sub_xx 
		&DSP::op_Sub_xxxx,						// Sub_xxxx 
		&DSP::op_Subl,							// Subl 
		&DSP::op_subr,							// subr 
		&DSP::op_Tcc_S1D1,						// Tcc_S1D1 
		&DSP::op_Tcc_S1D1S2D2,					// Tcc_S1D1S2D2 
		&DSP::op_Tcc_S2D2,						// Tcc_S2D2 
		&DSP::op_Tfr,							// Tfr 
		&DSP::op_Trap,							// Trap 
		&DSP::op_Trapcc,						// Trapcc 
		&DSP::op_Tst,							// Tst 
		&DSP::op_Vsl,							// Vsl 
		&DSP::op_Wait,							// Wait 
		&DSP::op_ResolveCache,					// ResolveCache
		&DSP::op_Parallel						// Parallel
	};

	// _____________________________________________________________________________
	// DSP
	//
	DSP::DSP( Memory& _memory, IPeripherals* _pX, IPeripherals* _pY  )
		: mem(_memory)
		, perif({_pX, _pY})
		, pcCurrentInstruction(0xffffff)
	{
		mem.setDSP(this);
		perif[0]->setDSP(this);
		perif[1]->setDSP(this);

		m_asm[0] = 0;

		clearOpcodeCache();

		resetHW();
	}

	// _____________________________________________________________________________
	// resetHW
	//
	void DSP::resetHW()
	{
		// 100162AEd01.pdf - page 2-16

		// TODO: internal peripheral devices are reset
		perif[0]->reset();
		if(perif[1] != perif[0])
			perif[1]->reset();

		reg.m[0] = reg.m[1] = reg.m[2] = reg.m[3] = reg.m[4] = reg.m[5] = reg.m[6] = reg.m[7] = TReg24(int(0xffffff));
		reg.r[0] = reg.r[1] = reg.r[2] = reg.r[3] = reg.r[4] = reg.r[5] = reg.r[6] = reg.r[7] = TReg24(int(0));
		reg.n[0] = reg.n[1] = reg.n[2] = reg.n[3] = reg.n[4] = reg.n[5] = reg.n[6] = reg.n[7] = TReg24(int(0));

		iprc(0);
		iprp(0);

		// TODO: The Bus Control Register (BCR), the Address Attribute Registers (AAR3?AAR0) and the DRAM Control Register (DCR) are set to their initial values as described in Chapter 9, External Memory Interface (Port A). The initial value causes a maximum number of wait states to be added to every external memory access.

		reg.sp = TReg24(int(0));
		reg.sc = TReg5(char(0));

		reg.sz.var = 0xbadbad; // The SZ register is not initialized during hardware reset, and must be set, using a MOVEC instruction, prior to enabling the stack extension.

		const CCRMask srClear	= static_cast<CCRMask>(SR_RM | SR_SM | SR_CE | SR_SA | SR_FV | SR_LF | SR_DM | SR_SC | SR_S0 | SR_S1 | 0xf);
		const CCRMask srSet		= static_cast<CCRMask>(SR_CP0 | SR_CP1 | SR_I0 | SR_I1);

		sr_clear( srClear );
		sr_set	( srSet );

		// The Instruction Cache Controller is initialized as described in Chapter 8,	Instruction Cache.
		cache.reset();
		sr_clear( SR_CE );

		// TODO: The PLL Control register is initialized as described in Chapter 6, PLL and Clock Generator.

		reg.vba = TReg24(int(0));

		// TODO: The DSP56300 core remains in the Reset state until RESET is deasserted. Upon leaving the Reset state, the Chip Operating mode bits of the OMR are loaded from the external mode select pins (MODA, MODB, MODC, MODD), and program execution begins at the program memory address as described in Chapter 11, Operating Modes and Memory Spaces.

		reg.pc = TReg24(int(0));
		reg.omr = TReg24(int(0));

		reg.ictr.var = 0;
	}

	// _____________________________________________________________________________
	// exec
	//
	void DSP::exec()
	{
		// we do not support 16-bit compatibility mode
		assert( (reg.sr.var & SR_SC) == 0 && "16 bit compatibility mode is not supported");

		perif[0]->exec();

		if(perif[1] != perif[0])
			perif[1]->exec();

		m_currentOpLen = 1;
		
		if(m_processingMode == Default && !m_pendingInterrupts.empty())
		{
			// TODO: priority sorting, masking
			const auto vba = m_pendingInterrupts.pop_front();

			const auto minPrio = mr().var & 0x3;

			TWord op0, op1;
			memRead2(MemArea_P, vba, op0, op1);

			const auto oldSP = reg.sp.var;

			m_opWordB = op1;

			m_processingMode = FastInterrupt;

			pcCurrentInstruction = vba;
			getASM(op0, op1);
			execOp(op0);

			auto jumped = reg.sp.var - oldSP;

			// only exec the second op if the first one was a one-word op and we did not jump into a long interrupt
			if(m_currentOpLen == 1 && !jumped)
			{
				pcCurrentInstruction = vba+1;
				getASM(op1, 0);
				execOp(op1);

				m_processingMode = DefaultPreventInterrupt;
			}
			else if(jumped)
			{
				m_processingMode = LongInterrupt;
			}

			return;
		}

		if(m_processingMode == DefaultPreventInterrupt)
			m_processingMode = Default;

		pcCurrentInstruction = reg.pc.toWord();

		const auto op = fetchPC();

		getASM(op, m_opWordB);

		execOp(op);
	}

	void DSP::execOp(const TWord op)
	{
		const TWord currentOp = pcCurrentInstruction;
		const auto opCache = m_opcodeCache[currentOp];

		exec_jump(static_cast<Instruction>(opCache & 0xff), op);

		++reg.ictr.var;
		++m_instructions;

		if(g_dumpPC && (opCache & 0xff) != ResolveCache && pcCurrentInstruction == currentOp)
			traceOp();
	}

	void DSP::exec_jump(const Instruction inst, const TWord op)
	{
		const auto func = g_jumpTable[inst];
		(this->*func)(op);
	}

	bool DSP::exec_parallel(const Instruction instMove, const Instruction instAlu, const TWord op)
	{
		// simulate latches registers for parallel instructions
		const auto preMoveX = reg.x;
		const auto preMoveY = reg.y;
		const auto preMoveA = reg.a;
		const auto preMoveB = reg.b;

		exec_jump(instMove, op);

		const auto postMoveX = reg.x;
		const auto postMoveY = reg.y;
		const auto postMoveA = reg.a;
		const auto postMoveB = reg.b;

		// restore previous state for the ALU to process them
		reg.x = preMoveX;
		reg.y = preMoveY;
		reg.a = preMoveA;
		reg.b = preMoveB;

		exec_jump(instAlu, op);

		// now check what has changed and get the final values for all registers
		if( postMoveX != preMoveX )
		{
			assert( preMoveX == reg.x && "ALU changed a register at the same time the MOVE command changed it!" );
			reg.x = postMoveX;
		}
		else if( reg.x != preMoveX )
		{
			assert( preMoveX == postMoveX && "ALU changed a register at the same time the MOVE command changed it!" );
		}

		if( postMoveY != preMoveY )
		{
			assert( preMoveY == reg.y && "ALU changed a register at the same time the MOVE command changed it!" );
			reg.y = postMoveY;
		}
		else if( reg.y != preMoveY )
		{
			assert( preMoveY == postMoveY && "ALU changed a register at the same time the MOVE command changed it!" );
		}

		if( postMoveA != preMoveA )
		{
			assert( preMoveA == reg.a && "ALU changed a register at the same time the MOVE command changed it!" );
			reg.a = postMoveA;
		}
		else if( reg.a != preMoveA )
		{
			assert( preMoveA == postMoveA && "ALU changed a register at the same time the MOVE command changed it!" );
		}

		if( postMoveB != preMoveB )
		{
			assert( preMoveB == reg.b && "ALU changed a register at the same time the MOVE command changed it!" );
			reg.b = postMoveB;
		}
		else if( reg.b != preMoveB )
		{
			assert( preMoveB == postMoveB && "ALU changed a register at the same time the MOVE command changed it!" );
		}
		return true;
	}

	bool DSP::exec_parallel_alu(TWord op)
	{
#ifdef _DEBUG
		const auto* opInfo = m_opcodes.findParallelAluOpcodeInfo(op);
#endif
		op &= 0xff;

		switch(op>>7)
		{
		case 0:
			return exec_parallel_alu_nonMultiply(op);
		case 1:
			return exec_parallel_alu_multiply(op);
		default:
			assert(0 && "invalid op");
			return false;
		}
	}

	bool DSP::exec_parallel_alu_nonMultiply(TWord op)
	{
		const auto kkk = op & 0x7;
		const auto D = (op>>3) & 0x1;
		const auto JJJ = (op>>4) & 0x7;

		if(!JJJ && !kkk)
			return true;	// TODO: docs say "MOVE" does it mean no ALU op?

		if(!kkk)
		{
			alu_add(D, decode_JJJ_read_56(JJJ, !D));
			return true;
		}

		if(kkk == 4)
		{
			alu_sub(D, decode_JJJ_read_56(JJJ, !D));
			return true;
		}

		if(JJJ >= 4)
		{
			switch (kkk)
			{
			case 1:		alu_tfr(D, decode_JJJ_read_56(JJJ, !D));			return true;
			case 2:		alu_or(D, decode_JJJ_read_24(JJJ, !D).var);			return true;
			case 3:		LOG_ERR_NOTIMPLEMENTED("eor");						return true;	// alu_eor(D, decode_JJJ_read_24(JJJ, !D).var);
			case 5:		alu_cmp(D, decode_JJJ_read_56(JJJ, !D), false);		return true;
			case 6:		alu_and(D, decode_JJJ_read_24(JJJ, !D).var);		return true;
			case 7:		alu_cmp(D, decode_JJJ_read_56(JJJ, !D), true);		return true;
			}
		}

		switch (JJJ)
		{
		case 0:
			switch (kkk)
			{
			case 1:		alu_tfr(D, decode_JJJ_read_56(JJJ, !D));			return true;
			case 2:		alu_addr(D);										return true;
			case 3:		alu_tst(D);											return true;
			case 5:		alu_cmp(D, D ? reg.a : reg.b, false);				return true;
			case 6:		LOG_ERR_NOTIMPLEMENTED("addr");						return true;	// alu_subr
			case 7:		alu_cmp(D, D ? reg.a : reg.b, true);				return true;
			}
		case 1:
			switch (kkk)
			{
			case 1:		alu_rnd(D);								return true;
			case 2:		alu_addl(D);							return true;
			case 3:		alu_clr(D);								return true;
			case 6:		LOG_ERR_NOTIMPLEMENTED("subl");			return true;	// alu_subl
			case 7:		alu_not(D);								return true;
			}
			break;
		case 2:
			switch (kkk)
			{
			case 2:		alu_asr(D, D, 1);						return true;
			case 3:		alu_lsr(D, 1);							return true;
			case 6:		alu_abs(D);								return true;
			case 7:		LOG_ERR_NOTIMPLEMENTED("ror");			return true;	// alu_ror
			}
			break;
		case 3:
			switch (kkk)
			{
			case 2:		alu_asl(D, D, 1);						return true;
			case 3:		alu_lsl(D, 1);							return true;
			case 6:		alu_neg(D);								return true;
			case 7:		alu_rol(D);								return true;
			}
			break;
		case 4:
		case 5:
			switch (kkk)
			{
			case 1:		LOG_ERR_NOTIMPLEMENTED("adc");			return true;	// alu_adc
			case 5:		LOG_ERR_NOTIMPLEMENTED("sbc");			return true;	// alu_sbc
			}
		}
		return false;
	}

	bool DSP::exec_parallel_alu_multiply(const TWord op)
	{
		const auto round = op & 0x1;
		const auto mulAcc = (op>>1) & 0x1;
		const auto negative = (op>>2) & 0x1;
		const auto ab = (op>>3) & 0x1;
		const auto qqq = (op>>4) & 0x7;

		TReg24 s1, s2;

		decode_QQQ_read(s1, s2, qqq);

		// TODO: ccr
		alu_mpy(ab, s1, s2, negative, mulAcc);
		if(round)
		{
			alu_rnd(ab);
		}

		return true;
	}

	// _____________________________________________________________________________
	// decode_MMMRRR
	//
	dsp56k::TWord DSP::decode_MMMRRR_read( TWord _mmmrrr )
	{
		switch( _mmmrrr & 0x3f )
		{
		case MMM_AbsAddr:		return fetchOpWordB();		// absolute address
		case MMM_ImmediateData:	return fetchOpWordB();		// immediate data
		}

		unsigned int regIdx = _mmmrrr & 0x7;

		const TReg24	_n = reg.n[regIdx];
		TReg24&			_r = reg.r[regIdx];
		const TReg24	_m = reg.m[regIdx];

		TWord r = _r.toWord();

		TWord a;

		switch( _mmmrrr & 0x38 )
		{
		case 0x00:	/* 000 */	a = r;					AGU::updateAddressRegister(r,-_n.var,_m.var);					break;
		case 0x08:	/* 001 */	a = r;					AGU::updateAddressRegister(r,+_n.var,_m.var);					break;
		case 0x10:	/* 010 */	a = r;					AGU::updateAddressRegister(r,-1,_m.var);						break;
		case 0x18:	/* 011 */	a =	r;					AGU::updateAddressRegister(r,+1,_m.var);						break;
		case 0x20:	/* 100 */	a = r;																					break;
		case 0x28:	/* 101 */	a = r + _n.toWord();																	break;
		case 0x38:	/* 111 */							AGU::updateAddressRegister(r,-1,_m.var);		a = _r.var;		break;

		default:
			assert(0 && "impossible to happen" );
			return 0xbadbad;
		}

		_r.var = r;

		assert( a >= 0x00000000 && a <= 0x00ffffff && "invalid memory address" );
		return a;
	}

	// _____________________________________________________________________________
	// decode_XMove_MMRRR
	//
	dsp56k::TWord DSP::decode_XMove_MMRRR( TWord _mmrrr )
	{
		unsigned int regIdx = _mmrrr & 0x07;

		const TReg24	_n = reg.n[regIdx];
		TReg24&			_r = reg.r[regIdx];
		const TReg24	_m = reg.m[regIdx];

		TWord r = _r.toWord();

		TWord a;

		switch( _mmrrr & 0x18 )
		{
		case 0x08:	/* 01 */	a = r;	AGU::updateAddressRegister(r,+_n.var,_m.var);	break;
		case 0x10:	/* 10 */	a = r;	AGU::updateAddressRegister(r,-1,_m.var);		break;
		case 0x18:	/* 11 */	a =	r;	AGU::updateAddressRegister(r,+1,_m.var);		break;
		case 0x00:	/* 00 */	a = r;													break;

		default:
			assert(0 && "impossible to happen" );
			return 0xbadbad;
		}

		_r.var = r;

		assert( a >= 0x00000000 && a <= 0x00ffffff && "invalid memory address" );
		return a;
	}
	// _____________________________________________________________________________
	// decode_YMove_mmrr
	//
	dsp56k::TWord DSP::decode_YMove_mmrr( TWord _mmrr, TWord _regIdxOffset )
	{
		unsigned int regIdx = _mmrr & 0x3;

		const TReg24	_n = reg.n[regIdx + _regIdxOffset];
		TReg24&			_r = reg.r[regIdx + _regIdxOffset];
		const TReg24	_m = reg.m[regIdx + _regIdxOffset];

		TWord r = _r.toWord();

		TWord a;

		switch( _mmrr & 0xc	 )
		{
		case 0x4:	/* 01 */	a = r;	AGU::updateAddressRegister(r,+_n.var,_m.var);	break;
		case 0x8:	/* 10 */	a = r;	AGU::updateAddressRegister(r,-1,_m.var);		break;
		case 0xc:	/* 11 */	a =	r;	AGU::updateAddressRegister(r,+1,_m.var);		break;
		case 0x0:	/* 00 */	a = r;													break;

		default:
			assert(0 && "impossible to happen" );
			return 0xbadbad;
		}

		_r.var = r;

		assert( a >= 0x00000000 && a <= 0x00ffffff && "invalid memory address" );
		return a;
	}

	// _____________________________________________________________________________
	// decode_RRR_read
	//
	dsp56k::TWord DSP::decode_RRR_read( TWord _mmmrrr, int _shortDisplacement )
	{
		unsigned int regIdx = _mmmrrr&0x07;

		const TReg24& _r = reg.r[regIdx];

		const int ea = _r.var + _shortDisplacement;

		return ea&0x00ffffff;
	}

	// _____________________________________________________________________________
	// decode_ddddd_pcr
	//
	dsp56k::TReg24 DSP::decode_ddddd_pcr_read( TWord _ddddd )
	{
		if( (_ddddd & 0x18) == 0x00 )
		{
			return reg.m[_ddddd&0x07];
		}

		switch( _ddddd )
		{
		case 0xa:	return reg.ep;
		case 0x10:	return reg.vba;
		case 0x11:	return TReg24(reg.sc.var);
		case 0x18:	return reg.sz;
		case 0x19:	return reg.sr;
		case 0x1a:	return reg.omr;
		case 0x1b:	return reg.sp;
		case 0x1c:	return ssh();
		case 0x1d:	return ssl();
		case 0x1e:	return reg.la;
		case 0x1f:	return reg.lc;
		}
		assert( 0 && "invalid ddddd value" );
		return TReg24(0xbadbad);
	}
	// _____________________________________________________________________________
	// decode_ddddd_pcr_write
	//
	void DSP::decode_ddddd_pcr_write( TWord _ddddd, TReg24 _val )
	{
		if( (_ddddd & 0x18) == 0x00 )
		{
			reg.m[_ddddd&0x07] = _val;
			return;
		}

		switch( _ddddd )
		{
		case 0xa:	reg.ep = _val;				return;
		case 0x10:	reg.vba = _val;				return;
		case 0x11:	reg.sc.var = _val.var&0x1f;	return;
		case 0x18:	reg.sz = _val;				return;
		case 0x19:	reg.sr = _val;				return;
		case 0x1a:	reg.omr = _val;				return;
		case 0x1b:	reg.sp = _val;				return;
		case 0x1c:	ssh(_val);					return;
		case 0x1d:	ssl(_val);					return;
		case 0x1e:	reg.la = _val;				return;
		case 0x1f:	reg.lc = _val;				return;
		}
		assert( 0 && "unreachable" );
	}

	// _____________________________________________________________________________
	// decode_eeeeee_read
	//
	TReg24 DSP::decode_dddddd_read( TWord _dddddd )
	{
		switch( _dddddd & 0x3f )
		{
			// 0000DD - 4 registers in data ALU - NOT DOCUMENTED but the motorola disasm claims it works, for example for the lua instruction
		case 0x00:	return x0();
		case 0x01:	return x1();
		case 0x02:	return y0();
		case 0x03:	return y1();
			// 0001DD - 4 registers in data ALU
		case 0x04:	return x0();
		case 0x05:	return x1();
		case 0x06:	return y0();
		case 0x07:	return y1();

			// 001DDD - 8 accumulators in data ALU
		case 0x08:	return a0();
		case 0x09:	return b0();
		case 0x0a:	{ TReg24 res; convertS(res,a2()); return res; }
		case 0x0b:	{ TReg24 res; convertS(res,b2()); return res; }
		case 0x0c:	return a1();
		case 0x0d:	return b1();
		case 0x0e:	return getA<TReg24>();
		case 0x0f:	return getB<TReg24>();

			// 010TTT - 8 address registers in AGU
		case 0x10:	return reg.r[0];
		case 0x11:	return reg.r[1];
		case 0x12:	return reg.r[2];
		case 0x13:	return reg.r[3];
		case 0x14:	return reg.r[4];
		case 0x15:	return reg.r[5];
		case 0x16:	return reg.r[6];
		case 0x17:	return reg.r[7];

			// 011NNN - 8 address offset registers in AGU
		case 0x18:	return reg.n[0];
		case 0x19:	return reg.n[1];
		case 0x1a:	return reg.n[2];
		case 0x1b:	return reg.n[3];
		case 0x1c:	return reg.n[4];
		case 0x1d:	return reg.n[5];
		case 0x1e:	return reg.n[6];
		case 0x1f:	return reg.n[7];

			// 100FFF - 8 address modifier registers in AGU
		case 0x20:	return reg.m[0];
		case 0x21:	return reg.m[1];
		case 0x22:	return reg.m[2];
		case 0x23:	return reg.m[3];
		case 0x24:	return reg.m[4];
		case 0x25:	return reg.m[5];
		case 0x26:	return reg.m[6];
		case 0x27:	return reg.m[7];

			// 101EEE - 1 adress register in AGU
		case 0x2a:	return reg.ep;

			// 110VVV - 2 program controller registers
		case 0x30:	return reg.vba;
		case 0x31:	return TReg24(reg.sc.var);

			// 111GGG - 8 program controller registers
		case 0x38:	return reg.sz;
		case 0x39:	return reg.sr;
		case 0x3a:	return reg.omr;
		case 0x3b:	return reg.sp;
		case 0x3c:	return ssh();
		case 0x3d:	return ssl();
		case 0x3e:	return reg.la;
		case 0x3f:	return reg.lc;
		}
		assert(0 && "invalid eeeeee value");
		return TReg24(0xbadbad);
	}

	// _____________________________________________________________________________
	// decode_eeeeee_write
	//
	void DSP::decode_dddddd_write( TWord _dddddd, TReg24 _val )
	{
		assert( (_val.var & 0xff000000) == 0 );

		switch( _dddddd & 0x3f )
		{
			// 0001DD - 4 registers in data ALU
		case 0x04:	x0(_val);	return;
		case 0x05:	x1(_val);	return;
		case 0x06:	y0(_val);	return;
		case 0x07:	y1(_val);	return;

			// 001DDD - 8 accumulators in data ALU
		case 0x08:	a0(_val);	return;
		case 0x09:	b0(_val);	return;
		case 0x0a:	{ TReg8 temp; convert(temp,_val); a2(temp);	return; }
		case 0x0b:	{ TReg8 temp; convert(temp,_val); b2(temp);	return; }
		case 0x0c:	a1(_val);	return;
		case 0x0d:	b1(_val);	return;
		case 0x0e:	setA(_val);	return;
		case 0x0f:	setB(_val);	return;

			// 010TTT - 8 address registers in AGU
		case 0x10:	reg.r[0] = _val;	return;
		case 0x11:	reg.r[1] = _val;	return;
		case 0x12:	reg.r[2] = _val;	return;
		case 0x13:	reg.r[3] = _val;	return;
		case 0x14:	reg.r[4] = _val;	return;
		case 0x15:	reg.r[5] = _val;	return;
		case 0x16:	reg.r[6] = _val;	return;
		case 0x17:	reg.r[7] = _val;	return;

			// 011NNN - 8 address offset registers in AGU
		case 0x18:	reg.n[0] = _val;	return;
		case 0x19:	reg.n[1] = _val;	return;
		case 0x1a:	reg.n[2] = _val;	return;
		case 0x1b:	reg.n[3] = _val;	return;
		case 0x1c:	reg.n[4] = _val;	return;
		case 0x1d:	reg.n[5] = _val;	return;
		case 0x1e:	reg.n[6] = _val;	return;
		case 0x1f:	reg.n[7] = _val;	return;

			// 100FFF - 8 address modifier registers in AGU
		case 0x20:	reg.m[0] = _val;	return;
		case 0x21:	reg.m[1] = _val;	return;
		case 0x22:	reg.m[2] = _val;	return;
		case 0x23:	reg.m[3] = _val;	return;
		case 0x24:	reg.m[4] = _val;	return;
		case 0x25:	reg.m[5] = _val;	return;
		case 0x26:	reg.m[6] = _val;	return;
		case 0x27:	reg.m[7] = _val;	return;

			// 101EEE - 1 adress register in AGU
		case 0x2a:	reg.ep = _val;		return;

			// 110VVV - 2 program controller registers
		case 0x30:	reg.vba = _val;			return;
		case 0x31:	reg.sc.var = _val.var & 0x1f;	return;

			// 111GGG - 8 program controller registers
		case 0x38:	reg.sz = _val;	return;
		case 0x39:	reg.sr = _val;	return;
		case 0x3a:	reg.omr = _val;	return;
		case 0x3b:	reg.sp = _val;	return;
		case 0x3c:	ssh(_val);	return;
		case 0x3d:	ssl(_val);	return;
		case 0x3e:	reg.la = _val;	return;
		case 0x3f:	reg.lc = _val;	return;
		}
		assert(0 && "unknown register");
	}
	// _____________________________________________________________________________
	// logSC
	//
	void DSP::logSC( const char* _func ) const
	{
		if( strstr(_func, "DO" ) )
			return;

		std::stringstream scss;
		
		for( int i=0; i<reg.sc.var; ++i )
		{
			scss << "\t";
		}

		LOG( std::string(scss.str()) << " SC=" << std::hex << std::setw(6) << std::setfill('0') << (int)reg.sc.var << " pcOld=" << pcCurrentInstruction << " pcNew=" << reg.pc.var << " ictr=" << reg.ictr.var << " func=" << _func );
	}

	// _____________________________________________________________________________
	// resetSW
	//
	void DSP::resetSW()
	{
		/*
		Reset the interrupt priority register and all on-chip peripherals. This is a
		software reset, which is not equivalent to a hardware RESET since only on-chip peripherals
		and the interrupt structure are affected. The processor state is not affected, and execution
		continues with the next instruction. All interrupt sources are disabled except for the stack
		error, NMI, illegal instruction, Trap, Debug request, and hardware reset interrupts.
		*/

		perif[0]->reset();
		if(perif[1] != perif[0])
			perif[1]->reset();
	}

	void DSP::jsr(const TReg24& _val)
	{
		pushPCSR();
		setPC(_val);
	}

	// _____________________________________________________________________________
	// exec_move_ddddd_MMMRRR
	//
	void DSP::exec_move_ddddd_MMMRRR( TWord ddddd, TWord mmmrrr, bool write, EMemArea memArea )
	{
		if( write && mmmrrr == MMM_ImmediateData )
		{
			decode_ddddd_write<TReg24>( ddddd, TReg24(fetchOpWordB()) );
			return;
		}

		const auto addr = decode_MMMRRR_read( mmmrrr );

		if(addr >= XIO_Reserved_High_First)
		{
			if( write )
			{
				decode_ddddd_write<TReg24>( ddddd, TReg24(memReadPeriph( memArea, addr )) );
			}
			else
			{
				memWritePeriph( memArea, addr, decode_ddddd_read<TWord>( ddddd ) );
			}
		}
		else
		{
			if( write )
			{
				decode_ddddd_write<TReg24>( ddddd, TReg24(memRead( memArea, addr )) );
			}
			else
			{
				memWrite( memArea, addr, decode_ddddd_read<TWord>( ddddd ) );
			}			
		}
	}

	// _____________________________________________________________________________
	// decode_cccc
	//
	bool DSP::decode_cccc( TWord cccc ) const
	{
	#define			SRT_C			sr_val(SRB_C)			// carry
	#define 		SRT_V			sr_val(SRB_V)			// overflow
	#define 		SRT_Z			sr_val(SRB_Z)			// zero
	#define 		SRT_N			sr_val(SRB_N)			// negative
	#define 		SRT_U			sr_val(SRB_U)			// unnormalized
	#define 		SRT_E			sr_val(SRB_E)			// extension
	#define 		SRT_L			sr_val(SRB_L)			// limit
	//#define 		SRT_S			sr_val(SRB_S)			// scaling

		switch( cccc )
		{
		case CCCC_CarrySet:			return SRT_C!=0;								// CC(LO)		Carry Set	(lower)
		case CCCC_CarryClear:		return !SRT_C;									// CC(HS)		Carry Clear (higher or same)

		case CCCC_ExtensionSet:		return SRT_E!=0;								// ES			Extension set
		case CCCC_ExtensionClear:	return !SRT_E;									// EC			Extension clear

		case CCCC_Equal:			return SRT_Z!=0;								// EQ			Equal
		case CCCC_NotEqual:			return !SRT_Z;									// NE			Not Equal

		case CCCC_LimitSet:			return SRT_L!=0;								// LS			Limit set
		case CCCC_LimitClear:		return !SRT_L;									// LC			Limit clear

		case CCCC_Minus:			return SRT_N!=0;								// MI			Minus
		case CCCC_Plus:				return !SRT_N;									// PL			Plus

		case CCCC_GreaterEqual:		return SRT_N == SRT_V;							// GE			Greater than or equal
		case CCCC_LessThan:			return SRT_N != SRT_V;							// LT			Less than

		case CCCC_Normalized:		return (SRT_Z + ((!SRT_U) | (!SRT_E))) == 1;	// NR			Normalized
		case CCCC_NotNormalized:	return (SRT_Z + ((!SRT_U) | !SRT_E)) == 0;		// NN			Not normalized

		case CCCC_GreaterThan:		return (SRT_Z + (SRT_N != SRT_V)) == 0;			// GT			Greater than
		case CCCC_LessEqual:		return (SRT_Z + (SRT_N != SRT_V)) == 1;			// LE			Less than or equal
		}
		assert( 0 && "unreachable" );
		return false;
	}

		void DSP::sr_debug(char* _dst) const
		{
			_dst[8] = 0;
			_dst[7] = sr_test(SR_C) ? 'C' : 'c';
			_dst[6] = sr_test(SR_V) ? 'V' : 'v';
			_dst[5] = sr_test(SR_Z) ? 'Z' : 'z';
			_dst[4] = sr_test(SR_N) ? 'N' : 'n';
			_dst[3] = sr_test(SR_U) ? 'U' : 'u';
			_dst[2] = sr_test(SR_E) ? 'E' : 'e';
			_dst[1] = sr_test(SR_L) ? 'L' : 'l';
			_dst[0] = sr_test(SR_S) ? 'S' : 's';
		}

		// _____________________________________________________________________________
	// dumpCCCC
	//
	void DSP::dumpCCCC() const
	{
		LOG( "CCCC Carry Clear        " << decode_cccc(0x0) );
		LOG( "CCCC >=                 " << decode_cccc(0x1) );
		LOG( "CCCC !=                 " << decode_cccc(0x2) );
		LOG( "CCCC Plus               " << decode_cccc(0x3) );
		LOG( "CCCC Not normalized     " << decode_cccc(0x4) );
		LOG( "CCCC Extension clear    " << decode_cccc(0x5) );
		LOG( "CCCC Limit clear        " << decode_cccc(0x6) );
		LOG( "CCCC >                  " << decode_cccc(0x7) );
		LOG( "CCCC Carry Set (lower)  " << decode_cccc(0x8) );
		LOG( "CCCC <                  " << decode_cccc(0x9) );
		LOG( "CCCC ==                 " << decode_cccc(0xa) );
		LOG( "CCCC Minus              " << decode_cccc(0xb) );
		LOG( "CCCC Normalized         " << decode_cccc(0xc) );
		LOG( "CCCC Extension Set      " << decode_cccc(0xd) );
		LOG( "CCCC Limit Set          " << decode_cccc(0xe) );
		LOG( "CCCC <=                 " << decode_cccc(0xf) );
	}
	// _____________________________________________________________________________
	// exec_do
	//
	bool DSP::do_exec( TReg24 _loopcount, TWord _addr )
	{
	//	LOG( "DO BEGIN: " << (int)sc.var << ", loop flag = " << sr_test(SR_LF) );

		if( !_loopcount.var )
		{
			if( sr_test( SR_SC ) )
				_loopcount.var = 65536;
			else
			{
				setPC(_addr+1);
				return true;
			}
		}

		ssh(reg.la);
		ssl(reg.lc);

		reg.la.var = _addr;
		reg.lc = _loopcount;

		pushPCSR();

		const auto stackCount = reg.sc.var;
		
		sr_set( SR_LF );

		traceOp();

		// __________________
		//

		while(sr_test(SR_LF) && reg.sc.var >= stackCount)
		{
			exec();

			if(reg.pc.var != (reg.la.var+1))
				continue;

			if( reg.lc.var <= 1 )
			{
				do_end();
				break;
			}

			--reg.lc.var;
			setPC(hiword(reg.ss[reg.sc.toWord()]));
		}
		return true;
	}

	// _____________________________________________________________________________
	// exec_do_end
	//
	bool DSP::do_end()
	{
		// restore PC to point to the next instruction after the last instruction of the loop
		setPC(reg.la.var+1);

		// restore previous loop flag
		sr_toggle( SR_LF, (ssl().var & SR_LF) != 0 );

		// decrement SP twice, restoring old loop settings
		decSP();

		reg.lc = ssl();
		reg.la = ssh();

	//	LOG( "DO END: loop flag = " << sr_test(SR_LF) << " sc=" << (int)sc.var << " lc:" << std::hex << lc.var << " la:" << std::hex << la.var );

		return true;
	}

	bool DSP::rep_exec(const TWord loopCount)
	{
		traceOp();

		pcCurrentInstruction = reg.pc.var;
		const auto op = fetchPC();

		const auto lcBackup = reg.lc;
		reg.lc.var = loopCount;

		getASM(op, m_opWordB);

		execOp(op);
		// TODO: remember function pointer and call directly

		while( reg.lc.var > 1 )
		{
			--reg.lc.var;
			execOp(op);
		}

		reg.lc = lcBackup;

		return true;
	}

	void DSP::traceOp()
	{
		if(!g_dumpPC || !m_trace)
			return;

		const auto op = memRead(MemArea_P, pcCurrentInstruction);

		std::stringstream ss;
		ss << "p:$" << HEX(pcCurrentInstruction) << ' ' << HEX(op);
		if(m_currentOpLen > 1)
			ss << ' ' << HEX(m_opWordB);
		else
			ss << "       ";
		ss << " = " << m_asm;
		const std::string str(ss.str());
		LOGF(str);

		dumpRegisters();

		for( size_t i=0; i<Reg_COUNT; ++i )
		{
			int64_t regVal = 0;
			const bool r = readRegToInt( (EReg)i, regVal );

			if( !r )
				continue;
			//			assert( r && "failed to read register" );

			if( regVal != m_prevRegStates[i].val )
			{
				SRegChange regChange;
				regChange.reg = (EReg)i;
				regChange.valOld.var = (int)m_prevRegStates[i].val;
				regChange.valNew.var = (int)regVal;
				regChange.pc = pcCurrentInstruction;
				regChange.ictr = reg.ictr.var;

				m_regChanges.push_back( regChange );

				m_prevRegStates[i].val = regVal;
			}
		}
	}

	void DSP::decSP()
	{
		LOGSC("return");

		assert(reg.sc.var > 0);
		--reg.sp.var;
		--reg.sc.var;
	}

	void DSP::incSP()
	{
		assert(reg.sc.var < reg.ss.eSize-1);
		++reg.sp.var;
		++reg.sc.var;

		const std::string sym = mem.getSymbol(MemArea_P, reg.pc.var);
			
		LOGSC((std::string(m_asm) + " - " + sym).c_str());
	//	assert( reg.sc.var <= 9 );
	}

	// _____________________________________________________________________________
	// alu_and
	//
	void DSP::alu_and( bool ab, TWord _val )
	{
		TReg56& d = ab ? reg.b : reg.a;

		d.var &= (TInt64(_val)<<24);

		// S L E U N Z V C
		// v - - - * * * -
		sr_toggle( SR_N, bittest( d, 47 ) );
		sr_toggle( SR_Z, (d.var & 0xffffff000000) == 0 );
		sr_clear( SR_V );
		sr_s_update();
	}


	// _____________________________________________________________________________
	// alu_or
	//
	void DSP::alu_or( bool ab, TWord _val )
	{
		TReg56& d = ab ? reg.b : reg.a;

		d.var |= (TInt64(_val)<<24);

		// S L E U N Z V C
		// v - - - * * * -
		sr_toggle( SR_N, bittest( d, 47 ) );
		sr_toggle( SR_Z, (d.var & 0xffffff000000) == 0 );
		sr_clear( SR_V );
		sr_s_update();
	}

	// _____________________________________________________________________________
	// alu_add
	//
	void DSP::alu_add( bool ab, const TReg56& _val )
	{
		TReg56& d = ab ? reg.b : reg.a;

		const TReg56 old = d;

		const TInt64 d64 = d.signextend<TInt64>();

		const TInt64 res = d64 + _val.signextend<TInt64>();

		d.var = res;
		d.doMasking();

		// S L E U N Z V C

		sr_s_update();
		sr_e_update(d);
		sr_u_update(d);
		sr_n_update(d);
		sr_z_update(d);
		sr_v_update(res,d);
		sr_l_update_by_v();
		sr_c_update_arithmetic(old, d);
	//	sr_c_update_logical(d);

	//	dumpCCCC();
	}

	// _____________________________________________________________________________
	// alu_cmp
	//
	void DSP::alu_cmp( bool ab, const TReg56& _val, bool _magnitude )
	{
		TReg56& d = ab ? reg.b : reg.a;

		const TReg56 oldD = d;

		TInt64 d64 = d.signextend<TInt64>();

		TInt64 val = _val.signextend<TInt64>();

		if( _magnitude )
		{
			d64 = d64 < 0 ? -d64 : d64;
			val = val < 0 ? -val : val;
		}

		const TInt64 res = d64 - val;

		d.var = res;
		d.doMasking();

		sr_s_update();
		sr_e_update(d);
		sr_u_update(d);
		sr_n_update(d);
		sr_z_update(d);
		sr_v_update(res,d);
		sr_l_update_by_v();
		sr_c_update_arithmetic(oldD,d);

		d = oldD;
	}
	// _____________________________________________________________________________
	// alu_sub
	//
	void DSP::alu_sub( bool ab, const TReg56& _val )
	{
		TReg56& d = ab ? reg.b : reg.a;

		const TReg56 old = d;

		const TInt64 d64 = d.signextend<TInt64>();

		const TInt64 res = d64 - _val.signextend<TInt64>();

		d.var = res;
		d.doMasking();

		// S L E U N Z V C

		sr_s_update();
		sr_e_update(d);
		sr_u_update(d);
		sr_n_update(d);
		sr_z_update(d);
		sr_v_update(res,d);
		sr_l_update_by_v();
		sr_c_update_arithmetic( old, d );
	//	sr_c_update_logical( d );
	}

	// _____________________________________________________________________________
	// alu_asr
	//
	void DSP::alu_asr( bool abDst, bool abSrc, int _shiftAmount )
	{
		const TReg56& dSrc = abSrc ? reg.b : reg.a;

		const TInt64 d64 = dSrc.signextend<TInt64>();

		sr_toggle( SR_C, _shiftAmount && bittest(d64,_shiftAmount-1) );

		const TInt64 res = d64 >> _shiftAmount;

		TReg56& d = abDst ? reg.b : reg.a;

		d.var = res & 0x00ffffffffffffff;

		// S L E U N Z V C

		sr_s_update();
		sr_e_update(d);
		sr_u_update(d);
		sr_n_update(d);
		sr_z_update(d);
		sr_clear(SR_V);
		sr_l_update_by_v();
	}

	// _____________________________________________________________________________
	// alu_asl
	//
	void DSP::alu_asl( bool abDst, bool abSrc, int _shiftAmount )
	{
		const TReg56& dSrc = abSrc ? reg.b : reg.a;

		const TInt64 d64 = dSrc.signextend<TInt64>();

		sr_toggle( SR_C, _shiftAmount && ((d64 & (TInt64(1)<<(56-_shiftAmount))) != 0) );

		const TInt64 res = d64 << _shiftAmount;

		TReg56& d = abDst ? reg.b : reg.a;

		d.var = res & 0x00ffffffffffffff;

		// Overflow: Set if Bit 55 is changed any time during the shift operation, cleared otherwise.
		// What that means for us if all bits that are shifted out need to be identical to not overflow
		int64_t overflowMaskI = 0x8000000000000000;
		overflowMaskI >>= _shiftAmount;
		uint64_t overflowMaskU = overflowMaskI;
		overflowMaskU >>= 8;
		const uint64_t v = dSrc.var & overflowMaskU;
		const bool isOverflow = v != overflowMaskU && v != 0;

		// S L E U N Z V C
		sr_s_update();
		sr_e_update(d);
		sr_u_update(d);
		sr_n_update(d);
		sr_z_update(d);
		sr_toggle(SR_V, isOverflow);
		sr_l_update_by_v();
	}

	// _____________________________________________________________________________
	// alu_lsl
	//
	void DSP::alu_lsl( bool ab, int _shiftAmount )
	{
		TReg24 d = ab ? b1() : a1();

		sr_toggle( SR_C, _shiftAmount && bittest( d, 23-_shiftAmount+1) );

		const int res = (d.var << _shiftAmount) & 0x00ffffff;

		if( ab )
			b1(TReg24(res));
		else
			a1(TReg24(res));

		// S L E U N Z V C

		sr_toggle( SR_N, bittest(res,23) );
		sr_toggle( SR_Z, res == 0 );
		sr_clear( SR_V );

		sr_s_update();
		sr_l_update_by_v();
	}

	// _____________________________________________________________________________
	// alu_lsr
	//
	void DSP::alu_lsr( bool ab, int _shiftAmount )
	{
		TReg24 d = ab ? b1() : a1();

		sr_toggle( SR_C, _shiftAmount && bittest( d, _shiftAmount-1) );

		const unsigned int res = ((unsigned int)d.var >> _shiftAmount);

		if( ab )
			b1(TReg24(res));
		else
			a1(TReg24(res));

		// S L E U N Z V C

		sr_toggle( SR_N, bittest(res,23) );
		sr_toggle( SR_Z, res == 0 );
		sr_clear( SR_V );

		sr_s_update();
		sr_l_update_by_v();
	}


	void DSP::alu_addr(bool ab)
	{
		TReg56&			d = ab ? reg.b : reg.a;
		const TReg56&	s = ab ? reg.a : reg.b;

		const TReg56 old = d;
		const TInt64 res = (d.signextend<TInt64>() >> 1) + s.signextend<TInt64>();
		d.var = res;
		d.doMasking();

		sr_s_update();
		sr_e_update(d);
		sr_u_update(d);
		sr_n_update(d);
		sr_z_update(d);
		sr_v_update(res, d);
		sr_l_update_by_v();
		sr_c_update_arithmetic(old,d);
	}

	void DSP::alu_rol(const bool ab)
	{
		auto& d = ab ? reg.b.var : reg.a.var;

		const auto c = bitvalue<uint64_t,47>(d);

		auto shifted = (d << 1) & 0x00ffffff000000;
		shifted |= sr_val(SRB_C) << 24;
		
		d &= 0xff000000ffffff;
		d |= shifted;

		sr_toggle(SR_N, bitvalue<uint64_t, 47>(shifted));	// Set if bit 47 of the result is set
		sr_toggle(SR_Z, shifted == 0);						// Set if bits 47?24 of the result are 0
		sr_clear(SR_V);										// This bit is always cleared
		sr_toggle(SR_C, c);									// Set if bit 47 of the destination operand is set, and cleared otherwise
	}

	void DSP::alu_addl(bool ab)
	{
		TReg56&			d = ab ? reg.b : reg.a;
		const TReg56&	s = ab ? reg.a : reg.b;

		const TReg56 old = d;
		const TInt64 res = (d.signextend<TInt64>() << 1) + s.signextend<TInt64>();
		d.var = res;
		d.doMasking();

		sr_s_update();
		sr_e_update(d);
		sr_u_update(d);
		sr_n_update(d);
		sr_z_update(d);
		sr_v_update(res, d);
		sr_l_update_by_v();
		sr_c_update_arithmetic(old,d);
	}

	inline void DSP::alu_clr(bool ab)
	{
		auto& dst = ab ? reg.b : reg.a;
		dst.var = 0;

		sr_clear( static_cast<CCRMask>(SR_E | SR_N | SR_V) );
		sr_set( static_cast<CCRMask>(SR_U | SR_Z) );
		// TODO: SR_L and SR_S are changed according to standard definition, but that should mean that no update is required?!
	}

	// _____________________________________________________________________________
	// alu_bclr
	//
	TWord DSP::alu_bclr( TWord _bit, TWord _val )
	{
		sr_toggle( SR_C, bittest(_val,_bit) );

		_val &= ~(1<<_bit);

		sr_l_update_by_v();
		sr_s_update();

		return _val;
	}

	// _____________________________________________________________________________
	// alu_mpy
	//
	void DSP::alu_mpy( bool ab, const TReg24& _s1, const TReg24& _s2, bool _negate, bool _accumulate )
	{
	//	assert( sr_test(SR_S0) == 0 && sr_test(SR_S1) == 0 );

		const int64_t s1 = _s1.signextend<int64_t>();
		const int64_t s2 = _s2.signextend<int64_t>();

		// TODO: revisit signed fraction multiplies
		auto res = s1 * s2;

		if( sr_test(SR_S0) )
		{
		}
		else if( sr_test(SR_S1) )
			res <<= 2;
		else
			res <<= 1;

		if( _negate )
			res = -res;

		TReg56& d = ab ? reg.b : reg.a;

		const TReg56 old = d;

		if( _accumulate )
			res += d.signextend<int64_t>();

		d.var = res & 0x00ffffffffffffff;

		// Update SR
		sr_s_update();
		sr_clear( SR_E );	// I don't know how this should happen because both operands are only 24 bits, the doc says "changed by standard definition" which should be okay here
		sr_u_update( d );
		sr_n_update( d );
		sr_z_update( d );
		sr_v_update(res,d);

		sr_l_update_by_v();
	}
	// _____________________________________________________________________________
	// alu_mpysuuu
	//
	void DSP::alu_mpysuuu( bool ab, TReg24 _s1, TReg24 _s2, bool _negate, bool _accumulate, bool _suuu )
	{
	//	assert( sr_test(SR_S0) == 0 && sr_test(SR_S1) == 0 );

		TInt64 res;

		if( _suuu )
			res = TInt64( TUInt64(_s1.var) * TUInt64(_s2.var) );
		else
			res = _s1.signextend<TInt64>() * TUInt64(_s2.var);

		// fractional multiplication requires one post-shift to be correct
		if( sr_test(SR_S0) )
		{
		}
		else if( sr_test(SR_S1) )
			res <<= 2;
		else
			res <<= 1;

		if( _negate )
			res = -res;

		TReg56& d = ab ? reg.b : reg.a;

		const TReg56 old = d;

		if( _accumulate )
			res += d.signextend<TInt64>();

		d.var = res;
		d.doMasking();

		// Update SR
		sr_e_update(d);
		sr_u_update( d );
		sr_n_update( d );
		sr_z_update( d );
		sr_v_update(res,d);

		sr_l_update_by_v();
	}
	// _____________________________________________________________________________
	// alu_dmac
	//
	void DSP::alu_dmac( bool ab, TReg24 _s1, TReg24 _s2, bool _negate, bool srcUnsigned, bool dstUnsigned )
	{
		assert( sr_test(SR_S0) == 0 && sr_test(SR_S1) == 0 );

		TInt64 res;

		if( srcUnsigned && dstUnsigned )	res = TInt64( TUInt64(_s1.var) * TUInt64(_s2.var) );
		else if( srcUnsigned )				res = TUInt64(_s1.var) * _s2.signextend<TInt64>();
		else if( dstUnsigned )				res = TUInt64(_s2.var) * _s1.signextend<TInt64>();
		else								res = _s2.signextend<TInt64>() * _s1.signextend<TInt64>();

		// fractional multiplication requires one post-shift to be correct
		if( sr_test(SR_S0) )
		{
		}
		else if( sr_test(SR_S1) )
			res <<= 2;
		else
			res <<= 1;

		if( _negate )
			res = -res;

		TReg56& d = ab ? reg.b : reg.a;

		const TReg56 old = d;

		TInt64 dShifted = d.signextend<TInt64>() >> 24;

		res += dShifted;

	//	LOG( "DMAC  " << std::hex << old.var << std::hex << " + " << _s1.var << " * " << std::hex << _s2.var << " = " << std::hex << res );

		d.var = res;
		d.doMasking();

		// Update SR
		sr_e_update(d);
		sr_u_update( d );
		sr_n_update( d );
		sr_z_update( d );
		sr_v_update(res,d);

		sr_l_update_by_v();
	}

	// _____________________________________________________________________________
	// alu_mac
	//
	void DSP::alu_mac( bool ab, TReg24 _s1, TReg24 _s2, bool _negate, bool _uu )
	{
		assert( sr_test(SR_S0) == 0 && sr_test(SR_S1) == 0 );

		TInt64 res;

		if( _uu )
			res = TInt64( TUInt64(_s1.var) * TUInt64(_s2.var) );
		else
			res = _s1.signextend<TInt64>() * TUInt64(_s2.var);

		// fractional multiplication requires one post-shift to be correct
		if( sr_test(SR_S0) )
		{
		}
		else if( sr_test(SR_S1) )
			res <<= 2;
		else
			res <<= 1;

		if( _negate )
			res = -res;

		TReg56& d = ab ? reg.b : reg.a;

		res += d.var;

		const TReg56 old = d;

		d.var = res;

		d.doMasking();

		// Update SR
		sr_s_update();
		sr_e_update(d);
		sr_u_update( d );
		sr_n_update( d );
		sr_z_update( d );
		sr_v_update(res,d);

		sr_l_update_by_v();
	}

	// _____________________________________________________________________________
	// alu_rnd
	//
	void DSP::alu_rnd( TReg56& _alu )
	{
	// 	if( sr_test( SR_S0 ) || sr_test( SR_S1 ) )
	// 		LOG_ERR_NOTIMPLEMENTED( "scaling modes" );

		const int lsb = int(_alu.var & 0x000000ffffff);

		if( lsb > 0x800000 )
		{
			_alu.var += 0x1000000;
		}
		else if( !sr_test(SR_RM) && lsb == 0x800000 && (_alu.var & 0x000001000000) != 0 )
		{
			_alu.var += 0x1000000;
		}
		_alu.var &= 0x00ffffffff000000;	// TODO: wrong?
	}

	// _____________________________________________________________________________
	// readReg
	//
	bool DSP::readReg( EReg _reg, TReg24& _res ) const
	{
		switch( _reg )
		{
		case Reg_N0:	_res = reg.n[0];	break;
		case Reg_N1:	_res = reg.n[1];	break;
		case Reg_N2:	_res = reg.n[2];	break;
		case Reg_N3:	_res = reg.n[3];	break;
		case Reg_N4:	_res = reg.n[4];	break;
		case Reg_N5:	_res = reg.n[5];	break;
		case Reg_N6:	_res = reg.n[6];	break;
		case Reg_N7:	_res = reg.n[7];	break;

		case Reg_R0:	_res = reg.r[0];	break;
		case Reg_R1:	_res = reg.r[1];	break;
		case Reg_R2:	_res = reg.r[2];	break;
		case Reg_R3:	_res = reg.r[3];	break;
		case Reg_R4:	_res = reg.r[4];	break;
		case Reg_R5:	_res = reg.r[5];	break;
		case Reg_R6:	_res = reg.r[6];	break;
		case Reg_R7:	_res = reg.r[7];	break;

		case Reg_M0:	_res = reg.m[0];	break;
		case Reg_M1:	_res = reg.m[1];	break;
		case Reg_M2:	_res = reg.m[2];	break;
		case Reg_M3:	_res = reg.m[3];	break;
		case Reg_M4:	_res = reg.m[4];	break;
		case Reg_M5:	_res = reg.m[5];	break;
		case Reg_M6:	_res = reg.m[6];	break;
		case Reg_M7:	_res = reg.m[7];	break;

		case Reg_A0:	_res = a0();	break;
		case Reg_A1:	_res = a1();	break;
		case Reg_B0:	_res = b0();	break;
		case Reg_B1:	_res = b1();	break;

		case Reg_X0:	_res = x0();	break;
		case Reg_X1:	_res = x1();	break;

		case Reg_Y0:	_res = y0();	break;
		case Reg_Y1:	_res = y1();	break;

		case Reg_PC:	_res = reg.pc;		break;
		case Reg_SR:	_res = reg.sr;		break;
		case Reg_OMR:	_res = reg.omr;		break;
		case Reg_SP:	_res = reg.sp;		break;

		case Reg_LA:	_res = reg.la;		break;
		case Reg_LC:	_res = reg.lc;		break;

		case Reg_ICTR:	_res = reg.ictr;	break;

		case Reg_SSH:	_res = hiword(reg.ss[reg.sc.toWord()]);	break;
		case Reg_SSL:	_res = loword(reg.ss[reg.sc.toWord()]);	break;

		case Reg_CNT1:	_res = reg.cnt1;		break;
		case Reg_CNT2:	_res = reg.cnt2;		break;
		case Reg_CNT3:	_res = reg.cnt3;		break;
		case Reg_CNT4:	_res = reg.cnt4;		break;

		case Reg_VBA:	_res = reg.vba;			break;
		case Reg_SZ:	_res = reg.sz;			break;
		case Reg_EP:	_res = reg.ep;			break;
		case Reg_DCR:	_res = reg.dcr;			break;
		case Reg_BCR:	_res = reg.bcr;			break;
		case Reg_IPRP:	_res.var = iprp();		break;
		case Reg_IPRC:	_res.var = iprc();		break;

		case Reg_AAR0:	_res = reg.aar0;		break;
		case Reg_AAR1:	_res = reg.aar1;		break;
		case Reg_AAR2:	_res = reg.aar2;		break;
		case Reg_AAR3:	_res = reg.aar3;		break;

		case Reg_REPLACE:	_res = reg.replace;	break;
		case Reg_HIT:	_res = reg.hit;			break;
		case Reg_MISS:	_res = reg.miss;		break;
		case Reg_CYC:	_res = reg.cyc;			break;
		default:
			return false;
		}

		return true;
	}
	// _____________________________________________________________________________
	// readReg
	//
	bool DSP::readReg( EReg _reg, TReg56& _res ) const
	{
		switch( _reg )
		{
		case Reg_A:		_res = reg.a;	return true;
		case Reg_B:		_res = reg.b;	return true;
		}
		return false;
	}
	// _____________________________________________________________________________
	// readReg
	//
	bool DSP::readReg( EReg _reg, TReg8& _res ) const
	{
		switch( _reg )
		{
		case Reg_A2:	_res = a2();	return true;
		case Reg_B2:	_res = b2();	return true;
		}
		return false;
	}
	// _____________________________________________________________________________
	// readReg
	//
	bool DSP::readReg( EReg _reg, TReg48& _res ) const
	{
		switch( _reg )
		{
		case Reg_X:		_res = reg.x;	return true;
		case Reg_Y:		_res = reg.y;	return true;
		}
		return false;
	}
	// _____________________________________________________________________________
	// readReg
	//
	bool DSP::readReg( EReg _reg, TReg5& _res ) const
	{
		if( _reg == Reg_SC )
		{
			_res = reg.sc;
			return true;
		}
		return false;
	}

	// _____________________________________________________________________________
	// readRegToInt
	//
	bool DSP::readRegToInt( EReg _reg, int64_t& _dst )
	{
		switch( g_regBitCount[_reg] )
		{
		case 56:
			{
				TReg56 dst;
				if( !readReg(_reg, dst) )
					return false;
				_dst = dst.var;
			};
			break;
		case 48:
			{
				TReg48 dst;
				if( !readReg(_reg, dst) )
					return false;
				_dst = dst.var;
			};
			break;
		case 24:
			{
				TReg24 dst;
				if( !readReg(_reg, dst) )
					return false;
				_dst = dst.var;
			};
			break;
		case 8:
			{
				TReg8 dst;
				if( !readReg(_reg, dst) )
					return false;
				_dst = dst.var;
			};
			break;
		case 5:
			{
				TReg5 dst;
				if( !readReg(_reg, dst) )
					return false;
				_dst = dst.var;
			};
			break;
		default:
			return false;
		}
		return true;
	}
	// _____________________________________________________________________________
	// writeReg
	//
	bool DSP::writeReg( EReg _reg, const TReg24& _res )
	{
		assert( (_res.var & 0xff000000) == 0 );
			
		switch( _reg )
		{
		case Reg_N0:	reg.n[0] = _res;	break;
		case Reg_N1:	reg.n[1] = _res;	break;
		case Reg_N2:	reg.n[2] = _res;	break;
		case Reg_N3:	reg.n[3] = _res;	break;
		case Reg_N4:	reg.n[4] = _res;	break;
		case Reg_N5:	reg.n[5] = _res;	break;
		case Reg_N6:	reg.n[6] = _res;	break;
		case Reg_N7:	reg.n[7] = _res;	break;
							
		case Reg_R0:	reg.r[0] = _res;	break;
		case Reg_R1:	reg.r[1] = _res;	break;
		case Reg_R2:	reg.r[2] = _res;	break;
		case Reg_R3:	reg.r[3] = _res;	break;
		case Reg_R4:	reg.r[4] = _res;	break;
		case Reg_R5:	reg.r[5] = _res;	break;
		case Reg_R6:	reg.r[6] = _res;	break;
		case Reg_R7:	reg.r[7] = _res;	break;
							
		case Reg_M0:	reg.m[0] = _res;	break;
		case Reg_M1:	reg.m[1] = _res;	break;
		case Reg_M2:	reg.m[2] = _res;	break;
		case Reg_M3:	reg.m[3] = _res;	break;
		case Reg_M4:	reg.m[4] = _res;	break;
		case Reg_M5:	reg.m[5] = _res;	break;
		case Reg_M6:	reg.m[6] = _res;	break;
		case Reg_M7:	reg.m[7] = _res;	break;
							
		case Reg_A0:	a0(_res);		break;
		case Reg_A1:	a1(_res);		break;
		case Reg_B0:	b0(_res);		break;
		case Reg_B1:	b1(_res);		break;
							
		case Reg_X0:	x0(_res);		break;
		case Reg_X1:	x1(_res);		break;
							
		case Reg_Y0:	y0(_res);		break;
		case Reg_Y1:	y1(_res);		break;

		case Reg_PC:	setPC(_res);	break;

		default:
			assert( 0 && "unknown register" );
			return false;
		}

		return true;
	}

	// _____________________________________________________________________________
	// writeReg
	//
	bool DSP::writeReg( EReg _reg, const TReg56& _val )
	{
		switch( _reg )
		{
		case Reg_A:		reg.a = _val;		return true;
		case Reg_B:		reg.b = _val;		return true;
		}
		assert( 0 && "unknown register" );
		return false;
	}

	// _____________________________________________________________________________
	// readDebugRegs
	//
	void DSP::readDebugRegs( dsp56k::SRegs& _regs ) const
	{
		readReg( Reg_X, _regs.x);
		readReg( Reg_Y, _regs.y);

		readReg( Reg_A, _regs.a);
		readReg( Reg_B, _regs.b);

		readReg( Reg_X0, _regs.x0);		
		readReg( Reg_X1, _regs.x1);		

		readReg( Reg_Y0, _regs.y0);		
		readReg( Reg_Y1, _regs.y1);		

		readReg( Reg_A0, _regs.a0);		
		readReg( Reg_A1, _regs.a1);		
		readReg( Reg_A2, _regs.a2);		

		readReg( Reg_B0, _regs.b0);		
		readReg( Reg_B1, _regs.b1);		
		readReg( Reg_B2, _regs.b2);		

		readReg( Reg_PC, _regs.pc);		
		readReg( Reg_SR, _regs.sr);		
		readReg( Reg_OMR, _regs.omr);	

		readReg( Reg_LA, _regs.la);		
		readReg( Reg_LC, _regs.lc);		

		readReg( Reg_SSH, _regs.ssh);	
		readReg( Reg_SSL, _regs.ssl);	
		readReg( Reg_SP, _regs.sp);		

		readReg( Reg_EP, _regs.ep);		
		readReg( Reg_SZ, _regs.sz);		
		readReg( Reg_SC, _regs.sc);		
		readReg( Reg_VBA, _regs.vba);	

		readReg( Reg_IPRC, _regs.iprc);	
		readReg( Reg_IPRP, _regs.iprp);	
		readReg( Reg_BCR, _regs.bcr);	
		readReg( Reg_DCR, _regs.dcr);	

		readReg( Reg_AAR0, _regs.aar0);	
		readReg( Reg_AAR1, _regs.aar1);	
		readReg( Reg_AAR2, _regs.aar2);	
		readReg( Reg_AAR3, _regs.aar3);	

		readReg( Reg_R0, _regs.r0);		
		readReg( Reg_R1, _regs.r1);		
		readReg( Reg_R2, _regs.r2);		
		readReg( Reg_R3, _regs.r3);		
		readReg( Reg_R4, _regs.r4);		
		readReg( Reg_R5, _regs.r5);		
		readReg( Reg_R6, _regs.r6);		
		readReg( Reg_R7, _regs.r7);		

		readReg( Reg_N0, _regs.n0);		
		readReg( Reg_N1, _regs.n1);		
		readReg( Reg_N2, _regs.n2);		
		readReg( Reg_N3, _regs.n3);		
		readReg( Reg_N4, _regs.n4);		
		readReg( Reg_N5, _regs.n5);		
		readReg( Reg_N6, _regs.n6);		
		readReg( Reg_N7, _regs.n7);		

		readReg( Reg_M0, _regs.m0);		
		readReg( Reg_M1, _regs.m1);		
		readReg( Reg_M2, _regs.m2);		
		readReg( Reg_M3, _regs.m3);		
		readReg( Reg_M4, _regs.m4);		
		readReg( Reg_M5, _regs.m5);		
		readReg( Reg_M6, _regs.m6);		
		readReg( Reg_M7, _regs.m7);		

		readReg( Reg_HIT, _regs.hit);		
		readReg( Reg_MISS, _regs.miss);		
		readReg( Reg_REPLACE, _regs.replace);	
		readReg( Reg_CYC, _regs.cyc);		
		readReg( Reg_ICTR, _regs.ictr);		
		readReg( Reg_CNT1, _regs.cnt1);		
		readReg( Reg_CNT2, _regs.cnt2);		
		readReg( Reg_CNT3, _regs.cnt3);		
		readReg( Reg_CNT4, _regs.cnt4);
	}

	// _____________________________________________________________________________
	// getASM
	//
	const char* DSP::getASM(const TWord _wordA, const TWord _wordB)
	{
	#ifdef _DEBUG
		unsigned long ops[3];
		ops[0] = _wordA;
		ops[1] = _wordB;
		ops[2] = 0;
		disassemble( m_asm, ops, reg.sr.var, reg.omr.var );
	#endif
		return m_asm;
	}

	// _____________________________________________________________________________
	// memWrite
	//
	bool DSP::memWrite( EMemArea _area, TWord _offset, TWord _value )
	{
		const auto res = mem.dspWrite( _area, _offset, _value );

//		if(_area == MemArea_P)	does not work for external memory
			m_opcodeCache[_offset] = ResolveCache;

		return res;
	}

	bool DSP::memWritePeriph( EMemArea _area, TWord _offset, TWord _value )
	{
		perif[_area]->write(_offset, _value );
		return true;
	}
	bool DSP::memWritePeriphFFFF80( EMemArea _area, TWord _offset, TWord _value )
	{
		return memWritePeriph( _area, _offset + 0xffff80, _value );
	}
	bool DSP::memWritePeriphFFFFC0( EMemArea _area, TWord _offset, TWord _value )
	{
		return memWritePeriph( _area, _offset + 0xffffc0, _value );
	}

	// _____________________________________________________________________________
	// memRead
	//
	dsp56k::TWord DSP::memRead( EMemArea _area, TWord _offset ) const
	{
		// app may access the instruction cache on the DSP 56362? not sure about this, not clear for me in the docs

	// 	if( _area == MemArea_P && sr_test(SR_CE) )
	// 	{
	// 		const bool ms = (reg.omr.var & OMR_MS) != 0;
	// 
	// 		if( !ms )
	// 		{
	// 			if( _offset >= 0x000800 && _offset < 0x000c00 )
	// 			{
	// 				return cache.readMemory( _offset - 0x000800 );
	// 			}
	// 		}
	// 		else
	// 		{
	// 			if( _offset >= 0x001000 && _offset < 0x001400 )
	// 				return cache.readMemory( _offset - 0x001000 );
	// 		}
	// 	}

		return mem.get( _area, _offset );
	}

	void DSP::memRead2(EMemArea _area, TWord _offset, TWord& _wordA, TWord& _wordB) const
	{
		mem.get2(_area, _offset, _wordA, _wordB);
	}

	TWord DSP::memReadPeriph(EMemArea _area, TWord _offset) const
	{
		return perif[_area]->read(_offset);
	}
	TWord DSP::memReadPeriphFFFF80(EMemArea _area, TWord _offset) const
	{
		return memReadPeriph(_area, _offset + 0xffff80);
	}
	TWord DSP::memReadPeriphFFFFC0(EMemArea _area, TWord _offset) const
	{
		return memReadPeriph(_area, _offset + 0xffffc0);
	}

	// _____________________________________________________________________________
	// alu_abs
	//
	void DSP::alu_abs( bool ab )
	{
		TReg56& d = ab ? reg.b : reg.a;

		TInt64 d64 = d.signextend<TInt64>();

		d64 = d64 < 0 ? -d64 : d64;

		d.var = d64 & 0xffffffffffffff;

		sr_s_update();
		sr_e_update(d);
		sr_u_update(d);
		sr_n_update(d);
		sr_z_update(d);
	//	sr_v_update(d);
		sr_l_update_by_v();
	}

	void DSP::alu_tfr(const bool ab, const TReg56& src)
	{
		auto& d = ab ? reg.b : reg.a;
		d = src;
		sr_s_update();
		sr_v_update(src.var, d);
	}

	void DSP::alu_tst(bool ab)
	{
		const bool c = sr_test(SR_C);
		alu_cmp(ab, TReg56(0), false);

		sr_clear(SR_V);		// "always cleared"
		sr_toggle(SR_C,c);	// "unchanged by the instruction" so reset to previous state
	}

	void DSP::alu_neg(bool ab)
	{
		TReg56& d = ab ? reg.b : reg.a;

		auto d64 = d.signextend<TInt64>();
		d64 = -d64;
		
		d.var = d64 & 0x00ffffffffffffff;

		sr_s_update();
		sr_e_update(d);
		sr_u_update(d);
		sr_n_update(d);
		sr_z_update(d);
	//	TODO: how to update v? test in sim		sr_v_update(d);
		sr_l_update_by_v();
	}

	void DSP::alu_not(const bool ab)
	{
		auto& d = ab ? reg.b.var : reg.a.var;

		const auto masked = ~d & 0x00ffffff000000;

		d &= 0xff000000ffffff;
		d |= masked;

		sr_toggle(SR_N, bitvalue<uint64_t, 47>(d));		// Set if bit 47 of the result is set
		sr_toggle(SR_Z, masked != 0);					// Set if bits 47?24 of the result are 0
		sr_clear(SR_V);									// Always cleared
		sr_s_update();									// Changed according to the standard definition
		sr_l_update_by_v();								// Changed according to the standard definition
	}

	// _____________________________________________________________________________
	// save
	//
	bool DSP::save( FILE* _file ) const
	{
		fwrite( &reg, sizeof(reg), 1, _file );
		fwrite( &pcCurrentInstruction, 1, 1, _file );
		fwrite( m_asm, sizeof(m_asm), 1, _file );
		fwrite( &cache, sizeof(cache), 1, _file );

		return true;
	}

	// _____________________________________________________________________________
	// load
	//
	bool DSP::load( FILE* _file )
	{
		fread( &reg, sizeof(reg), 1, _file );
		fread( &pcCurrentInstruction, 1, 1, _file );
		fread( m_asm, sizeof(m_asm), 1, _file );
		fread( &cache, sizeof(cache), 1, _file );

		return true;
	}

	void DSP::injectInterrupt(InterruptVectorAddress _interruptVectorAddress)
	{
		m_pendingInterrupts.push_back(_interruptVectorAddress);
	}

	void DSP::clearOpcodeCache()
	{
		m_opcodeCache.clear();
		m_opcodeCache.resize(mem.size(), ResolveCache);		
	}

	void DSP::dumpRegisters()
	{
		auto logReg = [this](EReg _reg, int _width)
		{
			std::stringstream ss;
			int64_t v;
			if(!readRegToInt(_reg, v))
			{
				readRegToInt(_reg, v);
				assert(false);
			}
			const bool c = m_prevRegStates[_reg].val != v;
			if(c)
				ss << '{';
			if(_width > 0)
				ss << '$' << std::hex << std::setfill('0') << std::setw(_width) << v;
			else
				ss << std::setfill('0') << std::setw(-_width) << v;
			if(c)
				ss << '}';
			const std::string res(ss.str());
			return res;
		};

		LOGF("   x=       " << logReg(Reg_X, 12) << "    y=       " << logReg(Reg_Y, 12));
		LOGF("   a=     " << logReg(Reg_A, 14) << "    b=     " << logReg(Reg_B, 14));
		LOGF("               x1=" << logReg(Reg_X1, 6) << "   x0=" << logReg(Reg_X0, 6) << "   r7=" << logReg(Reg_R7,6) << " n7=" << logReg(Reg_N7,6) << " m7=" << logReg(Reg_M7,6));
		LOGF("               y1=" << logReg(Reg_Y1, 6) << "   y0=" << logReg(Reg_Y0, 6) << "   r6=" << logReg(Reg_R6,6) << " n6=" << logReg(Reg_N6,6) << " m6=" << logReg(Reg_M6,6));
		LOGF("  a2=    " << logReg(Reg_A2, 2) << "   a1=" << logReg(Reg_A1, 6) << "   a0=" << logReg(Reg_A0,6) << "   r5=" << logReg(Reg_R5,6) << " n5=" << logReg(Reg_N5,6) << " m5=" << logReg(Reg_M5,6));
		LOGF("  b2=    " << logReg(Reg_B2, 2) << "   b1=" << logReg(Reg_B1, 6) << "   b0=" << logReg(Reg_B0,6) << "   r4=" << logReg(Reg_R4,6) << " n4=" << logReg(Reg_N4,6) << " m4=" << logReg(Reg_M4,6));
		LOGF("                                         r3=" << logReg(Reg_R3,6) << " n3=" << logReg(Reg_N3,6) << " m3=" << logReg(Reg_M3,6));
		LOGF("  pc=" << logReg(Reg_PC, 6) << "   sr=" << logReg(Reg_SR, 6) << "  omr=" << logReg(Reg_OMR,6) << "   r2=" << logReg(Reg_R2,6) << " n2=" << logReg(Reg_N2,6) << " m2=" << logReg(Reg_M2,6));
		LOGF("  la=" << logReg(Reg_LA, 6) << "   lc=" << logReg(Reg_LC, 6) << "                r1=" << logReg(Reg_R1,6) << " n1=" << logReg(Reg_N1,6) << " m1=" << logReg(Reg_M1,6));
		LOGF(" ssh=" << logReg(Reg_SSH, 6) << "  ssl=" << logReg(Reg_SR, 6) << "   sp=" << logReg(Reg_OMR,6) << "   r0=" << logReg(Reg_R0,6) << " n0=" << logReg(Reg_N0,6) << " m0=" << logReg(Reg_M0,6));
		LOGF("  ep=" << logReg(Reg_EP, 6) << "   sz=" << logReg(Reg_SZ, 6) << "   sc=" << logReg(Reg_SC,6) << "  vba=" << logReg(Reg_VBA,6));
		LOGF("iprc=" << logReg(Reg_IPRC, 6) << " iprp=" << logReg(Reg_IPRP, 6) << "  bcr=" << logReg(Reg_BCR,6) << "  dcr=" << logReg(Reg_DCR,6));
		LOGF("aar0=" << logReg(Reg_AAR0, 6) << " aar1=" << logReg(Reg_AAR1, 6) << " aar2=" << logReg(Reg_AAR2,6) << " aar3=" << logReg(Reg_AAR3,6));
		LOGF("  hit=   " << logReg(Reg_HIT, -6) << "   miss=   " << logReg(Reg_MISS, -6) << " replace=" << logReg(Reg_REPLACE,-6));
		LOGF("  cyc=   " << logReg(Reg_CYC, -6) << "   ictr=   " << logReg(Reg_ICTR, -6));
		LOGF(" cnt1=   " << logReg(Reg_CNT1, -6) << "   cnt2=   " << logReg(Reg_CNT2, -6) << " cnt3=   " << logReg(Reg_CNT2,-6) << "  cnt4=   " << logReg(Reg_CNT3,-6));
/*
   x=       $000000000000    y=       $000000000000
   a=     $00000000000051    b=     $00000000000000
               x1=$000000   x0=$000000   r7=$000000 n7=$000000 m7=$ffffff
               y1=$000000   y0=$000000   r6=$000000 n6=$000000 m6=$ffffff
  a2=    $00   a1=$000000   a0=$000051   r5=$000000 n5=$000000 m5=$ffffff
  b2=    $00   b1=$000000   b0=$000000   r4=$000000 n4=$000000 m4=$ffffff
                                         r3=$000000 n3=$000000 m3=$ffffff
  pc=$000102   sr=$c00300  omr=$00030e   r2=$000000 n2=$000000 m2=$ffffff
  la=$ffffff   lc=$000000                r1=$000100 n1=$000000 m1=$ffffff
 ssh=$000000  ssl=$000000   sp=$000000   r0=$000151 n0=$000000 m0=$ffffff
  ep=$000000   sz=$000000   sc=$000000  vba=$000000
iprc=$000000 iprp=$000000  bcr=$212421  dcr=$000000
aar0=$000008 aar1=$000000 aar2=$000000 aar3=$000000
  hit=   000000   miss=   000000 replace=000000
  cyc=   001027   ictr=   000259
 cnt1=   000000   cnt2=   000000 cnt3=   000000  cnt4=   000000
*/
	}
}
