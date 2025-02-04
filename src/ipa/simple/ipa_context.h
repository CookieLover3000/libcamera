/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2024 Red Hat, Inc.
 *
 * Simple pipeline IPA Context
 */

#pragma once

#include <array>
#include <optional>
#include <stdint.h>

#include <libipa/fc_queue.h>

namespace libcamera {

namespace ipa::soft {

struct IPASessionConfiguration {
	float gamma;
	struct {
		int32_t exposureMin, exposureMax;
		double againMin, againMax, againMinStep;
	} agc;
	struct {
		std::optional<uint8_t> level;
	} black;
	struct {
		int32_t afocusMax;
		int32_t stepValue;
	} af;
};

struct IPAActiveState {
	struct {
		uint8_t level;
	} blc;

	struct {
		double red;
		double green;
		double blue;
	} gains;

	struct {
		int32_t exposure;
		double again;
	} agc;

	static constexpr unsigned int kGammaLookupSize = 1024;
	struct {
		std::array<double, kGammaLookupSize> gammaTable;
		uint8_t blackLevel;
	} gamma;

	struct {
		int32_t focus;
		uint64_t sharpnessLock;
	} af;
};

struct IPAFrameContext : public FrameContext {
	struct {
		uint32_t exposure;
		double gain;
	} sensor;
};

struct IPAContext {
	IPASessionConfiguration configuration;
	IPAActiveState activeState;
	FCQueue<IPAFrameContext> frameContexts;
};

} /* namespace ipa::soft */

} /* namespace libcamera */
