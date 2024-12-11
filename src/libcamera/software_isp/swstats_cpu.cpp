/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2023, Linaro Ltd
 * Copyright (C) 2023, Red Hat Inc.
 *
 * Authors:
 * Hans de Goede <hdegoede@redhat.com>
 *
 * CPU based software statistics implementation
 */

#include "libcamera/internal/software_isp/swstats_cpu.h"

#include <libcamera/base/log.h>

#include <libcamera/formats.h>
#include <libcamera/stream.h>

#include "libcamera/internal/bayer_format.h"
#include "libcamera/internal/mapped_framebuffer.h"


namespace libcamera {

/**
 * \class SwStatsCpu
 * \brief Class for gathering statistics on the CPU
 *
 * CPU based software ISP statistics implementation.
 *
 * This class offers a configure function + functions to gather statistics on a
 * line by line basis. This allows CPU based software debayering to interleave
 * debayering and statistics gathering on a line by line basis while the input
 * data is still hot in the cache.
 *
 * It is also possible to specify a window over which to gather statistics
 * instead of processing the whole frame.
 */

/**
 * \fn bool SwStatsCpu::isValid() const
 * \brief Gets whether the statistics object is valid
 *
 * \return True if it's valid, false otherwise
 */

/**
 * \fn const SharedFD &SwStatsCpu::getStatsFD()
 * \brief Get the file descriptor for the statistics
 *
 * \return The file descriptor
 */

/**
 * \fn const Size &SwStatsCpu::patternSize()
 * \brief Get the pattern size
 *
 * For some input-formats, e.g. Bayer data, processing is done multiple lines
 * and/or columns at a time. Get width and height at which the (bayer) pattern
 * repeats. Window values are rounded down to a multiple of this and the height
 * also indicates if processLine2() should be called or not.
 * This may only be called after a successful configure() call.
 *
 * Valid sizes are: 1x1, 2x2, 4x2 or 4x4.
 *
 * \return The pattern size
 */

/**
 * \fn void SwStatsCpu::processLine0(unsigned int y, const uint8_t *src[])
 * \brief Process line 0
 * \param[in] y The y coordinate.
 * \param[in] src The input data.
 *
 * This function processes line 0 for input formats with
 * patternSize height == 1.
 * It'll process line 0 and 1 for input formats with patternSize height >= 2.
 * This function may only be called after a successful setWindow() call.
 *
 * This function takes an array of src pointers each pointing to a line in
 * the source image.
 *
 * Bayer input data requires (patternSize_.height + 1) src pointers, with
 * the middle element of the array pointing to the actual line being processed.
 * Earlier element(s) will point to the previous line(s) and later element(s)
 * to the next line(s). See the DebayerCpu::debayerFn documentation for details.
 *
 * Planar input data requires a src pointer for each plane, with src[0] pointing
 * to the line in plane 0, etc.
 *
 * For non Bayer single plane input data only a single src pointer is required.
 */

/**
 * \fn void SwStatsCpu::processLine2(unsigned int y, const uint8_t *src[])
 * \brief Process line 2 and 3
 * \param[in] y The y coordinate.
 * \param[in] src The input data.
 *
 * This function processes line 2 and 3 for input formats with
 * patternSize height == 4.
 * This function may only be called after a successful setWindow() call.
 */

/**
 * \var Signal<> SwStatsCpu::statsReady
 * \brief Signals that the statistics are ready
 */

/**
 * \var unsigned int SwStatsCpu::ySkipMask_
 * \brief Skip lines where this bitmask is set in y
 */

/**
 * \var Rectangle SwStatsCpu::window_
 * \brief Statistics window, set by setWindow(), used every line
 */

/**
 * \var unsigned int SwStatsCpu::xShift_
 * \brief The offset of x, applied to window_.x for bayer variants
 *
 * This can either be 0 or 1.
 */

LOG_DEFINE_CATEGORY(SwStatsCpu)

SwStatsCpu::SwStatsCpu()
	: sharedStats_("softIsp_stats")
{
	if (!sharedStats_)
		LOG(SwStatsCpu, Error)
			<< "Failed to create shared memory for statistics";
}

static constexpr unsigned int kRedYMul = 77; /* 0.299 * 256 */
static constexpr unsigned int kGreenYMul = 150; /* 0.587 * 256 */
static constexpr unsigned int kBlueYMul = 29; /* 0.114 * 256 */

#define SWSTATS_START_LINE_STATS(pixel_t) \
	pixel_t r, g, g2, b;              \
	uint64_t yVal;                    \
                                          \
	uint64_t sumR = 0;                \
	uint64_t sumG = 0;                \
	uint64_t sumB = 0;

#define SWSTATS_ACCUMULATE_LINE_STATS(div) \
	sumR += r;                         \
	sumG += g;                         \
	sumB += b;                         \
                                           \
	yVal = r * kRedYMul;               \
	yVal += g * kGreenYMul;            \
	yVal += b * kBlueYMul;             \
	stats_.yHistogram[yVal * SwIspStats::kYHistogramSize / (256 * 256 * (div))]++;

#define SWSTATS_FINISH_LINE_STATS() \
	stats_.sumR_ += sumR;       \
	stats_.sumG_ += sumG;       \
	stats_.sumB_ += sumB;

void SwStatsCpu::statsBGGR8Line0(const uint8_t *src[])
{
	const uint8_t *src0 = src[1] + window_.x;
	const uint8_t *src1 = src[2] + window_.x;

	SWSTATS_START_LINE_STATS(uint8_t)

	if (swapLines_)
		std::swap(src0, src1);

	/* x += 4 sample every other 2x2 block */
	for (int x = 0; x < (int)window_.width; x += 4) {
		b = src0[x];
		g = src0[x + 1];
		g2 = src1[x];
		r = src1[x + 1];

		g = (g + g2) / 2;

		SWSTATS_ACCUMULATE_LINE_STATS(1)
	}

	SWSTATS_FINISH_LINE_STATS()
}

void SwStatsCpu::statsBGGR10Line0(const uint8_t *src[])
{
	const uint16_t *src0 = (const uint16_t *)src[1] + window_.x;
	const uint16_t *src1 = (const uint16_t *)src[2] + window_.x;

	SWSTATS_START_LINE_STATS(uint16_t)

	if (swapLines_)
		std::swap(src0, src1);

	/* x += 4 sample every other 2x2 block */
	for (int x = 0; x < (int)window_.width; x += 4) {
		b = src0[x];
		g = src0[x + 1];
		g2 = src1[x];
		r = src1[x + 1];

		g = (g + g2) / 2;

		/* divide Y by 4 for 10 -> 8 bpp value */
		SWSTATS_ACCUMULATE_LINE_STATS(4)
	}

	SWSTATS_FINISH_LINE_STATS()
}

void SwStatsCpu::statsBGGR12Line0(const uint8_t *src[])
{
	const uint16_t *src0 = (const uint16_t *)src[1] + window_.x;
	const uint16_t *src1 = (const uint16_t *)src[2] + window_.x;

	SWSTATS_START_LINE_STATS(uint16_t)

	if (swapLines_)
		std::swap(src0, src1);

	/* x += 4 sample every other 2x2 block */
	for (int x = 0; x < (int)window_.width; x += 4) {
		b = src0[x];
		g = src0[x + 1];
		g2 = src1[x];
		r = src1[x + 1];

		g = (g + g2) / 2;

		/* divide Y by 16 for 12 -> 8 bpp value */
		SWSTATS_ACCUMULATE_LINE_STATS(16)
	}

	SWSTATS_FINISH_LINE_STATS()
}

void SwStatsCpu::statsBGGR10PLine0(const uint8_t *src[])
{
	const uint8_t *src0 = src[1] + window_.x * 5 / 4;
	const uint8_t *src1 = src[2] + window_.x * 5 / 4;
	const int widthInBytes = window_.width * 5 / 4;

	if (swapLines_)
		std::swap(src0, src1);

	SWSTATS_START_LINE_STATS(uint8_t)

	/* x += 5 sample every other 2x2 block */
	for (int x = 0; x < widthInBytes; x += 5) {
		/* BGGR */
		b = src0[x];
		g = src0[x + 1];
		g2 = src1[x];
		r = src1[x + 1];
		g = (g + g2) / 2;
		/* Data is already 8 bits, divide by 1 */
		SWSTATS_ACCUMULATE_LINE_STATS(1)
	}

	SWSTATS_FINISH_LINE_STATS()
}

void SwStatsCpu::statsGBRG10PLine0(const uint8_t *src[])
{
	const uint8_t *src0 = src[1] + window_.x * 5 / 4;
	const uint8_t *src1 = src[2] + window_.x * 5 / 4;
	const int widthInBytes = window_.width * 5 / 4;

	if (swapLines_)
		std::swap(src0, src1);

	SWSTATS_START_LINE_STATS(uint8_t)

	/* x += 5 sample every other 2x2 block */
	for (int x = 0; x < widthInBytes; x += 5) {
		/* GBRG */
		g = src0[x];
		b = src0[x + 1];
		r = src1[x];
		g2 = src1[x + 1];
		g = (g + g2) / 2;
		/* Data is already 8 bits, divide by 1 */
		SWSTATS_ACCUMULATE_LINE_STATS(1)
	}

	SWSTATS_FINISH_LINE_STATS()
}

void SwStatsCpu::statsYUV420Line0(const uint8_t *src[])
{
	uint64_t sumY = 0;
	uint64_t sumU = 0;
	uint64_t sumV = 0;
	uint8_t y, u, v;

	/* Adjust src[] for starting at window_.x */
	src[0] += window_.x;
	src[1] += window_.x / 2;
	src[2] += window_.x / 2;

	/* x += 4 sample every other 2x2 block */
	for (int x = 0; x < (int)window_.width; x += 4) {
		/*
		 * Take y from the top left corner of the 2x2 block instead
		 * of averaging 4 y-s.
		 */
		y = src[0][x];
		u = src[1][x];
		v = src[2][x];

		sumY += y;
		sumU += u;
		sumV += v;

		stats_.yHistogram[y * SwIspStats::kYHistogramSize / 256]++;
	}

	stats_.sumR_ += sumY;
	stats_.sumG_ += sumU;
	stats_.sumB_ += sumV;
}

/**
 * \brief Reset state to start statistics gathering for a new frame
 *
 * This may only be called after a successful setWindow() call.
 */
void SwStatsCpu::startFrame(void)
{
	if (window_.width == 0)
		LOG(SwStatsCpu, Error) << "Calling startFrame() without setWindow()";

	stats_.sumR_ = 0;
	stats_.sumB_ = 0;
	stats_.sumG_ = 0;
	stats_.yHistogram.fill(0);
}

/**
 * \brief Finish statistics calculation for the current frame
 * \param[in] frame The frame number
 * \param[in] bufferId ID of the statistics buffer
 *
 * This may only be called after a successful setWindow() call.
 */
void SwStatsCpu::finishFrame(uint32_t frame, uint32_t bufferId)
{
	if (finishFrame_)
		(this->*finishFrame_)();

	*sharedStats_ = stats_;
	statsReady.emit(frame, bufferId);
}

/**
 * \brief Setup SwStatsCpu object for standard Bayer orders
 * \param[in] order The Bayer order
 *
 * Check if order is a standard Bayer order and setup xShift_ and swapLines_
 * so that a single BGGR stats function can be used for all 4 standard orders.
 */
int SwStatsCpu::setupStandardBayerOrder(BayerFormat::Order order)
{
	switch (order) {
	case BayerFormat::BGGR:
		xShift_ = 0;
		swapLines_ = false;
		break;
	case BayerFormat::GBRG:
		xShift_ = 1; /* BGGR -> GBRG */
		swapLines_ = false;
		break;
	case BayerFormat::GRBG:
		xShift_ = 0;
		swapLines_ = true; /* BGGR -> GRBG */
		break;
	case BayerFormat::RGGB:
		xShift_ = 1; /* BGGR -> GBRG */
		swapLines_ = true; /* GBRG -> RGGB */
		break;
	default:
		return -EINVAL;
	}

	patternSize_.height = 2;
	patternSize_.width = 2;
	ySkipMask_ = 0x02; /* Skip every 3th and 4th line */
	return 0;
}

/**
 * \brief Configure the statistics object for the passed in input format
 * \param[in] inputCfg The input format
 *
 * \return 0 on success, a negative errno value on failure
 */
int SwStatsCpu::configure(const StreamConfiguration &inputCfg)
{
	stride_ = inputCfg.stride;
	frameSize_ = inputCfg.size;
	finishFrame_ = NULL;

	if (inputCfg.pixelFormat == formats::YUV420) {
		patternSize_.height = 2;
		patternSize_.width = 2;
		/* Skip every 3th and 4th line, sample every other 2x2 block */
		ySkipMask_ = 0x02;
		xShift_ = 0;
		swapLines_ = false;
		stats0_ = &SwStatsCpu::statsYUV420Line0;
		processFrame_ = &SwStatsCpu::processYUV420Frame;
		finishFrame_ = &SwStatsCpu::finishYUV420Frame;
		return 0;
	}

	BayerFormat bayerFormat =
		BayerFormat::fromPixelFormat(inputCfg.pixelFormat);

	if (bayerFormat.packing == BayerFormat::Packing::None &&
	    setupStandardBayerOrder(bayerFormat.order) == 0) {
		processFrame_ = &SwStatsCpu::processBayerFrame2;
		switch (bayerFormat.bitDepth) {
		case 8:
			stats0_ = &SwStatsCpu::statsBGGR8Line0;
			return 0;
		case 10:
			stats0_ = &SwStatsCpu::statsBGGR10Line0;
			return 0;
		case 12:
			stats0_ = &SwStatsCpu::statsBGGR12Line0;
			return 0;
		}
	}

	if (bayerFormat.bitDepth == 10 &&
	    bayerFormat.packing == BayerFormat::Packing::CSI2) {
		patternSize_.height = 2;
		patternSize_.width = 4; /* 5 bytes per *4* pixels */
		/* Skip every 3th and 4th line, sample every other 2x2 block */
		ySkipMask_ = 0x02;
		xShift_ = 0;
		processFrame_ = &SwStatsCpu::processBayerFrame2;

		switch (bayerFormat.order) {
		case BayerFormat::BGGR:
		case BayerFormat::GRBG:
			stats0_ = &SwStatsCpu::statsBGGR10PLine0;
			swapLines_ = bayerFormat.order == BayerFormat::GRBG;
			return 0;
		case BayerFormat::GBRG:
		case BayerFormat::RGGB:
			stats0_ = &SwStatsCpu::statsGBRG10PLine0;
			swapLines_ = bayerFormat.order == BayerFormat::RGGB;
			return 0;
		default:
			break;
		}
	}

	LOG(SwStatsCpu, Info)
		<< "Unsupported input format " << inputCfg.pixelFormat.toString();
	return -EINVAL;
}

/**
 * \brief Specify window coordinates over which to gather statistics
 * \param[in] window The window object.
 */
void SwStatsCpu::setWindow(const Rectangle &window)
{
	window_ = window;

	window_.x &= ~(patternSize_.width - 1);
	window_.x += xShift_;
	window_.y &= ~(patternSize_.height - 1);

	/* width_ - xShift_ to make sure the window fits */
	window_.width -= xShift_;
	window_.width &= ~(patternSize_.width - 1);
	window_.height &= ~(patternSize_.height - 1);
}

void SwStatsCpu::processYUV420Frame(MappedFrameBuffer &in)
{
	const uint8_t *linePointers[3];

	linePointers[0] = in.planes()[0].data();
	linePointers[1] = in.planes()[1].data();
	linePointers[2] = in.planes()[2].data();

	/* Adjust linePointers for starting at window_.y */
	linePointers[0] += window_.y * stride_;
	linePointers[1] += window_.y * stride_ / 4;
	linePointers[2] += window_.y * stride_ / 4;


	if(true) /* TODO: add boolean for when you want to calculate sharpness */
		calculateSharpness(in.planes()[0].data());

	for (unsigned int y = 0; y < window_.height; y += 2) {
		if (!(y & ySkipMask_))
			(this->*stats0_)(linePointers);

		linePointers[0] += stride_ * 2;
		linePointers[1] += stride_ / 2;
		linePointers[2] += stride_ / 2;
	}
}

void SwStatsCpu::calculateSharpness(uint8_t *frameY)
{
    unsigned int width = frameSize_.width * 0.3;
    unsigned int height = frameSize_.height * 0.3;

    unsigned int offsetX = (frameSize_.width - width) / 2;
    unsigned int offsetY = (frameSize_.height - height) / 2;

    /* Transform the cropped window of the 1D array to a 2D one */
    uint8_t src[width][height];

    for (unsigned int i = 0; i < width; ++i) {
        for (unsigned int j = 0; j < height; ++j) {
            unsigned int srcX = i + offsetX;
            unsigned int srcY = j + offsetY;
            src[i][j] = *(frameY + (srcX * stride_ + srcY));
        }
    }

    /* Apply kernel and calculate sharpness */
    int8_t kernel[3][3] = { {0, 1, 0},
                            {1, -4, 1},
                            {0, 1, 0} };

    double sumArray[width][height];
	double mean = 0.0;
	int count = 0;

    for (unsigned int w = 1; w < width - 1; ++w) {
        for (unsigned int h = 1; h < height - 1; ++h) {
            double sum = 0.0;
            for (int i = -1; i <= 1; ++i) {
                for (int j = -1; j <= 1; ++j) {
                    unsigned int srcW = w + i;
                    unsigned int srcH = h + j;
                    sum += kernel[i + 1][j + 1] * src[srcW][srcH];
                }
            }
            sumArray[w][h] = std::abs(sum);
			mean += sumArray[w][h];
			++count;
        }
    }

    /* Calculate standard deviation */
    double stddev = 0.0, variance = 0.0;
    mean /= count;

    for (unsigned int w = 0; w < width; ++w) {
        for (unsigned int h = 0; h < height; ++h) {
            double difference = sumArray[w][h] - mean;
            variance += difference * difference;
        }
    }
    stddev = variance / (count - 1);

    stats_.sharpnessValue_ = (int)(stddev * stddev);
    LOG(SwStatsCpu, Info) << stats_.sharpnessValue_;
}



void SwStatsCpu::finishYUV420Frame()
{
	/* sumR_ / G_ / B_ contain Y / U / V sums convert this */
	double divider = (uint64_t)window_.width * window_.height * 256 / 16;
	double Y = (double)stats_.sumR_ / divider;
	/* U and V 0 - 255 values represent -128 - 127 range */
	double U = (double)stats_.sumG_ / divider - 0.5;
	double V = (double)stats_.sumB_ / divider - 0.5;

	stats_.sumR_ = (Y + 1.140 * V) * divider;
	stats_.sumG_ = (Y - 0.395 * U - 0.581 * V) * divider;
	stats_.sumB_ = (Y + 2.032 * U) * divider;
}

void SwStatsCpu::processBayerFrame2(MappedFrameBuffer &in)
{
	
	const uint8_t *src = in.planes()[0].data();
	const uint8_t *linePointers[3];

	/* Adjust src for starting at window_.y */
	src += window_.y * stride_;

	for (unsigned int y = 0; y < window_.height; y += 2) {
		if (y & ySkipMask_) {
			src += stride_ * 2;
			continue;
		}

		/* linePointers[0] is not used by any stats0_ functions */
		linePointers[1] = src;
		linePointers[2] = src + stride_;
		(this->*stats0_)(linePointers);
		src += stride_ * 2;
	}
}

/**
 * \brief Calculate statistics for a frame in one go
 * \param[in] frame The frame number
 * \param[in] bufferId ID of the statistics buffer
 * \param[in] input The frame to process
 *
 * This may only be called after a successful setWindow() call.
 */
void SwStatsCpu::processFrame(uint32_t frame, uint32_t bufferId, FrameBuffer *input)
{
	bench_.startFrame();
	startFrame();

	MappedFrameBuffer in(input, MappedFrameBuffer::MapFlag::Read);
	if (!in.isValid()) {
		LOG(SwStatsCpu, Error) << "mmap-ing buffer(s) failed";
		return;
	}

	(this->*processFrame_)(in);
	finishFrame(frame, bufferId);
	bench_.finishFrame();
}

} /* namespace libcamera */
