// license:BSD-3-Clause
// copyright-holders:Karl Stenerud
/***************************************************************************

    m68kfpu.cpp

    Stub replacement for the 68881/68882 FPU emulation.  The Philips CD-i
    machines in this core use the FPU-less SCC68070 (and 8-bit MC68HC05
    MCUs); m_has_fpu is never set, so every FPU path was runtime-dead and
    existed only to satisfy the m68kops dispatch tables.  The full
    implementation (and its 3rdparty/softfloat floatx80 arithmetic) has
    been removed; F-line coprocessor opcodes take the 1111 exception, as
    they do on real FPU-less parts.

    The generic effective-address helpers below are kept verbatim from
    the original file: they are also used by the PMMU code inlined into
    the opcode handlers (m68kmmu.h).

***************************************************************************/

#include "emu.h"
#include "m68000.h"

u8 m68000_base_device::READ_EA_8(int ea)
{
	int mode = (ea >> 3) & 0x7;
	int reg = (ea & 0x7);

	switch (mode)
	{
		case 0:     // Dn
		{
			return REG_D()[reg];
		}
		case 1: // An
		{
			return REG_A()[reg];
		}
		case 2:     // (An)
		{
			u32 ea = REG_A()[reg];
			return m68ki_read_8(ea);
		}
		case 3:     // (An)+
		{
			u32 ea = EA_AY_PI_8();
			return m68ki_read_8(ea);
		}
		case 4:     // -(An)
		{
			u32 ea = EA_AY_PD_8();
			return m68ki_read_8(ea);
		}
		case 5:     // (d16, An)
		{
			u32 ea = EA_AY_DI_8();
			return m68ki_read_8(ea);
		}
		case 6:     // (An) + (Xn) + d8
		{
			u32 ea = EA_AY_IX_8();
			return m68ki_read_8(ea);
		}
		case 7:
		{
			switch (reg)
			{
				case 0:     // (xxx).W
				{
					u32 ea = OPER_I_16();
					return m68ki_read_8(ea);
				}
				case 1:     // (xxx).L
				{
					u32 d1 = OPER_I_16();
					u32 d2 = OPER_I_16();
					u32 ea = (d1 << 16) | d2;
					return m68ki_read_8(ea);
				}
				case 2:     // (d16, PC)
				{
					u32 ea = EA_PCDI_8();
					return m68ki_read_8(ea);
				}
				case 3:     // (PC) + (Xn) + d8
				{
					u32 ea =  EA_PCIX_8();
					return m68ki_read_8(ea);
				}
				case 4:     // #<data>
				{
					return  OPER_I_8();
				}
				default:    fatalerror("M68kFPU: READ_EA_8: unhandled mode %d, reg %d at %08X\n", mode, reg, m_pc);
			}
			break;
		}
		default:    fatalerror("M68kFPU: READ_EA_8: unhandled mode %d, reg %d at %08X\n", mode, reg, m_pc);
	}

	return 0;
}

u16 m68000_base_device::READ_EA_16(int ea)
{
	int mode = (ea >> 3) & 0x7;
	int reg = (ea & 0x7);

	switch (mode)
	{
		case 0:     // Dn
		{
			return (u16)(REG_D()[reg]);
		}
		case 1:     // An
		{
			return (u16)REG_A()[reg];
		}
		case 2:     // (An)
		{
			u32 ea = REG_A()[reg];
			return m68ki_read_16(ea);
		}
		case 3:     // (An)+
		{
			u32 ea = EA_AY_PI_16();
			return m68ki_read_16(ea);
		}
		case 4:     // -(An)
		{
			u32 ea = EA_AY_PD_16();
			return m68ki_read_16(ea);
		}
		case 5:     // (d16, An)
		{
			u32 ea = EA_AY_DI_16();
			return m68ki_read_16(ea);
		}
		case 6:     // (An) + (Xn) + d8
		{
			u32 ea = EA_AY_IX_16();
			return m68ki_read_16(ea);
		}
		case 7:
		{
			switch (reg)
			{
				case 0:     // (xxx).W
				{
					u32 ea = OPER_I_16();
					return m68ki_read_16(ea);
				}
				case 1:     // (xxx).L
				{
					u32 d1 = OPER_I_16();
					u32 d2 = OPER_I_16();
					u32 ea = (d1 << 16) | d2;
					return m68ki_read_16(ea);
				}
				case 2:     // (d16, PC)
				{
					u32 ea = EA_PCDI_16();
					return m68ki_read_16(ea);
				}
				case 3:     // (PC) + (Xn) + d8
				{
					u32 ea =  EA_PCIX_16();
					return m68ki_read_16(ea);
				}
				case 4:     // #<data>
				{
					return OPER_I_16();
				}

				default:    fatalerror("M68kFPU: READ_EA_16: unhandled mode %d, reg %d at %08X\n", mode, reg, m_pc);
			}
			break;
		}
		default:    fatalerror("M68kFPU: READ_EA_16: unhandled mode %d, reg %d at %08X\n", mode, reg, m_pc);
	}

	return 0;
}

