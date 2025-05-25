#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <omen/vdso.h>
#include <bits/ensure.h>
#include <frg/string.hpp>
#include <frg/vector.hpp>
#include <mlibc/debug.hpp>
#include <mlibc/elf/startup.h>
#include <sys/auxv.h>

#if MLIBC_STATIC_BUILD
void* __dso_handle;
#endif

// defined by the POSIX library
void __mlibc_initLocale();

extern "C" uintptr_t *__dlapi_entrystack();
extern "C" void __dlapi_enter(uintptr_t *);

extern char **environ;
mlibc::exec_stack_data __mlibc_stack_data;

struct LibraryGuard {
	LibraryGuard();
};

uint64_t ExecFlags = 0;

static LibraryGuard guard;

LibraryGuard::LibraryGuard() {
	__mlibc_initLocale();
	mlibc::parse_exec_stack(__dlapi_entrystack(), &__mlibc_stack_data);
	mlibc::infoLogger() << "Argc: " << __mlibc_stack_data.argc << frg::endlog;
	mlibc::infoLogger() << "Argv: " << __mlibc_stack_data.argv << frg::endlog;
	for (int i = 0; i < __mlibc_stack_data.argc; i++) {
		mlibc::infoLogger() << "Argv[" << i << "]: " << __mlibc_stack_data.argv[i] << frg::endlog;
	}
	mlibc::infoLogger() << "Envp: " << __mlibc_stack_data.envp << frg::endlog;
	for (int i = 0; __mlibc_stack_data.envp[i]; i++) {
		mlibc::infoLogger() << "Envp[" << i << "]: " << __mlibc_stack_data.envp[i] << frg::endlog;
	}
	mlibc::set_startup_data(__mlibc_stack_data.argc, __mlibc_stack_data.argv,
			__mlibc_stack_data.envp);
	mlibc::infoLogger() << "VDSO ADDRESS: " << getauxval(AT_SYSINFO_EHDR) << frg::endlog;
	set_vdso_base((void *)getauxval(AT_SYSINFO_EHDR));
}

extern "C" void __mlibc_entry(uintptr_t *entry_stack, int (*main_fn)(int argc, char *argv[], char *env[])) {
	__dlapi_enter(entry_stack);

	auto result = main_fn(__mlibc_stack_data.argc, __mlibc_stack_data.argv, environ);
	exit(result);
}