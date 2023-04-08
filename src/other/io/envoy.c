#include <squire/exception.h>
#include <squire/other/envoy.h>
#include <squire/shared.h>

#include <string.h>
#include <dlfcn.h>
// #include <ffi/ffi.h>

void sq_envoy_initialize(struct sq_envoy *envoy, const char *filename) {
	if (!(envoy->dyllib = dlopen(filename, 0)))
		sq_throw_io("unable to load dyllib '%s': %s", filename, dlerror());

	envoy->symlen = 0;
	envoy->symcap = 4;
	envoy->filename = strdup(filename);
	envoy->syms = sq_malloc(sq_sizeof_array(struct sq_envoy_sym, envoy->symcap));
}

void sq_envoy_deallocate(struct sq_envoy *envoy) {
	for (unsigned i = 0; i < envoy->symlen; ++i)
		free(envoy->syms[i].name); // no need to free `syms[]->ptr`

	if (dlclose(envoy->dyllib))
		sq_throw_io("unable to unload dyllib '%s': %s", envoy->filename, dlerror());

	free(envoy->filename);
}

sq_value sq_envoy_get_attr(const struct sq_envoy *envoy, const char *attr) {
	for (unsigned i = 0; i < envoy->symlen; ++i)
		if (!strcmp(envoy->syms[i].name, attr))
			sq_throw_io("found: %s", attr);

	return SQ_UNDEFINED;
}

bool sq_envoy_set_attr(struct sq_envoy *envoy, const char *attr, sq_value value) {
	(void)envoy;
	(void) attr;
	(void) value;
	return 0;
}

void sq_envoy_dump(FILE *out, const struct sq_envoy *envoy) {
	fprintf(out, "Envoy(%s)\n", envoy->filename);
}
