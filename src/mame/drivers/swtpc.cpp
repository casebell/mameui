// license:BSD-3-Clause
// copyright-holders:Robbbert,68bit
/***************************************************************************

        SWTPC 6800 Computer System

        http://www.swtpc.com/mholley/swtpc_6800.htm

    MIKBUG is made for a PIA (parallel) interface.
    SWTBUG is made for a ACIA (serial) interface at the same address.
    MIKBUG will actually read the bits as they arrive and assemble a byte.
    Its delay loops are based on an underclocked XTAL.

    Note: All commands must be in uppercase. See the SWTBUG manual.

    ToDo:
        - Emulate MP-A2 revision of CPU board, with four 2716 ROM sockets
          and allowance for extra RAM boards at A000-BFFF and C000-DFFF

    The MP-B motherboard decodes 0x8000 to 0x9fff as being I/O on the
    motherboard, but the MP-B2 motherboard narrowed this to 0x8000 to 0x8fff
    allowing other boards to use the address range 0x9000 to 0x9fff, and this
    narrower range is emulated here.

    The MP-B2 motherboard decoded I/O address range can be relocate to any 4K
    boundary from 0x8000 to 0xf000 with 0x8000 being the default, but
    relocation requires an alternative monitor. TODO could add config options
    for this choice if ever needed.

    Within the 4K I/O address range only A2 to A5 are decoded, and A6 to A11
    are not considered. By default A5 must be zero, but there is an option to
    alternatively select the I/O when A5 is high to allow two motherboards to
    be used together to support more cards, one for A5 low and the other for
    A5 high. TODO might consider expanding the emulated SS30 bus to support
    two banks of 8 I/O cards if anyone really need that many cards in the
    emulator.

    The address range 0x0000 to 0x7fff was used for memory expansion, off the
    motherboard.

    The MIKBUG or SWTBUG monitor ROMS required RAM in the address range 0xa000
    to 0xa07f, which could be provided by a MCM6810 installed on the
    MP-A. However FLEX 2 operating system requires RAM in the address range
    0xa000 to 0xbfff and the MCM6810 was disable and that RAM provided
    externally to the MB and this larger RAM configuration is emulated here.

    The address range 0xc000 to 0xdfff was usable for a 2K, 4K, 6K, or 8K
    PROM (the Low PROM), or for RAM (MP-8M). This emulator implements RAM in
    this region. TODO support a Low PROM.

    The address range 0xe000 to 0xffff was usable for MIKBUG, SWTBUG (1K), or
    a 2K, 4K, or 8K PROM (the High PROM). TODO support a High PROM.

    Although the maximum baud rate generated by the MP-A is 1200 baud, the
    documentation notes that the 14411 baud rate generator does generate other
    rates up to 9600 baud and documents the pins to jumper to use these other
    rates. This emulator implements such a modification supplying 9600 baud on
    the otherwise 150 baud line, which consistent with the rate that the SWTPC
    6809 supplies on this bus line.

    The FDC card, e.g. DC5, requires five addresses, four for the WD FDC and
    one for a control register, which is one more than the 4 byte I/O
    selection address range allows. On the 6800 the FDC was expected to be
    installed in slot 6 and a jumper wire installed from the slot 5 select
    line to the FDC card UD3 line input to select the control register. The
    6809 MB moved to 16 byte I/O ranges to cleanly address this
    limitation. This hardware patch does not fit nicely in the emulator bus so
    is implemented here by placing the card in slot 5 and adapting the FDC
    card emulation to decode an 8 byte address range rather than 4
    addresses. TODO reconsider if this can be handler better.

    The DC5 FDC card offers an extra control register that can be used to set
    the density and clock rate, see the DC5 source code for more
    information. This might help booting disk images that are purely double
    density with the SWTBUG disk boot support. E.g. For a 3.5" HD disk image
    enter 'M 8016' which should show '02', the DC5 version, and then enter
    '28' to select double density and a 2MHz clock. TODO include a better boot
    ROM.

Commands:
B Breakpoint
C Clear screen
D Disk boot
E End of tape
F Find a byte
G Goto
J Jump
L Ascii Load
M Memory change (enter to quit, - to display next byte)
O Optional Port
P Ascii Punch
R Register dump
Z Goto Prom (0xC000)

****************************************************************************/

