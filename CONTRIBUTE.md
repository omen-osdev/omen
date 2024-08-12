# OMEN - Contributor's quick start guide

An open approach to resilience

# How to contribute to OMEN

## First steps

1. Read the [Docs](./README.md) to understand the project.
2. Read the [Code of Conduct](./docs/CODE_OF_CONDUCT.md) to understand the rules.
3. Choose the module you want to contribute to from the [Modules](./docs/MODULES.md) list.
4. Create your patch or feature and mail it to the module maintainer.

## What is a module

A module is a standalone piece of code. It should fit one of the following categories:

- An architecture-specific module
- A library
- A utility or a tool
- An abstract resource manager
- An abstraction layer

All but the first one should be architecture-agnostic and should include a test suite that
can run in a host environment (A regular linux machine).

Dependencies to other modules should be kept to a minimum and mocked in the test suite.

The API of each module has to be fully documented, see the generic API implementation for
reference. 

### Architecture-specific module

This kind of module directly interacts with the hardware. It should contain none to minimal
logic and should not depend on any other module.

The input to a module of this kind should not be validated and assumed to be correct. Just
like in a Data Access Object (DAO) in a database and doesn't need to take concurrency into
account. Locking should be done exclusively in abstract resource managers.

Testing in this case should be done on the target hardware or in a simulator.

All the structures should be abstracted, so the only module that can depend on this kind is
an abstraction layer module (or some specific utilities like panic).

In the documentation of an architecture-specific module, you should include the following:

- The hardware it is designed for.
- The references to the hardware documentation.
- 100% coverage of the logic (in a perfect world there should be no logic) in testing.
- As hardware state is shared, all changes in registers should be documented.

An example of an architecture-specific module is the `gdt` module. As you can see, multiple
modules can act over the same hardware. In this case it is important to use the same naming
conventions and to document the shared state. So if the first module calls gdt gdt, the second
one cannot call it: global_descriptor_table, it should be gdt as well. This also applies to
registers, flags, etc.

### Library

A library is a piece of software that aims to be reused by other modules. It doesn't need to be
thread-safe (they can, tho), but should be architecture-agnostic. They should be testable in a host environment.

Very important: Algorithms should always be implemented in libraries.

In the documentation of a library, you should include the following:

- The purpose of the library.
- The API of the library.
- Every relevant performance consideration.
- End to end tests and performance tests.

Libraries can only depend on other libraries or utilities and should only be used to store and
process generic data (void*). Libraries cannot have collateral effects (if they do then they are
not libraries but utilities).

An example of a library is a linked list implementation.

### Utility or a tool

A utility or a tool is similar to a library in that it is architecture-agnostic and it is designed
to be reused. The difference is that utilities and tools can have collateral effects (i.e. they can
write to the screen, to a file, etc).

In the documentation of a utility or a tool, you should include the following:

- The purpose of the utility or tool.
- The API of the utility or tool.
- A description of the collateral effects.
- End to end tests and performance tests.

An example of a utility is the `panic` module.

### Abstract resource manager

This is the bread and butter of the project. An abstract resource manager is a module that is
responsible for managing a shared resource. It should be thread-safe and should be able to handle
concurrency. It should be architecture-agnostic and should be testable in a host environment with
mocked dependencies.

In the documentation of an abstract resource manager, you should include the following:

- The purpose of the abstract resource manager.
- The API of the abstract resource manager.
- The list of resources it manages.
- Full coverage of the logic in testing.
- Performance considerations.
- Concurrency considerations.

An example of an abstract resource manager is the `memory` module.

### Abstraction layer

An abstraction layer is a module that is responsible for abstracting the hardware. It should be
architecture-agnostic and should be testable in a host environment with mocked dependencies.

They should not contain almost any logic, just the API of the hardware it abstracts and a bit
of validation (if needed).

In the documentation of an abstraction layer, you should include the following:

- The purpose of the abstraction layer.
- The API of the abstraction layer.
- The list of hardware it abstracts.
- A coverage of the logic in testing.

An example of an abstraction layer is the `interrupts` module.

## Architecture example

Photo:
![OMEN architecture](./arch.png)

## Mantras of the development

- **Keep it simple**: The simpler the better. If you can do it in 10 lines, don't do it in 20.
If you can't explain your module in a sentence, it's too complex.
- **Make it work and reliable at the same time, only after that make it fast**: The aim of this
kernel is to be stable. A code that works but crashes is not valid. Slow code is acceptable.
- **Reutilize**: Never duplicate code that is already written, rewriting is allowed though.
- **Test**: Test everything. Leave no edge case untested.
- **Concurrent by default**: Always think that your code will be run in parallel.
