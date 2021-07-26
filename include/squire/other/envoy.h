#ifndef SQ_ENVOY_H
#define SQ_ENVOY_H

#include <squire/value.h>

struct sq_envoy {
	void *dyllib;
	char *filename;
	unsigned symlen, symcap;
	struct sq_envoy_sym {
		char *name;
		void *ptr;
	} *syms;
};

enum sq_envoy_kind {
	SQ_ENVOY_T_UCHAR,
	SQ_ENVOY_T_SCHAR,
	SQ_ENVOY_T_USHORT,
	SQ_ENVOY_T_SSHORT,
	SQ_ENVOY_T_SINT,
	SQ_ENVOY_T_UINT,
	SQ_ENVOY_T_SLONG,
	SQ_ENVOY_T_ULONG,
	SQ_ENVOY_T_SLONGLONG,
	SQ_ENVOY_T_ULONGLONG,
	SQ_ENVOY_T_U8,
	SQ_ENVOY_T_S8,
	SQ_ENVOY_T_U16,
	SQ_ENVOY_T_S16,
	SQ_ENVOY_T_U32,
	SQ_ENVOY_T_S32,
	SQ_ENVOY_T_U64,
	SQ_ENVOY_T_S64,
	SQ_ENVOY_T_USIZE_T,
	SQ_ENVOY_T_SSIZE_T,
	SQ_ENVOY_T_VOID_PTR,
	SQ_ENVOY_T_CHAR_PTR,
};


void sq_envoy_initialize(struct sq_envoy *envoy, const char *filename);
void sq_envoy_deallocate(struct sq_envoy *envoy);

void sq_envoy_dump(FILE *out, const struct sq_envoy *envoy);

sq_value sq_envoy_get_attr(const struct sq_envoy *envoy, const char *attr);
bool sq_envoy_set_attr(struct sq_envoy *envoy, const char *attr, sq_value value);


#endif /* !SQ_ENVOY_H */
