/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2014, 2015.

    This file is part of the Sortix C Library.

    The Sortix C Library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    The Sortix C Library is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
    License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with the Sortix C Library. If not, see <http://www.gnu.org/licenses/>.

    ubsan/ubsan.cpp
    Undefined behavior sanitizer runtime support.

*******************************************************************************/

#include <errno.h>
#include <fcntl.h>
#include <scram.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#if defined(__is_sortix_kernel)
#include <sortix/kernel/log.h>
#include <sortix/kernel/panic.h>
#endif

struct ubsan_source_location
{
	const char* filename;
	uint32_t line;
	uint32_t column;
};

struct ubsan_type_descriptor
{
	uint16_t type_kind;
	uint16_t type_info;
	char type_name[];
};

typedef uintptr_t ubsan_value_handle_t;

static const struct ubsan_source_location unknown_location =
{
	"<unknown file>",
	0,
	0,
};

__attribute__((noreturn))
static void ubsan_abort(const struct ubsan_source_location* location,
                        const char* violation)
{
	if ( !location || !location->filename )
		location = &unknown_location;
#if __STDC_HOSTED__
	struct scram_undefined_behavior info;
	info.filename = location->filename;
	info.line = location->line;
	info.column = location->column;
	info.violation = violation;
	scram(SCRAM_UNDEFINED_BEHAVIOR, &info);
#elif defined(__is_sortix_kernel)
	Sortix::PanicF("Undefined behavior: %s at %s:%u:%u",
		violation,
		location->filename,
		location->line,
		location->column);
#else
	#warning "You need to implement a undefined-behavior reporting mechanism"
	(void) location;
	(void) violation;
	abort();
#endif
}

#define ABORT_VARIANT(name, params, call) \
extern "C" __attribute__((noreturn)) \
void __ubsan_handle_##name##_abort params \
{ \
	__ubsan_handle_##name call; \
	__builtin_unreachable(); \
}

#define ABORT_VARIANT_VP(name) \
ABORT_VARIANT(name, (void* a), (a))
#define ABORT_VARIANT_VP_VP(name) \
ABORT_VARIANT(name, (void* a, void* b), (a, b))
#define ABORT_VARIANT_VP_IP(name) \
ABORT_VARIANT(name, (void* a, intptr_t b), (a, b))
#define ABORT_VARIANT_VP_VP_VP(name) \
ABORT_VARIANT(name, (void* a, void* b, void* c), (a, b, c))

struct ubsan_type_mismatch_data
{
	struct ubsan_source_location location;
	struct ubsan_type_descriptor* type;
	uintptr_t alignment;
	unsigned char type_check_kind;
};

extern "C"
void __ubsan_handle_type_mismatch(void* data_raw,
                                  void* pointer_raw)
{
	struct ubsan_type_mismatch_data* data =
		(struct ubsan_type_mismatch_data*) data_raw;
	ubsan_value_handle_t pointer = (ubsan_value_handle_t) pointer_raw;
	const char* violation = "type mismatch";
	if ( !pointer )
		violation = "null pointer access";
	else if ( data->alignment && (pointer & (data->alignment - 1)) )
		violation = "unaligned access";
	ubsan_abort(&data->location, violation);
}

ABORT_VARIANT_VP_VP(type_mismatch);

struct ubsan_overflow_data
{
	struct ubsan_source_location location;
	struct ubsan_type_descriptor* type;
};

extern "C"
void __ubsan_handle_add_overflow(void* data_raw,
                                 void* lhs_raw,
                                 void* rhs_raw)
{
	struct ubsan_overflow_data* data = (struct ubsan_overflow_data*) data_raw;
	ubsan_value_handle_t lhs = (ubsan_value_handle_t) lhs_raw;
	ubsan_value_handle_t rhs = (ubsan_value_handle_t) rhs_raw;
	(void) lhs;
	(void) rhs;
	ubsan_abort(&data->location, "addition overflow");
}

ABORT_VARIANT_VP_VP_VP(add_overflow);

extern "C"
void __ubsan_handle_sub_overflow(void* data_raw,
                                 void* lhs_raw,
                                 void* rhs_raw)
{
	struct ubsan_overflow_data* data = (struct ubsan_overflow_data*) data_raw;
	ubsan_value_handle_t lhs = (ubsan_value_handle_t) lhs_raw;
	ubsan_value_handle_t rhs = (ubsan_value_handle_t) rhs_raw;
	(void) lhs;
	(void) rhs;
	ubsan_abort(&data->location, "subtraction overflow");
}

