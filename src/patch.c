#include <ini_configobj.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include "logging.h"

struct ini_cfgobj *preeny_patch_load(char *conf_file)
{
	struct ini_cfgfile *file_ctx = NULL;
	struct ini_cfgobj *ini_config = NULL;
	char **errors = NULL;
	int r;

	preeny_debug("loading config file %s\n", conf_file);

	r = ini_config_create(&ini_config);
	if (r != 0)
	{
		preeny_error("failed to create config object\n");
		return NULL;
	}

	r = ini_config_file_open(conf_file, 0, &file_ctx);
	if (r != 0)
	{
		preeny_error("failed to open config file %s\n", conf_file);
		ini_config_destroy(ini_config);
		return NULL;
	}

	r = ini_config_parse(file_ctx, INI_STOP_ON_ANY, 0, 0, ini_config);
	ini_config_file_destroy(file_ctx);

	if (r != 0)
	{
		preeny_error("patch file %s contains errors\n", conf_file);
		if (preeny_error_on)
		{
			if (ini_config_error_count(ini_config) > 0)
			{
				ini_config_get_errors(ini_config, &errors);
				if (errors)
				{
					ini_config_print_errors(stderr, errors);
					fprintf(stderr, "\n");
					ini_config_free_errors(errors);
				}
			}
		}
		ini_config_destroy(ini_config);
		return NULL;
	}

	preeny_debug("config file %s successfully loaded!\n", conf_file);
	return ini_config;
}

struct value_obj *preeny_patch_get_config_item(char *conf_file, char *section, struct ini_cfgobj *patch, char *name)
{
	struct value_obj *vo = NULL;
	int error;

	error = ini_get_config_valueobj(section, name, patch, INI_GET_FIRST_VALUE, &vo);
	if (!vo || error != 0)
	{
		preeny_debug("couldn't get %s item from section %s in patchfile %s\n", name, section, conf_file);
		return NULL;
	}

	return vo;
}

void *preeny_patch_get_pointer(char *conf_file, char *section, struct ini_cfgobj *patch, char *name)
{
	struct value_obj *vo = NULL;
	int error;

	const char *ptr_str;
	void *ptr;

	vo = preeny_patch_get_config_item(conf_file, section, patch, name);
	if (!vo) { preeny_error("error getting %s from section %s in patchfile %s\n", name, section, conf_file); return NULL; }

	ptr_str = ini_get_const_string_config_value(vo, &error);
	if (error != 0) { preeny_error("error converting %s from section %s in patchfile %s\n", name, section, conf_file); return NULL; }

	sscanf(ptr_str, "%p", &ptr);
	preeny_debug("retrieved %s: %p\n", name, ptr);
	return ptr;
}

void *preeny_patch_get_content(char *conf_file, char *section, struct ini_cfgobj *patch, int *content_length)
{
	struct value_obj *vo = NULL;
	int error;

	void *content;

	vo = preeny_patch_get_config_item(conf_file, section, patch, "content");
	if (!vo) return NULL;

	content = ini_get_bin_config_value(vo, content_length, &error);
	if (error != 0) { preeny_error("error converting content from section %s in patchfile %s\n", section, conf_file); return NULL; }

	return content;
}

int preeny_patch_apply_patch(void *target, void *content, int length)
{
	char error_str[1024];
	int error;
	int pagesize = getpagesize();
	uintptr_t target_page = ((uintptr_t)target)/pagesize*pagesize;

	preeny_debug("mprotecting pages containing %d bytes at %p so that we can write the patch\n", length, target);
	error = mprotect((void *)target_page, length, PROT_READ | PROT_WRITE | PROT_EXEC);
	if (error != 0)
	{
		strerror_r(errno, error_str, 1024);
		preeny_error("error '%s' making pages containing %d bytes at %p writeable\n", error_str, length, target);
		return error;
	}

	preeny_debug("writing %d bytes at %p\n", length, target);
	memcpy(target, content, length);

	preeny_debug("wrote %d bytes at %p\n", length, target);

	return 0;
}

int preeny_patch_apply_file(char *conf_file, struct ini_cfgobj *patch)
{
	int num_sections = 0;
	char **sections;
	char *section;
	int error = 0;
	int ret = 0;
	int i;

	void *addr;
	void *content;
	int content_length = 0;

	sections = ini_get_section_list(patch, &num_sections, &error);
	if (error != 0) { preeny_error("error getting section list from %s\n", conf_file); return -1; }

	for (i = 0; i < num_sections; i++)
	{
		section = sections[i];
		preeny_debug("apply patches for section %s in file %s\n", section, conf_file);

		addr = preeny_patch_get_pointer(conf_file, section, patch, "address");
		if (addr == NULL)
		{
			preeny_error("got NULL target for section %s from %s\n", section, conf_file);
			ret = -1;
			goto cleanup;
		}

		content = preeny_patch_get_content(conf_file, section, patch, &content_length);
		if (content == NULL)
		{
			preeny_error("got NULL content for section %s from %s\n", section, conf_file);
			ret = -1;
			goto cleanup;
		}

		preeny_info("section %s in file %s specifies %d-byte patch at %p\n", section, conf_file, content_length, addr);

		error = preeny_patch_apply_patch(addr, content, content_length);
		ini_free_bin_config_value(content);
		if (error != 0) { preeny_error("error applying patch section %s from %s\n", section, conf_file); ret = -1; goto cleanup; }
	}

cleanup:
	ini_free_section_list(sections);
	return ret;
}

__attribute__((constructor)) void preeny_patch_program()
{
	char *patchfile = getenv("PATCH");
	struct ini_cfgobj *p;

	if (patchfile)
	{
		p = preeny_patch_load(patchfile);
		if (p == NULL) exit(137);

		preeny_patch_apply_file(patchfile, p);
		ini_config_destroy(p);
	}

	preeny_debug("done patching!\n");
}
