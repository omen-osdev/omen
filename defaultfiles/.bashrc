##
## Slightly nicer .bashrc
## Makes pretty colors and stuff
##

## Set $PATH, which tells the computer where to search for commands
export PATH="$PATH:/usr/bin:/export"

## Where to search for manual pages
export MANPATH="/usr/share/man:/usr/local/man:/usr/local/local_dfs/man"

## Which pager to use.
export PAGER=less

## Choose your weapon
EDITOR=/usr/bin/vim
#EDITOR=/usr/bin/emacs
#EDITOR=/usr/bin/nano
export EDITOR

## The maximum number of lines in your history file
export HISTFILESIZE=50

## UVM!
export ORGANIZATION="Omen osdev"

## Enables displaying colors in the terminal
export TERM=xterm-color

# Uncomment the following lines if you are an ARC/INFO user
#alias arc=/usr/local/bin/arc
#alias arcdoc=/usr/local/bin/arcdoc
#alias info=/usr/local/bin/arcinfo

## Disable automatic mail checking
unset MAILCHECK 

## If this is an interactive console, disable messaging
#tty -s && mesg n

## Aliases from 'ol EMBA tcsh
#alias bye=logout
#alias h=history
#alias jobs='jobs -l'
#alias lf='ls -algF'
#alias log=logout
#alias cls=clear
#alias edit=$EDITOR
#alias restore=/usr/local/local_dfs/bin/restore

## Automatically correct mistyped 'cd' directories
#shopt -s cdspell

## Append to history file; do not overwrite
shopt -s histappend

## Prevent accidental overwrites when using IO redirection
set -o noclobber

export PS1='$USER@$HOSTNAME: '