ABORT_VARIANT_VP_VP_VP(sub_overflow);

extern "C"
void __ubsan_handle_mul_overflow(void* data_raw,
                                 void* lhs_raw,
                                 void* rhs_raw)
{
	struct ubsan_overflow_data* data = (struct ubsan_overflow_data*) data_raw;
	ubsan_value_handle_t lhs = (ubsan_value_handle_t) lhs_raw;
	ubsan_value_handle_t rhs = (ubsan_value_handle_t) rhs_raw;
	(void) lhs;
	(void) rhs;
	ubsan_abort(&data->location, "multiplication overflow");
}

ABORT_VARIANT_VP_VP_VP(mul_overflow);

extern "C"
void __ubsan_handle_negate_overflow(void* data_raw,
                                    void* old_value_raw)
{
	struct ubsan_overflow_data* data = (struct ubsan_overflow_data*) data_raw;
	ubsan_value_handle_t old_value = (ubsan_value_handle_t) old_value_raw;
	(void) old_value;
	ubsan_abort(&data->location, "negation overflow");
}

ABORT_VARIANT_VP_VP(negate_overflow);

extern "C"
void __ubsan_handle_divrem_overflow(void* data_raw,
                                    void* lhs_raw,
                                    void* rhs_raw)
{
	struct ubsan_overflow_data* data = (struct ubsan_overflow_data*) data_raw;
	ubsan_value_handle_t lhs = (ubsan_value_handle_t) lhs_raw;
	ubsan_value_handle_t rhs = (ubsan_value_handle_t) rhs_raw;
	(void) lhs;
	(void) rhs;
	ubsan_abort(&data->location, "division remainder overflow");
}

ABORT_VARIANT_VP_VP_VP(divrem_overflow);

struct ubsan_shift_out_of_bounds_data
{
	struct ubsan_source_location location;
	struct ubsan_type_descriptor* lhs_type;
	struct ubsan_type_descriptor* rhs_type;
};

extern "C"
void __ubsan_handle_shift_out_of_bounds(void* data_raw,
                                        void* lhs_raw,
                                        void* rhs_raw)
{
	struct ubsan_shift_out_of_bounds_data* data =
		(struct ubsan_shift_out_of_bounds_data*) data_raw;
	ubsan_value_handle_t lhs = (ubsan_value_handle_t) lhs_raw;
	ubsan_value_handle_t rhs = (ubsan_value_handle_t) rhs_raw;
	(void) lhs;
	(void) rhs;
	ubsan_abort(&data->location, "shift out of bounds");
}

ABORT_VARIANT_VP_VP_VP(shift_out_of_bounds);

struct ubsan_out_of_bounds_data
{
	struct ubsan_source_location location;
	struct ubsan_type_descriptor* array_type;
	struct ubsan_type_descriptor* index_type;
};

extern "C"
void __ubsan_handle_out_of_bounds(void* data_raw,
                                  void* index_raw)
{
	struct ubsan_out_of_bounds_data* data =
		(struct ubsan_out_of_bounds_data*) data_raw;
	ubsan_value_handle_t index = (ubsan_value_handle_t) index_raw;
	(void) index;
	ubsan_abort(&data->location, "out of bounds");
}

ABORT_VARIANT_VP_VP(out_of_bounds);

struct ubsan_unreachable_data
{
	struct ubsan_source_location location;
};

extern "C" __attribute__((noreturn))
void __ubsan_handle_builtin_unreachable(void* data_raw)
{
	struct ubsan_unreachable_data* data =
		(struct ubsan_unreachable_data*) data_raw;
	ubsan_abort(&data->location, "reached unreachable");
}

extern "C" __attribute__((noreturn))
void __ubsan_handle_missing_return(void* data_raw)
{
	struct ubsan_unreachable_data* data =
		(struct ubsan_unreachable_data*) data_raw;
	ubsan_abort(&data->location, "missing return");
}

struct ubsan_vla_bound_data
{
	struct ubsan_source_location location;
	struct ubsan_type_descriptor* type;
};

extern "C"
void __ubsan_handle_vla_bound_not_positive(void* data_raw,
                                           void* bound_raw)
{
	struct ubsan_vla_bound_data* data = (struct ubsan_vla_bound_data*) data_raw;
	ubsan_value_handle_t bound = (ubsan_value_handle_t) bound_raw;
	(void) bound;
	ubsan_abort(&data->location, "negative variable array length");
}

