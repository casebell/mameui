// license:BSD-3-Clause
// copyright-holders:Joakim Larsson Edstrom
/*
 * nld_74164.cpp
 *
 * Thanks to the 74161 work of Ryan and the huge Netlist effort by Couriersud
 * implementing this was simple.
 *
 */

#include "nld_74164.h"
#include "netlist/nl_base.h"

namespace netlist
{
	namespace devices
	{
	NETLIB_OBJECT(74164)
	{
		NETLIB_CONSTRUCTOR(74164)
		, m_A(*this, "A")
		, m_B(*this, "B")
		, m_CLRQ(*this, "CLRQ")
		, m_CLK(*this, "CLK")
		, m_cnt(*this, "m_cnt", 0)
		, m_last_CLK(*this, "m_last_CLK", 0)
		, m_Q(*this, {"QA", "QB", "QC", "QD", "QE", "QF", "QG", "QH"})
		, m_power_pins(*this)
		{
		}

		NETLIB_RESETI()
		{
			m_cnt = 0;
			m_last_CLK = 0;
		}

		NETLIB_UPDATEI();

		friend class NETLIB_NAME(74164_dip);
	private:
		logic_input_t m_A;
		logic_input_t m_B;
		logic_input_t m_CLRQ;
		logic_input_t m_CLK;

		state_var<unsigned> m_cnt;
		state_var<unsigned> m_last_CLK;

		object_array_t<logic_output_t, 8> m_Q;
		nld_power_pins m_power_pins;
	};

	NETLIB_OBJECT(74164_dip)
	{
		NETLIB_CONSTRUCTOR(74164_dip)
		, A(*this, "A")
		{
			register_subalias("1", A.m_A);
			register_subalias("2", A.m_B);
			register_subalias("3", A.m_Q[0]);
			register_subalias("4", A.m_Q[1]);
			register_subalias("5", A.m_Q[2]);
			register_subalias("6", A.m_Q[3]);
			register_subalias("7", "A.GND");

			register_subalias("8", A.m_CLK);
			register_subalias("9", A.m_CLRQ);
			register_subalias("10", A.m_Q[4]);
			register_subalias("11", A.m_Q[5]);
			register_subalias("12", A.m_Q[6]);
			register_subalias("13", A.m_Q[7]);
			register_subalias("14", "A.VCC");
		}
	private:
		NETLIB_SUB(74164) A;
	};


	NETLIB_UPDATE(74164)
	{
		if (!m_CLRQ())
		{
			m_cnt = 0;
		}
		else if (m_CLK() && !m_last_CLK)
		{
			m_cnt = (m_cnt << 1) & 0xfe;
			if (m_A() && m_B())
			{
				m_cnt |= 0x01;
			}
			else
			{
				m_cnt &= 0xfe;
			}
		}

		m_last_CLK = m_CLK();

		for (std::size_t i=0; i<8; i++)
		{
			m_Q[i].push((m_cnt >> i) & 1, NLTIME_FROM_NS(30));
		}
	}

	NETLIB_DEVICE_IMPL(74164, "TTL_74164", "+A,+B,+CLRQ,+CLK,@VCC,@GND")
	NETLIB_DEVICE_IMPL(74164_dip, "TTL_74164_DIP", "")

	} //namespace devices
} // namespace netlist
