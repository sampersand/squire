#ifndef SQ_FORM_H
#define SQ_FORM_H

#include <squire/value.h>
#include <squire/journey.h>
#include <squire/shared.h>

#include <assert.h>
#include <stddef.h>

// TODO: use `participates in` for `instanceof`

/** A class within Squire, also called a Form.
 * 
 * These are based off of Plato's theory of forms, with a splash of Aristotle's
 * amendments. The reasoning is as follows:
 * 
 * `form` (classes): Forms are perfect beings which are the embodiment of some
 *     property. Just like they're unreachable from the human world, Squire code
 *     cannot "create" Forms. Instead, they imitate them.
 * `essence` (static fields): When discussing Forms, you must describe what
 *     their *essences* are: What that form truly is. (Eg the form of Beauty is
 *     simply the essence of beauty.) As such, essences are static fields.
 *     (Technically, Plato believed Forms were unchanging, and thus there was no
 *     concept of modifying forms. But we do what we can.)
 * `recollection` (static functions): According to Plato (in the Phaedo), we
 *     humans were once in contact with the Forms in a prior life---after all,
 *     how else would we know what Beauty is? Therefore, we thing back to our
 *     previous lives to recollect actions that Forms take.
 * `imitate` (constructor): Since Forms cannot exist in the "real world," all
 *     physical objects are simply shallow imitations of the true Forms. Thus,
 *     you create an instance of a Form by imitating it.
 * `matter` (instance fields): Pulling a bit from Aristotle here. Aristotle
 *     believed that every object had two components: Matter and Form, where the
 *     matter was physical stuff and its properties were described by its Form.
 *     Therefore, we describe imitations by talking about their matter.
 * `change` (instance methods): A key point in Plato's philosophy was that 
 *     objects in the real world change, whereas the perfect Forms do not. Thus,
 *     when describing actions that imitations can perform, we talk about how
 *     the imitations change.
 * `parents` (TODO: describe aristotle)
 */
struct sq_form {
	struct sq_basic basic;
	unsigned nessences, nrecollections, nmatter, nchanges, nparents;

	char *name;
	struct sq_essence {
		char *name;
		sq_value value;
		sq_value genus; // may be SQ_UNDEFINED.
	} *essences;

	struct sq_form_matter {
		char *name;
		sq_value genus; // may be SQ_UNDEFINED.
	} *matter;
	struct sq_journey **changes, **recollections, *imitate; // imitate may be null
	struct sq_form **parents;
};

#warning todo
// SQ_VALUE_ASSERT_SIZE(struct sq_form);

/** An instance of a `sq_form`. */
struct sq_imitation {
	struct sq_basic basic;
	struct sq_form *form;
	sq_value *matter;
};
SQ_VALUE_ASSERT_SIZE(struct sq_imitation);

/** Allocates a new form with the given `name`.
 * 
 * Ownership of `name` is passed to this function; it should not be NULL.
 */
struct sq_form *sq_form_new(char *name);

void sq_form_mark(struct sq_form *form);
void sq_form_deallocate(struct sq_form *form);

/** Prints a debug representation of `form` to `out`. */
void sq_form_dump(FILE *out, const struct sq_form *form);

/** Fetches an essence (static field) on `form` named `name`.
 * 
 * If no such essence exists, `NULL` is returned.
 */
struct sq_essence *sq_form_lookup_essence(struct sq_form *form, const char *name);

/** Fetches a recollection (class function) on `form` named `name`.
 * 
 * If no such recollection exists, `NULL` is returned.
 */
struct sq_journey *sq_form_lookup_recollection(struct sq_form *form, const char *name);

/** Looks up either an `essence` or `recollection` with the given `name`.
 * 
 * If neither exists, `SQ_UNDEFINED` is returned.
 * Note that unlike `sq_form_lookup_essence` and `sq_form_lookup_recollection`,
 * this function passes ownership of the returned `sq_value` to the caller.
 */
sq_value sq_form_get_attr(struct sq_form *form, const char *attr);
bool sq_form_set_attr(struct sq_form *form, const char *name, sq_value value);

bool sq_form_is_parent_of(const struct sq_form *form, sq_value value);

/** Creates a new imitation of `form`, given the arguments.
 * 
 * An exception will be thrown if the arguments aren't valid.
 */
struct sq_imitation *sq_form_imitate(struct sq_form *form, struct sq_args args);

/**
 * Creates a new imitation (instance) for the given `form`, with the given
 * fields.
 * 
 * Note that ownership of both `form`'s and `fields`'s transferred.
 */
struct sq_imitation *sq_imitation_new(struct sq_form *form, sq_value *fields);

/** Fetches matter (instance field) from `imitation` with the given `name`.
 * 
 * If no matter with the given name exists, `NULL` is returned.
 */
sq_value *sq_imitation_lookup_matter(struct sq_imitation *imitation, const char *name);

/** Fetches a change (instance method) from `imitation` with the given `name`.
 * 
 * If no change with the given name exists, `NULL` is returned.
 */
struct sq_journey *sq_imitation_lookup_change(struct sq_imitation *imitation, const char *name);

/** Looks up either an `essence` or `recollection` with the given `name`.
 * 
 * If neither exists, `SQ_UNDEFINED` is returned.
 * Note that unlike `sq_imitation_lookup_matter` and `sq_imitation_lookup_change`,
 * this function passes ownership of the returned `sq_value` to the caller.
 */
sq_value sq_imitation_get_attr(struct sq_imitation *imitation, const char *name);
bool sq_imitation_set_attr(struct sq_imitation *imitation, const char *name, sq_value value);
void sq_imitation_mark(struct sq_imitation *imitation);
void sq_imitation_deallocate(struct sq_imitation *imitation);

/** Prints a debug representation of `imitation` to `out`. */
void sq_imitation_dump(FILE *, const struct sq_imitation *imitation);

#endif /* !SQ_FORM_H */