#include "emu.h"
#include "cpu/m6800/m6800.h"
#include "machine/input_merger.h"
#include "machine/mc14411.h"
#include "machine/ram.h"
#include "bus/ss50/interface.h"

class swtpc_state : public driver_device
{
public:
	swtpc_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
		, m_ram(*this, "ram")
		, m_brg(*this, "brg")
		, m_maincpu_clock(*this, "MAINCPU_CLOCK")
		, m_swtbug_ready_wait(*this, "SWTBUG_READY_WAIT")
		, m_swtbug_load_at_a100(*this, "SWTBUG_LOAD_AT_A100")
	{ }

	void swtpcm(machine_config &config);
	void swtpc(machine_config &config);

	DECLARE_INPUT_CHANGED_MEMBER(maincpu_clock_change);

private:
	virtual void machine_reset() override;
	virtual void machine_start() override;

	void mem_map(address_map &map);

	required_device<cpu_device> m_maincpu;
	required_device<ram_device> m_ram;
	required_device<mc14411_device> m_brg;
	required_ioport m_maincpu_clock;
	required_ioport m_swtbug_ready_wait;
	required_ioport m_swtbug_load_at_a100;
};

void swtpc_state::mem_map(address_map &map)
{
	map.unmap_value_high();
	map(0x8000, 0x8003).mirror(0x0fc0).rw("io0", FUNC(ss50_interface_port_device::read), FUNC(ss50_interface_port_device::write));
	map(0x8004, 0x8007).mirror(0x0fc0).rw("io1", FUNC(ss50_interface_port_device::read), FUNC(ss50_interface_port_device::write));
	map(0x8008, 0x800b).mirror(0x0fc0).rw("io2", FUNC(ss50_interface_port_device::read), FUNC(ss50_interface_port_device::write));
	map(0x800c, 0x800f).mirror(0x0fc0).rw("io3", FUNC(ss50_interface_port_device::read), FUNC(ss50_interface_port_device::write));
	map(0x8010, 0x8013).mirror(0x0fc0).rw("io4", FUNC(ss50_interface_port_device::read), FUNC(ss50_interface_port_device::write));
	// For the FDC a hardware patch was necesseary routing IO5 select to
	// the FDC installed in slot 6. For the Emulator, give IO5 an 8 byte
	// range for the FDC.
	map(0x8014, 0x801b).mirror(0x0fc0).rw("io5", FUNC(ss50_interface_port_device::read), FUNC(ss50_interface_port_device::write));
	//map(0x8018, 0x801b).mirror(0x0fc0).rw("io6", FUNC(ss50_interface_port_device::read), FUNC(ss50_interface_port_device::write));
	map(0x801c, 0x801f).mirror(0x0fc0).rw("io7", FUNC(ss50_interface_port_device::read), FUNC(ss50_interface_port_device::write));
	map(0xa000, 0xbfff).ram();
	// TODO low prom, 2K, 4K, 6K, or 8K.
	map(0xc000, 0xdfff).ram();
	// TODO high prom, 1K, 2K, 4K, 8K.
	map(0xe000, 0xe3ff).mirror(0x1c00).rom().region("mcm6830", 0);
}

