# vim-cmd

## STATUS ALPHA

This is a strawman version.  It is little more than a repository for a basic makefile.

This is being test on Windows; Linux; MacOS

This is NOT anything to do with VMWARE.

This is an indepdent project that has goals that are presented by the name.  Virtual-Infrastructure-Manager a straightforward derived name for a straightforward and common purpose.

vim-vmd is mean to commmunicate with hostd

hostd is meant to be a CPU emulator server.

EVERYTHING HERE IS A WORK IN PROGRESS

## INSTALLATION

```bash
cd /opt
git clone https://github.com/tlh45342/vim-cmd.git
cd vim-cmd
make ; make install
```

## REMARK:  If doing this on a Mac....

If on a Mac it will install the "vim-cmnd" command in the local users/bin directory.  i.e.  If the users name was user (for example) it 
would go in /User/user/bin.  it will modify the local users $PATH. It will check to see if this path already exists and NOT add it a second time.
Because the $PATH variable is assigned at launch of the shell... for now close the shelll and relaunch so that it picks up the 

## CONFIGURING:

vim-cmd> /set mode=tcp host=192.168.105.103 port=9000
[cfg] wrote /root/.config/vim-cmd/config

