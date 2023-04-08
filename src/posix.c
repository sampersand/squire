#ifdef __GNUC__
struct _nothing_to_see_here;
#else

#include <squire/shared.h>
#include <string.h>
#include <stdint.h>

char *strdup(char *str) {
	size_t length = strlen(str) + 1;

	return memcpy(sq_malloc(length), str, length);
}

char *strndup(char *str, size_t maxlen) {
	size_t length = strlen(str);

	if (length < maxlen)
		return strdup(str);

	char *result = sq_malloc(maxlen);
	memcpy(result, str, maxlen);
	result[maxlen] = '\0';

	return result;
}

#endif /* __GNUC__ */
