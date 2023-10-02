#ifndef __SRC_DOMAINS_H__
#define __SRC_DOMAINS_H__

#include <linux/fs.h>
#include <linux/mm_types.h>

#include "dll.h"
#include "tyche_api.h"
#include "tyche_capabilities_types.h"
#define _IN_MODULE
#include "tyche_driver.h"
#undef _IN_MODULE

// ————————————————————————————————— Types —————————————————————————————————— //

#define UNINIT_USIZE (~((usize)0))
#define UNINIT_DOM_ID (~((domain_id_t)0))

/// Internal state within the driver, used for domains and segments.
/// This mirrors whether the information has been sent to tyche or not.
typedef enum driver_state_t {
	DRIVER_NOT_COMMITED = 0,
	DRIVER_COMMITED = 1,
	DRIVER_DEAD = 2,
} driver_state_t;

/// Describes an domain's memory segment in user process address space.
typedef struct segment_t {
	/// Start of the virtual memory segment.
	usize va;

	/// Corresponding start of the physical segment.
	usize pa;

	/// Size of the memory segment.
	usize size;

	/// Protection flags.
	memory_access_right_t flags;

	/// Type for the region: {Shared|Confidential}.
	segment_type_t tpe;

	/// The offset at which the segment is mapped (gpa).
	usize alias;

	/// Segment state.
	driver_state_t state;

	/// Segments are stored in a double linked list.
	dll_elem(struct segment_t, list);
} segment_t;

/// An entry point on a core for the domain.
typedef struct entry_t {
	usize cr3;
	usize rip;
	usize rsp;
} entry_t;

#define ENTRIES_PER_DOMAIN (16)

/// Entries per core for the domain.
typedef struct entries_t {
	/// One entry per core, total number of entries.
	entry_t entries[ENTRIES_PER_DOMAIN];
} entries_t;

/// Indicies in the domain config array.
typedef tyche_configurations_t driver_domain_config_t;

/// Describes an domain.
typedef struct driver_domain_t {
	/// The creator task's pid.
	pid_t pid;

	/// The domain's handle within the driver.
	domain_handle_t handle;

	/// The domain's domain id.
	domain_id_t domain_id;

	/// The domain's state.
	driver_state_t state;

	/// The domain's configuration.
	usize configs[TYCHE_NR_CONFIGS];

	/// The domain's entry points per core.
	entries_t entries;

	/// The available raw memory segments.
	/// This is typically allocated during the mmap (from userspace),
	/// or taken from KVM (kvm_memory_regions).
	dll_list(segment_t, raw_segments);

	/// The initialized segments for the domain.
	/// The access rights have been set.
	dll_list(segment_t, segments);

	/// Domains are stored in a global list by the driver.
	dll_elem(struct driver_domain_t, list);
} driver_domain_t;

// ———————————————————————————————— Helpers ————————————————————————————————— //

// Find a currently active domain from a file descriptor.
driver_domain_t *find_domain(domain_handle_t handle);

// ——————————————————————————————— Functions ———————————————————————————————— //

/// Initializes the driver.
void driver_init_domains(void);

/// Initializes the capability library.
int driver_init_capabilities(void);

/// Create a new domain with handle.
/// If ptr is not null, it points to the newly created driver domain.
int driver_create_domain(domain_handle_t handle, driver_domain_t **ptr);
/// Handles an mmap call to the driver.
/// This reserves a contiguous region and registers it until a domain claims
/// it.
int driver_mmap_segment(driver_domain_t *domain, struct vm_area_struct *vma);

/// Add a raw memory segment to the domain.
int driver_add_raw_segment(driver_domain_t *dom, usize va, usize pa,
			   usize size);

/// Returns the domain's physoffset.
/// We expect the handle to be valid, and the virtaddr to exist in segments.
int driver_get_physoffset_domain(driver_domain_t *domain, usize *phys_offset);

/// Sets up access rights and conf|share for the segment.
int driver_mprotect_domain(driver_domain_t *domain, usize vstart, usize size,
			   memory_access_right_t flags, segment_type_t tpe,
			   usize alias);

/// Sets the domain's configuration (cores, traps, switch type).
int driver_set_domain_configuration(driver_domain_t *domain,
				    driver_domain_config_t tpe, usize value);

/// Expose the configuration of fields.
int driver_set_domain_core_config(driver_domain_t *dom, usize core,
				  register_group_t group, usize idx,
				  usize value);

/// Set the entry point on a core.
int driver_set_entry_on_core(driver_domain_t *domain, usize core, usize cr3,
			     usize rip, usize rsp);

/// Performs the calls to tyche monitor for the selected regions.
int driver_commit_regions(driver_domain_t *dom);

/// Commit the configuration, i.e., call the capabilities.
int driver_commit_domain_configuration(driver_domain_t *dom,
				       driver_domain_config_t idx);

/// Commit the entry on a core, i.e., call the capabilities.
int driver_commit_entry_on_core(driver_domain_t *dom, usize core);

/// Commits the domain. This is where the capability operations are mostly
/// done.
int driver_commit_domain(driver_domain_t *domain, int full);

/// Implements the transition into a domain.
int driver_switch_domain(driver_domain_t *domain, void *args);

/// Delete the domain and revoke the capabilities.
int driver_delete_domain(driver_domain_t *domain);
#endif /*__SRC_DOMAINS_H__*/
