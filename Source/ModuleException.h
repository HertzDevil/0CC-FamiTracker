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

#include <string>
#include <vector>
#include <exception>

enum module_error_level_t : unsigned char {		// // //
	MODULE_ERROR_NONE,		/*!< No error checking at all (warning) */
	MODULE_ERROR_DEFAULT,	/*!< Usual error checking */
	MODULE_ERROR_OFFICIAL,	/*!< Special bounds checking according to the official build */
	MODULE_ERROR_STRICT,	/*!< Extra validation for some values */
};

/*!
	\brief An exception object raised while reading and writing FTM files.
*/
class CModuleException : public std::exception
{
public:
	/*!	\brief Constructor of the exception object with an empty message. */
	CModuleException() = default;
	/*! \brief Virtual destructor. */
	virtual ~CModuleException() noexcept = default;

	/*!	\brief Obtains the error description.
		\details The description consists of zero or more lines followed by the footer specified in the
		constructor. This exception object does not use std::exception::what.
		\return The error string. */
	std::string GetErrorString() const;
	/*!	\brief Appends an error string to the exception.
		\param str The error message. */
	void AppendError(std::string str);
	/*!	\brief Sets the footer string of the error message.
		\param footer The new footer string. */
	void SetFooter(std::string footer);

public:
	static CModuleException WithMessage(std::string str) {
		CModuleException e;
		e.AppendError(std::move(str));
		return e;
	}

private:
	std::vector<std::string> m_strError;
	std::string m_strFooter;
};
