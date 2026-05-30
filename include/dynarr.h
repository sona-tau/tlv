#pragma once
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "prelude.h"

/// @file dynarr.h
/// @brief Type-safe dynamic array using a header-behind-pointer layout.
///
/// Each array is a single heap allocation: a DynArrHeader immediately followed
/// by the element data. The pointer you hold points to the data region, so
/// plain C array indexing (@c arr[i]) works directly.
///
/// @code
///   int *nums = da_new(int);
///   da_append(nums, 42);
///   printf("%d\n", nums[0]);
///   da_free(nums);
/// @endcode
///
/// @note @p arr must always be a typed pointer variable; many macros may
///       reassign it when a reallocation moves the underlying block.

/// @brief Internal metadata stored immediately before the data pointer.
typedef struct {
	size_t count;    ///< Number of elements currently stored.
	size_t capacity; ///< Number of elements the allocation can hold.
} DynArrHeader;

/// @cond INTERNAL
void* new_header(void);
void* copy_header(void *arr, size_t elem_size);

/// @brief Retrieve the DynArrHeader that precedes @p arr in memory.
#define da_header(arr) ((DynArrHeader*)(arr) - 1)
/// @endcond

// ---------------------------------------------------------------------------
// Lifecycle from README.md
// ---------------------------------------------------------------------------

/// @brief Allocate a new empty dynamic array.
///
/// @param type The element type (e.g. @c int, @c float, a struct). Used only
///             as documentation; the initial allocation contains no elements.
/// @return A @c void* pointing to the (empty) data region. Assign to a typed
///         pointer: @code int *arr = da_new(int); @endcode
/// @note Aborts on allocation failure.
#define da_new(type) new_header()

/// @brief Free a dynamic array.
///
/// Releases the entire allocation (header + data). @p arr must not be used
/// after this call.
///
/// @param arr The array to free.
#define da_free(arr) do { \
		free(da_header(arr)); \
	} while (0)

/// @brief Return an independent copy of @p arr.
///
/// Allocates a fresh block and copies all elements into it. The copy is
/// @e tight: its capacity equals its count. Mutations to either array do not
/// affect the other.
///
/// @param arr The source array.
/// @return A new @c void* pointing to the copied data. Assign to a typed
///         pointer of the same type as @p arr.
/// @note Aborts on allocation failure.
#define da_copy(arr) copy_header((void*)(arr), sizeof(*(arr)))

// ---------------------------------------------------------------------------
// Adding elements from README.md
// ---------------------------------------------------------------------------

/// @brief Append a single element to the end of the array.
///
/// Grows the allocation when @c count == @c capacity. Initial capacity is 8;
/// subsequent growth adds 50% (@c capacity + @c capacity/2).
///
/// @param arr  The array. May be reassigned if a reallocation occurs.
/// @param elem The value to append (evaluated once).
/// @note Aborts if @p arr is @c NULL or if reallocation fails.
#define da_append(arr, elem) do { \
		exists(arr); \
		DynArrHeader* h = da_header(arr); \
		if (h->count == h->capacity) { \
			size_t new_cap = h->capacity == 0 ? 8 : h->capacity + h->capacity / 2; \
			void*tmp = realloc(h, sizeof(DynArrHeader) + new_cap * sizeof(elem)); \
			exists(tmp); \
			h = tmp; \
			h->capacity = new_cap; \
		} \
		arr = (void*)(h + 1); \
		arr[h->count++] = elem; \
	} while (0)

/// @brief Append @p n elements from a plain C array.
///
/// Equivalent to calling da_append() in a loop but more efficient: at most
/// one reallocation is performed.
///
/// @param arr The destination array. May be reassigned if a reallocation occurs.
/// @param ptr Pointer to the source elements.
/// @param n   Number of elements to copy from @p ptr.
/// @note Aborts on allocation failure.
#define da_extend(arr, ptr, n) do { \
		size_t _n = (n); \
		size_t _count = da_header(arr)->count; \
		da_reserve(arr, _count + _n); \
		memcpy((arr) + _count, (ptr), _n * sizeof(*(arr))); \
		da_header(arr)->count = _count + _n; \
	} while (0)

/// @brief Pre-allocate capacity for at least @p n elements.
///
/// Ensures that at least @p n elements can be stored without further
/// reallocation. A no-op if current capacity already meets or exceeds @p n.
///
/// @param arr The array. May be reassigned if a reallocation occurs.
/// @param n   Minimum desired capacity.
/// @note Aborts on allocation failure.
#define da_reserve(arr, n) do { \
		DynArrHeader* h = da_header(arr); \
		if ((n) > h->capacity) { \
			void* tmp = realloc(h, sizeof(DynArrHeader) + (n) * sizeof(*(arr))); \
			exists(tmp); \
			h = tmp; \
			h->capacity = (n); \
			arr = (void*)(h + 1); \
		} \
	} while (0)

// ---------------------------------------------------------------------------
// Removing elements from README.md
// ---------------------------------------------------------------------------

/// @brief Remove and return the last element.
///
/// Decrements @c count and returns the value that was at @c arr[count].
/// Capacity is unchanged.
///
/// @param arr The array.
/// @return The removed element (lvalue).
#define da_pop(arr) (arr)[--da_header(arr)->count]

/// @brief O(1) unordered removal at index @p i.
///
/// Overwrites @c arr[i] with the last element then decrements @c count.
/// Does @e not preserve element order. Use da_last() to read the element
/// before removal if you need its value.
///
/// @param arr The array.
/// @param i   Index to remove. Bounds-checked with @c assert.
#define da_swap_remove(arr, i) do { \
		DynArrHeader* h = da_header(arr); \
		assert((i) < h->count); \
		(arr)[(i)] = (arr)[h->count - 1]; \
		h->count--; \
	} while (0)

/// @brief Reset the element count to zero, keeping the allocation intact.
///
/// After a clear, da_empty() returns true and the buffer can be reused
/// without another allocation.
///
/// @param arr The array.
#define da_clear(arr) do { da_header(arr)->count = 0; } while (0)

// ---------------------------------------------------------------------------
// Accessing elements from README.md
// ---------------------------------------------------------------------------

/// @brief Bounds-checked element access via @c assert.
///
/// @param arr The array.
/// @param i   Index (must be < da_length(@p arr)).
/// @return The element at index @p i (lvalue).
#define da_get(arr, i) (assert((i) < da_header(arr)->count), (arr)[(i)])

/// @brief The last element (no bounds check).
///
/// @param arr The array. Must be non-empty.
/// @return The element at @c arr[count - 1] (lvalue).
#define da_last(arr) (arr)[da_header(arr)->count - 1]

// ---------------------------------------------------------------------------
// Inspecting state from README.md
// ---------------------------------------------------------------------------

/// @brief Number of elements currently stored.
/// @param arr The array.
/// @return @c count as @c size_t.
#define da_length(arr) da_header(arr)->count

/// @brief Current allocated capacity (elements, not bytes).
/// @param arr The array.
/// @return @c capacity as @c size_t.
#define da_capacity(arr) da_header(arr)->capacity

/// @brief Test whether the array has no elements.
/// @param arr The array.
/// @return @c 1 if @c count == 0, @c 0 otherwise.
#define da_empty(arr) (da_header(arr)->count == 0)
