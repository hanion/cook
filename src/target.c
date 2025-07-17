#include "target.h"
#include "build_command.h"
#include "executer.h"

StringBuilder target_generate_cmdline_cstr(Arena* arena, struct BuildCommand* bc, Target* t) {
	StringBuilder sb = target_generate_cmdline(arena, bc, t);
	da_append_arena(arena, &sb, '\0');
	return sb;
}


inline static void target_print_stringview(Arena* arena, StringBuilder* sb, StringView sv) {
	da_reserve_arena(arena, sb, sb->count + sv.count + 1);
	int written = snprintf(sb->items + sb->count, sv.count + 2, "%.*s ", (int)sv.count, sv.items);
	sb->count += written;
}

inline static void target_string_list_print_flat(Arena* arena, StringBuilder* sb, const StringList* list, const char* prefix) {
	if (!list || !list->items) {
		return;
	}
	for (size_t i = 0; i < list->count; ++i) {
		if (prefix && strlen(prefix) > 0) {
			int written = sprintf(sb->items, "%s", prefix);
			sb->count += written;
		}
		target_print_stringview(arena, sb, list->items[i]);
	}
}

StringBuilder target_generate_cmdline(Arena* arena, struct BuildCommand* bc, Target* t) {
	StringBuilder sb = {0};
	if (!arena || !bc || !t) return sb;


	if (bc->compiler.count > 0) {
		target_print_stringview(arena, &sb, bc->compiler);
	}

	target_string_list_print_flat(arena, &sb, &bc->cflags, "");

	if (bc->build_type == BUILD_OBJECT) {
		da_append_many_arena(arena, &sb, "-c ", 3);
	}

	da_append_many_arena(arena, &sb, "-o ", 3);
	target_print_stringview(arena, &sb, sv_from_sb(t->output_name));
	target_print_stringview(arena, &sb, sv_from_sb(t->input_name));

	target_string_list_print_flat(arena, &sb, &bc->include_dirs, "-I");
	target_string_list_print_flat(arena, &sb, &bc->input_files, "");
	target_string_list_print_flat(arena, &sb, &bc->input_objects, "");
	target_string_list_print_flat(arena, &sb, &bc->library_dirs, "-L");
	target_string_list_print_flat(arena, &sb, &bc->library_links, "-l");
	target_string_list_print_flat(arena, &sb, &bc->ldflags, "");

	return sb;
}


bool target_check_dirty(struct BuildCommand* bc, Target* t) {
	uint64_t out_time = get_modification_time_sv(sv_from_sb(t->output_name));
	uint64_t in_time  = get_modification_time_sv(sv_from_sb(t->input_name));

	for (size_t i = 0; i < bc->input_files.count; ++i) {
		uint64_t time = get_modification_time_sv(bc->input_files.items[i]);
		if (time != 0 && time > in_time) {
			in_time = time;
		}
	}

	if (out_time > in_time) {
		return false;
	}
	
	t->dirty = true;
	bc->dirty = true;

	BuildCommand* p = bc->parent;
	while (p) {
		p->dirty = true;
		p = p->parent;
	}

	return true;
}