u32 m68000_base_device::READ_EA_32(int ea)
{
	int mode = (ea >> 3) & 0x7;
	int reg = (ea & 0x7);

	switch (mode)
	{
		case 0:     // Dn
		{
			return REG_D()[reg];
		}
		case 1:     // An
		{
			return REG_A()[reg];
		}
		case 2:     // (An)
		{
			u32 ea = REG_A()[reg];
			return m68ki_read_32(ea);
		}
		case 3:     // (An)+
		{
			u32 ea = EA_AY_PI_32();
			return m68ki_read_32(ea);
		}
		case 4:     // -(An)
		{
			u32 ea = EA_AY_PD_32();
			return m68ki_read_32(ea);
		}
		case 5:     // (d16, An)
		{
			u32 ea = EA_AY_DI_32();
			return m68ki_read_32(ea);
		}
		case 6:     // (An) + (Xn) + d8
		{
			u32 ea = EA_AY_IX_32();
			return m68ki_read_32(ea);
		}
		case 7:
		{
			switch (reg)
			{
				case 0:     // (xxx).W
				{
					u32 ea = OPER_I_16();
					return m68ki_read_32(ea);
				}
				case 1:     // (xxx).L
				{
					u32 d1 = OPER_I_16();
					u32 d2 = OPER_I_16();
					u32 ea = (d1 << 16) | d2;
					return m68ki_read_32(ea);
				}
				case 2:     // (d16, PC)
				{
					u32 ea = EA_PCDI_32();
					return m68ki_read_32(ea);
				}
				case 3:     // (PC) + (Xn) + d8
				{
					u32 ea =  EA_PCIX_32();
					return m68ki_read_32(ea);
				}
				case 4:     // #<data>
				{
					return  OPER_I_32();
				}
				default:    fatalerror("M68kFPU: READ_EA_32: unhandled mode %d, reg %d at %08X\n", mode, reg, m_pc);
			}
			break;
		}
		default:    fatalerror("M68kFPU: READ_EA_32: unhandled mode %d, reg %d at %08X\n", mode, reg, m_pc);
	}
	return 0;
}

u64 m68000_base_device::READ_EA_64(int ea)
{
	int mode = (ea >> 3) & 0x7;
	int reg = (ea & 0x7);
	u32 h1, h2;

	switch (mode)
	{
		case 2:     // (An)
		{
			u32 ea = REG_A()[reg];
			h1 = m68ki_read_32(ea+0);
			h2 = m68ki_read_32(ea+4);
			return  (u64)(h1) << 32 | (u64)(h2);
		}
		case 3:     // (An)+
		{
			u32 ea = REG_A()[reg];
			REG_A()[reg] += 8;
			h1 = m68ki_read_32(ea+0);
			h2 = m68ki_read_32(ea+4);
			return  (u64)(h1) << 32 | (u64)(h2);
		}
		case 4:     // -(An)
		{
			u32 ea = REG_A()[reg]-8;
			REG_A()[reg] -= 8;
			h1 = m68ki_read_32(ea+0);
			h2 = m68ki_read_32(ea+4);
			return  (u64)(h1) << 32 | (u64)(h2);
		}
		case 5:     // (d16, An)
		{
			u32 ea = EA_AY_DI_32();
			h1 = m68ki_read_32(ea+0);
			h2 = m68ki_read_32(ea+4);
			return  (u64)(h1) << 32 | (u64)(h2);
		}
		case 6:     // (An) + (Xn) + d8
		{
			u32 ea = EA_AY_IX_32();
			h1 = m68ki_read_32(ea+0);
			h2 = m68ki_read_32(ea+4);
			return  (u64)(h1) << 32 | (u64)(h2);
		}
		case 7:
		{
			switch (reg)
			{
				case 1:     // (xxx).L
				{
					u32 d1 = OPER_I_16();
					u32 d2 = OPER_I_16();
					u32 ea = (d1 << 16) | d2;
					return (u64)(m68ki_read_32(ea)) << 32 | (u64)(m68ki_read_32(ea+4));
				}
				case 3:     // (PC) + (Xn) + d8
				{
					u32 ea =  EA_PCIX_32();
					h1 = m68ki_read_32(ea+0);
					h2 = m68ki_read_32(ea+4);
					return  (u64)(h1) << 32 | (u64)(h2);
				}
				case 4:     // #<data>
				{
					h1 = OPER_I_32();
					h2 = OPER_I_32();
					return  (u64)(h1) << 32 | (u64)(h2);
				}
				case 2:     // (d16, PC)
				{
					u32 ea = EA_PCDI_32();
					h1 = m68ki_read_32(ea+0);
					h2 = m68ki_read_32(ea+4);
					return  (u64)(h1) << 32 | (u64)(h2);
				}
				default:    fatalerror("M68kFPU: READ_EA_64: unhandled mode %d, reg %d at %08X\n", mode, reg, m_pc);
			}
			break;
		}
		default:    fatalerror("M68kFPU: READ_EA_64: unhandled mode %d, reg %d at %08X\n", mode, reg, m_pc);
	}

	return 0;
}

