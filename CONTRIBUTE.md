# OMEN - Contributor's quick start guide

An open approach to resilience

# How to contribute to OMEN

## Quick guide

This is the **only** reference you will ever need to contribute to this project,
everything else is outdated or irrelevant. If you think there is a problem with
this guide please contact us on discord!

In order to contribute:

0. If you want, you can try to simply run ramen.sh on the root of this repo
   it will guide you through the entire process, or do the steps below!

2. Fork the repository to your own account.

3. Clone the repository at the develop branch:

```bash
# Clone the repository, you need to have git installed
git clone git@github.com:YOUR_GITHUB_USERNAME/omen.git
# List the branches and make sure you are in the develop branch
git branch -a
# If you are not in the develop branch, switch to it
git checkout develop
```

3. (Optional if you are just looking around) Create a new branch for your feature from the develop branch:

```bash
# Create a new branch
git checkout -b feature/your-feature-name
```

4. Make sure you have all the dependencies installed (the sudo command is also a dependency, feel free to edit it):

Choose the appropiate command for your specific distro from the list down here:

```bash
# Install the following dependencies (only tested for Ubuntu/Debian, adapt to your OS)
sudo apt install build-essential qemu-system qemu-system-x86 nasm make parted gdisk gdb tmux dosfstools tree

#Untested command for MacOS
xcode-select --install
brew install x86_64-elf-gcc x86_64-elf-gdb nasm dosfstools qemu

#Untested command for Fedora
sudo dnf install @development-tools qemu-system qemu-system-x86 nasm make parted gptfdisk gdb tmux dosfstools tree

#Untested command for CentOS/RHEL
sudo yum groupinstall "Development Tools"
sudo yum install qemu-system qemu-system-x86 nasm make parted gdisk gdb tmux dosfstools tree

#Untested command for Arch
sudo pacman -S base-devel qemu-system qemu-arch-extra nasm make parted gptfdisk gdb tmux dosfstools tree

#Untested command for OpenSUSE
sudo zypper install -t pattern devel_basis
sudo zypper install qemu-system qemu-system-x86 nasm make parted gdisk gdb tmux dosfstools tree

#Untested command for Alpine
sudo apk add build-base qemu-system qemu-system-x86 nasm make parted gptfdisk gdb tmux dosfstools tree

```

5. Go to the omen folder and open your favorite IDE:

```bash
# Go to the omen folder
cd omen
# Open your favorite IDE, a.e: if you use vscode:
code .
```

6. Setup the environment:

```bash
# Setup the environment
make setup-gpt
```

You should see something like this:

```bash
tretorn@pc:~/omen$ make setup-gpt
make[1]: Entering directory '/home/tretorn/omen/buildenv'
102400+0 records in
102400+0 records out
419430400 bytes (419 MB, 400 MiB) copied, 0.19487 s, 2.2 GB/s
Cloning into 'limine'...
remote: Enumerating objects: 17, done.
remote: Counting objects: 100% (17/17), done.
remote: Compressing objects: 100% (16/16), done.
remote: Total 17 (delta 1), reused 10 (delta 1), pack-reused 0
Receiving objects: 100% (17/17), 603.88 KiB | 4.54 MiB/s, done.
Resolving deltas: 100% (1/1), done.
'/home/tretorn/omen/buildenv/limine/limine.sys' -> '/home/tretorn/omen/buildenv/../build/iso_root/limine.sys'
'/home/tretorn/omen/buildenv/limine/limine-cd.bin' -> '/home/tretorn/omen/buildenv/../build/iso_root/limine-cd.bin'
'/home/tretorn/omen/buildenv/limine/limine-cd-efi.bin' -> '/home/tretorn/omen/buildenv/../build/iso_root/limine-cd-efi.bin'
make[1]: Leaving directory '/home/tretorn/omen/buildenv'
```

7. Try to compile and run the project (you will be prompted for sudo):

```bash
# Compile and run the project
make gpt
```
If everything went well, you should see QEMU booting up.

8. Now for the fun part, debugging. There are three posibilities:

- **Native linux or mac with tmux**: Just run `make debugpt` and you will be set. A tmux session will open with gdb and qemu. Wait for the gdb prompt and type `c` to continue.

- **Native linux or mac without tmux** : Go to the file `buildenv/GNUmakefile` and search for this line:

```makefile
	tmux split-window -h '$(GDB) $(GDBFLAGS)' & $(QEMU) -S -s $(QFLAGSEXP)$(ISODIR)/$(IMG)
```

Edit the line to remove the `tmux split-window -h` part and substitute with the
command of your preference.

- **Windows + WSL** : As this is my current env, I have created a target called
`debugpt-wsl` that will do everything for you.

9. Now you are all set, go ahead and select the module that you want to work on,
find the maintainer in the [maintainers list](./docs/MODULES.md) and send him a
message on the discord, if no maintainer exists, please contact Tretorn or ReanuKeeves.

10. Other importante make targets are:

- `make clean` : Cleans the project.
- `make cleansetup` : Uninstalls the dev environment.
- `make debugsetup` : Checks for leftovers from a previous debug session.

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
