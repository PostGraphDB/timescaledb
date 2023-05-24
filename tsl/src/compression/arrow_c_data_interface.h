/*
 * This file and its contents are licensed under the Timescale License.
 * Please see the included NOTICE for copyright information and
 * LICENSE-TIMESCALE for a copy of the license.
 */

/*
 * This header describes the Arrow C data interface which is a well-known
 * standard for in-memory interchange of columnar data.
 *
 * https://arrow.apache.org/docs/format/CDataInterface.html
 */

#pragma once

typedef struct ArrowArray
{
	/*
	 * Mandatory. The logical length of the array (i.e. its number of items).
	 */
	int64 length;

	/*
	 * Mandatory. The number of null items in the array. MAY be -1 if not yet
	 * computed.
	 */
	int64 null_count;

	/*
	 * Mandatory. The logical offset inside the array (i.e. the number of
	 * items from the physical start of the buffers). MUST be 0 or positive.
	 *
	 * Producers MAY specify that they will only produce 0-offset arrays to
	 * ease implementation of consumer code. Consumers MAY decide not to
	 * support non-0-offset arrays, but they should document this limitation.
	 */
	int64 offset;

	/*
	 * Mandatory. The number of physical buffers backing this array. The
	 * number of buffers is a function of the data type, as described in the
	 * Columnar format specification.
	 *
	 * Buffers of children arrays are not included.
	 */
	int64 n_buffers;

	/*
	 * Mandatory. The number of children this type has.
	 */
	int64 n_children;

	/*
	 * Mandatory. A C array of pointers to the start of each physical buffer
	 * backing this array. Each void* pointer is the physical start of a
	 * contiguous buffer. There must be ArrowArray.n_buffers pointers.
	 *
	 * The producer MUST ensure that each contiguous buffer is large enough to
	 * represent length + offset values encoded according to the Columnar
	 * format specification.
	 *
	 * It is recommended, but not required, that the memory addresses of the
	 * buffers be aligned at least according to the type of primitive data
	 * that they contain. Consumers MAY decide not to support unaligned
	 * memory.
	 *
	 * The buffer pointers MAY be null only in two situations:
	 *
	 * - for the null bitmap buffer, if ArrowArray.null_count is 0;
	 *
	 * - for any buffer, if the size in bytes of the corresponding buffer would
	 * be 0.
	 *
	 * Buffers of children arrays are not included.
	 */
	const void **buffers;

	struct ArrowArray **children;
	struct ArrowArray *dictionary;

	/*
	 * Mandatory. A pointer to a producer-provided release callback.
	 *
	 * See below for memory management and release callback semantics.
	 */
	void (*release)(struct ArrowArray *);

	/* Opaque producer-specific data */
	void *private_data;
} ArrowArray;

static pg_attribute_always_inline bool
arrow_validity_bitmap_get(const uint64 *bitmap, int row_number)
{
	const int qword_index = row_number / 64;
	const int bit_index = row_number % 64;
	const uint64 mask = 1ull << bit_index;
	return (bitmap[qword_index] & mask) ? 1 : 0;
}

static pg_attribute_always_inline void
arrow_validity_bitmap_set(uint64 *bitmap, int row_number, bool value)
{
	const int qword_index = row_number / 64;
	const int bit_index = row_number % 64;
	const uint64 mask = 1ull << bit_index;

	bitmap[qword_index] = (bitmap[qword_index] & ~mask) | (((uint64) !!value) << bit_index);

	Assert(arrow_validity_bitmap_get(bitmap, row_number) == value);
}