void m68000_base_device::WRITE_EA_8(int ea, u8 data)
{
	int mode = (ea >> 3) & 0x7;
	int reg = (ea & 0x7);

	switch (mode)
	{
		case 0:     // Dn
		{
			REG_D()[reg] = data;
			break;
		}
		case 2:     // (An)
		{
			u32 ea = REG_A()[reg];
			m68ki_write_8(ea, data);
			break;
		}
		case 3:     // (An)+
		{
			u32 ea = EA_AY_PI_8();
			m68ki_write_8(ea, data);
			break;
		}
		case 4:     // -(An)
		{
			u32 ea = EA_AY_PD_8();
			m68ki_write_8(ea, data);
			break;
		}
		case 5:     // (d16, An)
		{
			u32 ea = EA_AY_DI_8();
			m68ki_write_8(ea, data);
			break;
		}
		case 6:     // (An) + (Xn) + d8
		{
			u32 ea = EA_AY_IX_8();
			m68ki_write_8(ea, data);
			break;
		}
		case 7:
		{
			switch (reg)
			{
				case 1:     // (xxx).B
				{
					u32 d1 = OPER_I_16();
					u32 d2 = OPER_I_16();
					u32 ea = (d1 << 16) | d2;
					m68ki_write_8(ea, data);
					break;
				}
				case 2:     // (d16, PC)
				{
					u32 ea = EA_PCDI_16();
					m68ki_write_8(ea, data);
					break;
				}
				default:    fatalerror("M68kFPU: WRITE_EA_8: unhandled mode %d, reg %d at %08X\n", mode, reg, m_pc);
			}
			break;
		}
		default:    fatalerror("M68kFPU: WRITE_EA_8: unhandled mode %d, reg %d, data %08X at %08X\n", mode, reg, data, m_pc);
	}
}

void m68000_base_device::WRITE_EA_16(int ea, u16 data)
{
	int mode = (ea >> 3) & 0x7;
	int reg = (ea & 0x7);

	switch (mode)
	{
		case 0:     // Dn
		{
			REG_D()[reg] = data;
			break;
		}
		case 2:     // (An)
		{
			u32 ea = REG_A()[reg];
			m68ki_write_16(ea, data);
			break;
		}
		case 3:     // (An)+
		{
			u32 ea = EA_AY_PI_16();
			m68ki_write_16(ea, data);
			break;
		}
		case 4:     // -(An)
		{
			u32 ea = EA_AY_PD_16();
			m68ki_write_16(ea, data);
			break;
		}
		case 5:     // (d16, An)
		{
			u32 ea = EA_AY_DI_16();
			m68ki_write_16(ea, data);
			break;
		}
		case 6:     // (An) + (Xn) + d8
		{
			u32 ea = EA_AY_IX_16();
			m68ki_write_16(ea, data);
			break;
		}
		case 7:
		{
			switch (reg)
			{
				case 1:     // (xxx).W
				{
					u32 d1 = OPER_I_16();
					u32 d2 = OPER_I_16();
					u32 ea = (d1 << 16) | d2;
					m68ki_write_16(ea, data);
					break;
				}
				case 2:     // (d16, PC)
				{
					u32 ea = EA_PCDI_16();
					m68ki_write_16(ea, data);
					break;
				}
				default:    fatalerror("M68kFPU: WRITE_EA_16: unhandled mode %d, reg %d at %08X\n", mode, reg, m_pc);
			}
			break;
		}
		default:    fatalerror("M68kFPU: WRITE_EA_16: unhandled mode %d, reg %d, data %08X at %08X\n", mode, reg, data, m_pc);
	}
}