/* Input ports */
static INPUT_PORTS_START( swtpc )
	// Support some clock variations. The MP-A did not use a crystal for
	// the CPU clock and the frequency was variable. The 6800 was
	// available at speeds up to 2MHz so that might not have been
	// impossible. An overclock option of 4MHz is also implemented.
	PORT_START("MAINCPU_CLOCK")
	PORT_CONFNAME(0xffffff, 1000000, "CPU clock") PORT_CHANGED_MEMBER(DEVICE_SELF, swtpc_state, maincpu_clock_change, 0)
	PORT_CONFSETTING( 898550, "0.89855 MHz")  // MIKBUG
	PORT_CONFSETTING( 921600, "0.92160 MHz")  // SWTPC
	PORT_CONFSETTING(1000000, "1.0 MHz")
	PORT_CONFSETTING(2000000, "2.0 MHz")
	PORT_CONFSETTING(4000000, "4.0 MHz")

	// Patch the SWTBUG to wait for the motor to start. The SWTBUG
	// accesses the FDC control register and then waits a period for the
	// motor to start. Unfortunately the DC series of FDCs do not trigger
	// the motor when accessing the control register, the drive does not
	// have time to become ready before commands are issued and the boot
	// fails. This workaround is necessary in practice.
	PORT_START("SWTBUG_READY_WAIT")
	PORT_CONFNAME(0x1, 1, "SWTBUG ready wait patch")
	PORT_CONFSETTING(0, "No")
	PORT_CONFSETTING(1, "Yes - apply patch")

	// Patch SWTBUG to load the disk boot code at 0xa100 rather than
	// 0x2400. The disk boot code is typically position dependent and many
	// disk images expect to have their boot code loaded at 0xa100. TODO
	// consider adding a separate machine using NEWBUG etc that loads at
	// 0xa100 or perhaps better implement support for the high PROM to
	// allow custom code to be used which is needed anyway as even NEWBUG
	// appears to have issues and is not optimized for the DC5 FDC.
	PORT_START("SWTBUG_LOAD_AT_A100")
	PORT_CONFNAME(0x1, 1, "SWTBUG disk boot patch, to load at 0xa100")
	PORT_CONFSETTING(0, "No")
	PORT_CONFSETTING(1, "Yes - apply patch")

INPUT_PORTS_END

static DEVICE_INPUT_DEFAULTS_START( dc5 )
	DEVICE_INPUT_DEFAULTS("address_mode", 0xf, 0)
DEVICE_INPUT_DEFAULTS_END

INPUT_CHANGED_MEMBER(swtpc_state::maincpu_clock_change)
{
	m_maincpu->set_clock(newval);
}

void swtpc_state::machine_reset()
{
	uint32_t maincpu_clock = m_maincpu_clock->read();
	if (maincpu_clock)
		m_maincpu->set_clock(maincpu_clock);

	// TODO make these SWTBUG patches conditional on using SWTBUG!

	if (m_swtbug_ready_wait->read())
	{
		// Patch SWTBUG to also wait until the drive is ready.
		uint8_t* swtbug = memregion("mcm6830")->base();
		swtbug[0x029b] = 0x81;
	}

	if (m_swtbug_load_at_a100->read())
	{
		// Patch SWTBUG to load the disk boot sector at 0xa100.
		uint8_t* swtbug = memregion("mcm6830")->base();
		swtbug[0x02a7] = 0xa1; // Load address
		swtbug[0x02a8] = 0x00;
		swtbug[0x02bb] = 0xa1; // Entry address
		swtbug[0x02bc] = 0x00;
	}
}

void swtpc_state::machine_start()
{
	m_brg->rsa_w(0);
	m_brg->rsb_w(1);

	m_maincpu->space(AS_PROGRAM).install_ram(0, m_ram->size() - 1, m_ram->pointer());
}

