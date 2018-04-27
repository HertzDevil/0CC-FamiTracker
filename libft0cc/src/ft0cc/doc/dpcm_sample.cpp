/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * 0CC-FamiTracker is (C) 2014-2018 HertzDevil
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the GNU General Public License Version 2, as described below:
 *
 * This file is free software: you may copy, redistribute and/or modify
 * it under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 2 of the License, or (at your
 * option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/. */

#include "ft0cc/doc/dpcm_sample.hpp"
#include <algorithm>
#include <random>

using namespace ft0cc::doc;

dpcm_sample::dpcm_sample(std::size_t sz) : data_(sz, pad_value) {
}

dpcm_sample::dpcm_sample(std::vector<sample_t> samples, std::string_view name) :
	data_(std::move(samples)), name_(name)
{
}

bool dpcm_sample::operator==(const dpcm_sample &other) const {
	return data_ == other.data_ && name_ == other.name_;
}

std::string_view dpcm_sample::name() const {
	return name_;
}

void dpcm_sample::rename(std::string_view sv) {
	name_ = sv.substr(0, max_name_length);
}

dpcm_sample::sample_t dpcm_sample::sample_at(std::size_t pos) const {
	return pos < size() ? data_[pos] : pad_value;
}

void dpcm_sample::set_sample_at(std::size_t pos, sample_t sample) {
	if (pos < size())
		data_[pos] = sample;
}

const dpcm_sample::sample_t *dpcm_sample::data() const {
	return data_.data();
}

std::size_t dpcm_sample::size() const {
	return data_.size();
}

void dpcm_sample::resize(std::size_t sz) {
	std::size_t old = size();
	data_.resize(sz);
	if (sz > old)
		std::fill(data_.begin() + old, data_.end(), pad_value);
}

void dpcm_sample::cut_samples(std::size_t b, std::size_t e) {
	data_.erase(data_.begin() + b, data_.begin() + e);
}

void dpcm_sample::tilt(std::size_t b, std::size_t e) {
	std::size_t Diff = e - b;

	int Nr = 10;
	unsigned Step = (Diff * 8) / Nr;
	std::random_device rd;
	std::mt19937 gen {rd()};
	unsigned Cntr = std::uniform_int_distribution<unsigned>(0, Step - 1)(gen);

	for (std::size_t i = b; i < e; ++i) {
		for (int j = 0; j < 8; ++j) {
			if (++Cntr == Step) {
				data_[i] &= (0xFF ^ (1 << j));
				Cntr = 0;
			}
		}
	}
}
