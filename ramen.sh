#!/bin/bash

#function say to print a message given as argument in blue
say() {
    echo -e "\e[34m$@\e[0m"
}

err() {
    echo -e "\e[31m$@\e[0m"
}

install_deps() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        case $ID in
            ubuntu|debian)
                say "Installing dependencies for $ID"
                sudo apt install -y build-essential qemu-system qemu-system-x86 nasm make parted gdisk gdb tmux dosfstools tree
                ;;
            arch)
                say "Installing dependencies for Arch Linux"
                sudo pacman -S --noconfirm base-devel qemu nasm make parted gdisk gdb tmux dosfstools tree
                ;;
            fedora)
                say "Installing dependencies for Fedora"
                sudo dnf install -y @development-tools qemu-system-x86 nasm make parted gdisk gdb tmux dosfstools tree
                ;;
            centos|rhel)
                say "Installing dependencies for $ID"
                sudo yum groupinstall -y "Development Tools"
                sudo yum install -y qemu-system-x86 nasm make parted gdisk gdb tmux dosfstools tree
                ;;
            darwin)
                say "Installing dependencies for macOS"
                xcode-select --install
                brew install qemu nasm make gdisk gdb tmux dosfstools tree
                ;;
            alpine)
                say "Installing dependencies for Alpine Linux"
                apk add build-base qemu-system-x86 nasm make parted gdisk gdb tmux dosfstools tree
                ;;
            *)
                err "Unsupported OS detected. Please install the required dependencies manually:"
                err "build-essential, qemu-system, qemu-system-x86, nasm, make, parted, gdisk, gdb, tmux, dosfstools, tree"
                ;;
        esac
    else
        err "Unable to detect the operating system. Please install the required dependencies manually:"
        err "build-essential, qemu-system, qemu-system-x86, nasm, make, parted, gdisk, gdb, tmux, dosfstools, tree"
    fi
}

say Run Automatically o\(MEN\), automator for newbies
say Dependencies for this script are bash, git and sudo.
say First of all, go to github and fork this repo: https://github.com/omen-osdev/omen.git
read -p "Press enter when you are ready"
say Now enter your github username \(ex. omen-osdev\)
read username
say Now enter the name of the repo you just forked \(ex. omen\)
read reponame

cloneuri="git@github.com:$username/$reponame.git"
git clone $cloneuri
cd $reponame

git checkout develop
say Now name your new feature branch, please include feature/ at the beginning \(ex: feature/your-feature-name\)
say If you are just seeing the code atm, leave this empty
read featurename
if [ -z "$featurename" ]
then
    say "No feature name provided, skipping branch creation"
else
    git checkout -b $featurename
fi

say We will now install the dependencies.

install_deps

read -p "If everything was ok, press enter to setup the project"
cd omen
make setup-gpt
read -p "If everything was ok, press enter to compile and run"
make gpt
say Everything is ready for you to start fiddling, just open the omen folder in your favourite IDE