void swtpc_state::swtpc(machine_config &config)
{
	/* basic machine hardware */
	M6800(config, m_maincpu, XTAL(1'843'200) / 2);
	m_maincpu->set_addrmap(AS_PROGRAM, &swtpc_state::mem_map);

	MC14411(config, m_brg, XTAL(1'843'200));
	// 1200b
	m_brg->out_f<7>().set("io0", FUNC(ss50_interface_port_device::f600_1200_w));
	m_brg->out_f<7>().append("io1", FUNC(ss50_interface_port_device::f600_1200_w));
	m_brg->out_f<7>().append("io2", FUNC(ss50_interface_port_device::f600_1200_w));
	m_brg->out_f<7>().append("io3", FUNC(ss50_interface_port_device::f600_1200_w));
	m_brg->out_f<7>().append("io4", FUNC(ss50_interface_port_device::f600_1200_w));
	m_brg->out_f<7>().append("io5", FUNC(ss50_interface_port_device::f600_1200_w));
	m_brg->out_f<7>().append("io6", FUNC(ss50_interface_port_device::f600_1200_w));
	m_brg->out_f<7>().append("io7", FUNC(ss50_interface_port_device::f600_1200_w));
	// 600b
	m_brg->out_f<8>().set("io0", FUNC(ss50_interface_port_device::f600_4800_w));
	m_brg->out_f<8>().append("io1", FUNC(ss50_interface_port_device::f600_4800_w));
	m_brg->out_f<8>().append("io2", FUNC(ss50_interface_port_device::f600_4800_w));
	m_brg->out_f<8>().append("io3", FUNC(ss50_interface_port_device::f600_4800_w));
	m_brg->out_f<8>().append("io4", FUNC(ss50_interface_port_device::f600_4800_w));
	m_brg->out_f<8>().append("io5", FUNC(ss50_interface_port_device::f600_4800_w));
	m_brg->out_f<8>().append("io6", FUNC(ss50_interface_port_device::f600_4800_w));
	m_brg->out_f<8>().append("io7", FUNC(ss50_interface_port_device::f600_4800_w));
	// 300b
	m_brg->out_f<9>().set("io0", FUNC(ss50_interface_port_device::f300_w));
	m_brg->out_f<9>().append("io1", FUNC(ss50_interface_port_device::f300_w));
	m_brg->out_f<9>().append("io2", FUNC(ss50_interface_port_device::f300_w));
	m_brg->out_f<9>().append("io3", FUNC(ss50_interface_port_device::f300_w));
	m_brg->out_f<9>().append("io4", FUNC(ss50_interface_port_device::f300_w));
	m_brg->out_f<9>().append("io5", FUNC(ss50_interface_port_device::f300_w));
	m_brg->out_f<9>().append("io6", FUNC(ss50_interface_port_device::f300_w));
	m_brg->out_f<9>().append("io7", FUNC(ss50_interface_port_device::f300_w));
	// 150b pin 11, modified to wire the 14411 pin 1 to the f150 line for 9600b
	m_brg->out_f<1>().set("io0", FUNC(ss50_interface_port_device::f150_9600_w));
	m_brg->out_f<1>().append("io1", FUNC(ss50_interface_port_device::f150_9600_w));
	m_brg->out_f<1>().append("io2", FUNC(ss50_interface_port_device::f150_9600_w));
	m_brg->out_f<1>().append("io3", FUNC(ss50_interface_port_device::f150_9600_w));
	m_brg->out_f<1>().append("io4", FUNC(ss50_interface_port_device::f150_9600_w));
	m_brg->out_f<1>().append("io5", FUNC(ss50_interface_port_device::f150_9600_w));
	m_brg->out_f<1>().append("io6", FUNC(ss50_interface_port_device::f150_9600_w));
	m_brg->out_f<1>().append("io7", FUNC(ss50_interface_port_device::f150_9600_w));
	// 110b
	m_brg->out_f<13>().set("io0", FUNC(ss50_interface_port_device::f110_w));
	m_brg->out_f<13>().append("io1", FUNC(ss50_interface_port_device::f110_w));
	m_brg->out_f<13>().append("io2", FUNC(ss50_interface_port_device::f110_w));
	m_brg->out_f<13>().append("io3", FUNC(ss50_interface_port_device::f110_w));
	m_brg->out_f<13>().append("io4", FUNC(ss50_interface_port_device::f110_w));
	m_brg->out_f<13>().append("io5", FUNC(ss50_interface_port_device::f110_w));
	m_brg->out_f<13>().append("io6", FUNC(ss50_interface_port_device::f110_w));
	m_brg->out_f<13>().append("io7", FUNC(ss50_interface_port_device::f110_w));

	ss50_interface_port_device &io0(SS50_INTERFACE(config, "io0", ss50_default_2rs_devices, nullptr));
	io0.irq_cb().set("mainirq", FUNC(input_merger_device::in_w<0>));
	io0.firq_cb().set("mainnmi", FUNC(input_merger_device::in_w<0>));
	ss50_interface_port_device &io1(SS50_INTERFACE(config, "io1", ss50_default_2rs_devices, "mps"));
	io1.irq_cb().set("mainirq", FUNC(input_merger_device::in_w<1>));
	io1.firq_cb().set("mainnmi", FUNC(input_merger_device::in_w<1>));
	ss50_interface_port_device &io2(SS50_INTERFACE(config, "io2", ss50_default_2rs_devices, nullptr));
	io2.irq_cb().set("mainirq", FUNC(input_merger_device::in_w<2>));
	io2.firq_cb().set("mainnmi", FUNC(input_merger_device::in_w<2>));
	ss50_interface_port_device &io3(SS50_INTERFACE(config, "io3", ss50_default_2rs_devices, nullptr));
	io3.irq_cb().set("mainirq", FUNC(input_merger_device::in_w<3>));
	io3.firq_cb().set("mainnmi", FUNC(input_merger_device::in_w<3>));
	ss50_interface_port_device &io4(SS50_INTERFACE(config, "io4", ss50_default_2rs_devices, "mpt"));
	io4.irq_cb().set("mainirq", FUNC(input_merger_device::in_w<4>));
	io4.firq_cb().set("mainnmi", FUNC(input_merger_device::in_w<4>));
	ss50_interface_port_device &io5(SS50_INTERFACE(config, "io5", ss50_default_2rs_devices, "dc5"));
	io5.irq_cb().set("mainirq", FUNC(input_merger_device::in_w<5>));
	io5.firq_cb().set("mainnmi", FUNC(input_merger_device::in_w<5>));
	ss50_interface_port_device &io6(SS50_INTERFACE(config, "io6", ss50_default_2rs_devices, nullptr));
	io6.irq_cb().set("mainirq", FUNC(input_merger_device::in_w<6>));
	io6.firq_cb().set("mainnmi", FUNC(input_merger_device::in_w<6>));
	ss50_interface_port_device &io7(SS50_INTERFACE(config, "io7", ss50_default_2rs_devices, nullptr));
	io7.irq_cb().set("mainirq", FUNC(input_merger_device::in_w<7>));
	io7.firq_cb().set("mainnmi", FUNC(input_merger_device::in_w<7>));

	INPUT_MERGER_ANY_HIGH(config, "mainirq").output_handler().set_inputline(m_maincpu, M6800_IRQ_LINE);
	INPUT_MERGER_ANY_HIGH(config, "mainnmi").output_handler().set_inputline(m_maincpu, INPUT_LINE_NMI);

	RAM(config, RAM_TAG).set_default_size("32K").set_extra_options("4K,8K,12K,16K,20K,24K,28K,32K");

	io5.set_option_device_input_defaults("dc5", DEVICE_INPUT_DEFAULTS_NAME(dc5));
}