ABORT_VARIANT_VP_VP(vla_bound_not_positive);

struct ubsan_float_cast_overflow_data
{
	// TODO: Remove this GCC 5.x compatibility after switching to GCC 6.x. The
	//       GCC developers accidentally forgot the source location. Their
	//       libubsan probes to see if it looks like a path, but we don't need
	//       to maintain compatibility with multiple gcc releases. See below.
#if !(defined(__GNUC__) && __GNUC__< 6)
	struct ubsan_source_location location;
#endif
	struct ubsan_type_descriptor* from_type;
	struct ubsan_type_descriptor* to_type;
};

extern "C"
void __ubsan_handle_float_cast_overflow(void* data_raw,
                                        void* from_raw)
{
	struct ubsan_float_cast_overflow_data* data =
		(struct ubsan_float_cast_overflow_data*) data_raw;
	ubsan_value_handle_t from = (ubsan_value_handle_t) from_raw;
	(void) from;
#if !(defined(__GNUC__) && __GNUC__< 6)
	ubsan_abort(&data->location, "float cast overflow");
#else
	ubsan_abort(((void) data, &unknown_location), "float cast overflow");
#endif
}

ABORT_VARIANT_VP_VP(float_cast_overflow);

struct ubsan_invalid_value_data
{
	struct ubsan_source_location location;
	struct ubsan_type_descriptor* type;
};

extern "C"
void __ubsan_handle_load_invalid_value(void* data_raw,
                                       void* value_raw)
{
	struct ubsan_invalid_value_data* data =
		(struct ubsan_invalid_value_data*) data_raw;
	ubsan_value_handle_t value = (ubsan_value_handle_t) value_raw;
	(void) value;
	ubsan_abort(&data->location, "invalid value load");
}

ABORT_VARIANT_VP_VP(load_invalid_value);

struct ubsan_function_type_mismatch_data
{
	struct ubsan_source_location location;
	struct ubsan_type_descriptor* type;
};

extern "C"
void __ubsan_handle_function_type_mismatch(void* data_raw,
                                           void* value_raw)
{
	struct ubsan_function_type_mismatch_data* data =
		(struct ubsan_function_type_mismatch_data*) data_raw;
	ubsan_value_handle_t value = (ubsan_value_handle_t) value_raw;
	(void) value;
	ubsan_abort(&data->location, "function type mismatch");
}

ABORT_VARIANT_VP_VP(function_type_mismatch);

struct ubsan_nonnull_return_data
{
	struct ubsan_source_location location;
	struct ubsan_source_location attr_location;
};

extern "C"
void __ubsan_handle_nonnull_return(void* data_raw)
{
	struct ubsan_nonnull_return_data* data =
		(struct ubsan_nonnull_return_data*) data_raw;
	ubsan_abort(&data->location, "null return");
}

ABORT_VARIANT_VP(nonnull_return);

struct ubsan_nonnull_arg_data
{
	struct ubsan_source_location location;
	struct ubsan_source_location attr_location;
};

// TODO: GCC's libubsan does not have the second parameter, but its builtin
//       somehow has it and conflict if we don't match it.
extern "C"
void __ubsan_handle_nonnull_arg(void* data_raw,
                                intptr_t index_raw)
{
	struct ubsan_nonnull_arg_data* data =
		(struct ubsan_nonnull_arg_data*) data_raw;
	ubsan_value_handle_t index = (ubsan_value_handle_t) index_raw;
	(void) index;
	ubsan_abort(&data->location, "null argument");
}

ABORT_VARIANT_VP_IP(nonnull_arg);

struct ubsan_cfi_bad_icall_data
{
	struct ubsan_source_location location;
	struct ubsan_type_descriptor* type;
};

extern "C"
void __ubsan_handle_cfi_bad_icall(void* data_raw,
                                  void* value_raw)
{
	struct ubsan_cfi_bad_icall_data* data =
		(struct ubsan_cfi_bad_icall_data*) data_raw;
	ubsan_value_handle_t value = (ubsan_value_handle_t) value_raw;
	(void) value;
	ubsan_abort(&data->location,
	            "control flow integrity check failure during indirect call");
}

ABORT_VARIANT_VP_VP(cfi_bad_icall);