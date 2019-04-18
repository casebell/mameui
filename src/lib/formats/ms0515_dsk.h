// license:BSD-3-Clause
// copyright-holders:Sergey Svishchev
/*********************************************************************

    formats/ms0515_dsk.h

    ms0515 format

*********************************************************************/
#ifndef MAME_FORMATS_MS0515_DSK_H
#define MAME_FORMATS_MS0515_DSK_H

#include "wd177x_dsk.h"

class ms0515_format : public wd177x_format
{
public:
	ms0515_format();

	virtual const char *name() const override;
	virtual const char *description() const override;
	virtual const char *extensions() const override;

private:
	static const format formats[];
};

extern const floppy_format_type FLOPPY_MS0515_FORMAT;

#endif // MAME_FORMATS_MS0515_DSK_H