void swtpc_state::swtpcm(machine_config &config)
{
	swtpc(config);
	m_maincpu->set_clock(XTAL(1'797'100) / 2);

	m_brg->set_clock(XTAL(1'797'100));

	subdevice<ss50_interface_port_device>("io1")->set_default_option("mpc");
}

/* ROM definition */
ROM_START( swtpc )
	ROM_REGION( 0x0400, "mcm6830", 0 )
	ROM_LOAD("swtbug.bin", 0x0000, 0x0400, CRC(f9130ef4) SHA1(089b2d2a56ce9526c3e78ce5d49ce368b9eabc0c))
ROM_END

ROM_START( swtpcm )
	ROM_REGION( 0x0400, "mcm6830", 0 )
	ROM_LOAD("mikbug.bin", 0x0000, 0x0400, CRC(e7f4d9d0) SHA1(5ad585218f9c9c70f38b3c74e3ed5dfe0357621c))
ROM_END

/* Driver */

//    YEAR  NAME    PARENT  COMPAT  MACHINE  INPUT  CLASS        INIT        COMPANY                                     FULLNAME      FLAGS
COMP( 1977, swtpc,  0,      0,      swtpc,   swtpc, swtpc_state, empty_init, "Southwest Technical Products Corporation", "SWTPC 6800 Computer System (with SWTBUG)", MACHINE_NO_SOUND_HW )
COMP( 1975, swtpcm, swtpc,  0,      swtpcm,  swtpc, swtpc_state, empty_init, "Southwest Technical Products Corporation", "SWTPC 6800 Computer System (with MIKBUG)", MACHINE_NO_SOUND_HW )