void m68000_base_device::WRITE_EA_32(int ea, u32 data)
{
	int mode = (ea >> 3) & 0x7;
	int reg = (ea & 0x7);

	switch (mode)
	{
		case 0:     // Dn
		{
			REG_D()[reg] = data;
			break;
		}
		case 1:     // An
		{
			REG_A()[reg] = data;
			break;
		}
		case 2:     // (An)
		{
			u32 ea = REG_A()[reg];
			m68ki_write_32(ea, data);
			break;
		}
		case 3:     // (An)+
		{
			u32 ea = EA_AY_PI_32();
			m68ki_write_32(ea, data);
			break;
		}
		case 4:     // -(An)
		{
			u32 ea = EA_AY_PD_32();
			m68ki_write_32(ea, data);
			break;
		}
		case 5:     // (d16, An)
		{
			u32 ea = EA_AY_DI_32();
			m68ki_write_32(ea, data);
			break;
		}
		case 6:     // (An) + (Xn) + d8
		{
			u32 ea = EA_AY_IX_32();
			m68ki_write_32(ea, data);
			break;
		}
		case 7:
		{
			switch (reg)
			{
				case 0:     // (xxx).W
				{
					u32 ea = OPER_I_16();
					m68ki_write_32(ea, data);
					break;
				}
				case 1:     // (xxx).L
				{
					u32 d1 = OPER_I_16();
					u32 d2 = OPER_I_16();
					u32 ea = (d1 << 16) | d2;
					m68ki_write_32(ea, data);
					break;
				}
				case 2:     // (d16, PC)
				{
					u32 ea = EA_PCDI_32();
					m68ki_write_32(ea, data);
					break;
				}
				default:    fatalerror("M68kFPU: WRITE_EA_32: unhandled mode %d, reg %d at %08X\n", mode, reg, m_pc);
			}
			break;
		}
		default:    fatalerror("M68kFPU: WRITE_EA_32: unhandled mode %d, reg %d, data %08X at %08X\n", mode, reg, data, m_pc);
	}
}

void m68000_base_device::WRITE_EA_64(int ea, u64 data)
{
	int mode = (ea >> 3) & 0x7;
	int reg = (ea & 0x7);

	switch (mode)
	{
		case 2:     // (An)
		{
			u32 ea = REG_A()[reg];
			m68ki_write_32(ea, (u32)(data >> 32));
			m68ki_write_32(ea+4, (u32)(data));
			break;
		}
		case 3:     // (An)+
		{
			u32 ea = REG_A()[reg];
			REG_A()[reg] += 8;
			m68ki_write_32(ea+0, (u32)(data >> 32));
			m68ki_write_32(ea+4, (u32)(data));
			break;
		}
		case 4:     // -(An)
		{
			u32 ea;
			REG_A()[reg] -= 8;
			ea = REG_A()[reg];
			m68ki_write_32(ea+0, (u32)(data >> 32));
			m68ki_write_32(ea+4, (u32)(data));
			break;
		}
		case 5:     // (d16, An)
		{
			u32 ea = EA_AY_DI_32();
			m68ki_write_32(ea+0, (u32)(data >> 32));
			m68ki_write_32(ea+4, (u32)(data));
			break;
		}
		case 6:     // (An) + (Xn) + d8
		{
			u32 ea = EA_AY_IX_32();
			m68ki_write_32(ea+0, (u32)(data >> 32));
			m68ki_write_32(ea+4, (u32)(data));
			break;
		}
		case 7:
		{
			switch (reg)
			{
				case 1:     // (xxx).L
				{
					u32 d1 = OPER_I_16();
					u32 d2 = OPER_I_16();
					u32 ea = (d1 << 16) | d2;
					m68ki_write_32(ea+0, (u32)(data >> 32));
					m68ki_write_32(ea+4, (u32)(data));
					break;
				}
				case 2:     // (d16, PC)
				{
					u32 ea = EA_PCDI_32();
					m68ki_write_32(ea+0, (u32)(data >> 32));
					m68ki_write_32(ea+4, (u32)(data));
					break;
				}
				default:    fatalerror("M68kFPU: WRITE_EA_64: unhandled mode %d, reg %d at %08X\n", mode, reg, m_pc);
			}
			break;
		}
		default:    fatalerror("M68kFPU: WRITE_EA_64: unhandled mode %d, reg %d, data %08X%08X at %08X\n", mode, reg, (u32)(data >> 32), (u32)(data), m_pc);
	}
}


/* FPU opcode entry points: no FPU on any CPU in this core */

void m68000_base_device::m68040_fpu_op0()
{
	m68ki_exception_1111();
}

void m68000_base_device::m68040_fpu_op1()
{
	m68ki_exception_1111();
}

void m68000_base_device::m68881_ftrap()
{
	m68ki_exception_1111();
}
