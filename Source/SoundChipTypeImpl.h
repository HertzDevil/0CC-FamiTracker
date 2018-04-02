/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Library General Public License for more details.  To obtain a
** copy of the GNU Library General Public License, write to the Free
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
*/


#pragma once

#include "SoundChipType.h"

class CSoundChipType2A03 : public CSoundChipType {
	sound_chip_t GetID() const override;
	std::size_t GetSupportedChannelCount() const override;
	chan_id_t GetFirstChannelID() const override;

	std::string_view GetShortName() const override;
	std::string_view GetFullName() const override;
	std::string_view GetChannelShortName(std::size_t subindex) const override;
	std::string_view GetChannelFullName(std::size_t subindex) const override;

	std::unique_ptr<CSoundChip> MakeSoundDriver(CMixer &mixer) const override;
	std::unique_ptr<CChipHandler> MakeChipHandler() const override;
};

class CSoundChipTypeVRC6 : public CSoundChipType {
	sound_chip_t GetID() const override;
	std::size_t GetSupportedChannelCount() const override;
	chan_id_t GetFirstChannelID() const override;

	std::string_view GetShortName() const override;
	std::string_view GetFullName() const override;
	std::string_view GetChannelShortName(std::size_t subindex) const override;
	std::string_view GetChannelFullName(std::size_t subindex) const override;

	std::unique_ptr<CSoundChip> MakeSoundDriver(CMixer &mixer) const override;
	std::unique_ptr<CChipHandler> MakeChipHandler() const override;
};

class CSoundChipTypeVRC7 : public CSoundChipType {
	sound_chip_t GetID() const override;
	std::size_t GetSupportedChannelCount() const override;
	chan_id_t GetFirstChannelID() const override;

	std::string_view GetShortName() const override;
	std::string_view GetFullName() const override;
	std::string_view GetChannelShortName(std::size_t subindex) const override;
	std::string_view GetChannelFullName(std::size_t subindex) const override;

	std::unique_ptr<CSoundChip> MakeSoundDriver(CMixer &mixer) const override;
	std::unique_ptr<CChipHandler> MakeChipHandler() const override;
};

class CSoundChipTypeFDS : public CSoundChipType {
	sound_chip_t GetID() const override;
	std::size_t GetSupportedChannelCount() const override;
	chan_id_t GetFirstChannelID() const override;

	std::string_view GetShortName() const override;
	std::string_view GetFullName() const override;
	std::string_view GetChannelShortName(std::size_t subindex) const override;
	std::string_view GetChannelFullName(std::size_t subindex) const override;

	std::unique_ptr<CSoundChip> MakeSoundDriver(CMixer &mixer) const override;
	std::unique_ptr<CChipHandler> MakeChipHandler() const override;
};

class CSoundChipTypeMMC5 : public CSoundChipType {
	sound_chip_t GetID() const override;
	std::size_t GetSupportedChannelCount() const override;
	chan_id_t GetFirstChannelID() const override;

	std::string_view GetShortName() const override;
	std::string_view GetFullName() const override;
	std::string_view GetChannelShortName(std::size_t subindex) const override;
	std::string_view GetChannelFullName(std::size_t subindex) const override;

	std::unique_ptr<CSoundChip> MakeSoundDriver(CMixer &mixer) const override;
	std::unique_ptr<CChipHandler> MakeChipHandler() const override;
};

class CSoundChipTypeN163 : public CSoundChipType {
	sound_chip_t GetID() const override;
	std::size_t GetSupportedChannelCount() const override;
	chan_id_t GetFirstChannelID() const override;

	std::string_view GetShortName() const override;
	std::string_view GetFullName() const override;
	std::string_view GetChannelShortName(std::size_t subindex) const override;
	std::string_view GetChannelFullName(std::size_t subindex) const override;

	std::unique_ptr<CSoundChip> MakeSoundDriver(CMixer &mixer) const override;
	std::unique_ptr<CChipHandler> MakeChipHandler() const override;
};

class CSoundChipTypeS5B : public CSoundChipType {
	sound_chip_t GetID() const override;
	std::size_t GetSupportedChannelCount() const override;
	chan_id_t GetFirstChannelID() const override;

	std::string_view GetShortName() const override;
	std::string_view GetFullName() const override;
	std::string_view GetChannelShortName(std::size_t subindex) const override;
	std::string_view GetChannelFullName(std::size_t subindex) const override;

	std::unique_ptr<CSoundChip> MakeSoundDriver(CMixer &mixer) const override;
	std::unique_ptr<CChipHandler> MakeChipHandler() const override;
};
