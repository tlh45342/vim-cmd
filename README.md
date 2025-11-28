This is NOT anything to do with VMWARE.

This is an intendent project that has goals that are presented by the name. Virtual-Infrastructure-Manager a straightforward derived name for a straightforward and common purpose.

vim-vmd is mean to communicate with hostd

hostd is meant to be a CPU emulator server.

EVERYTHING HERE IS A WORK IN PROGRESS

## INSTALLATION

```
cd /opt
git clone https://github.com/tlh45342/vim-cmd.git
cd vim-cmd
make ; make install
```

## REMARK: If doing this on a Mac....

If on a Mac it will install the "vim-cmd" command in the local users/bin directory. i.e. If the users name was user (for example) it would go in /User/user/bin. it will modify the local users $PATH. It will check to see if this path already exists and NOT add it a second time. Because the $PATH variable is assigned at launch of the shell... for now close the shelll and relaunch so that it picks up the

## CONFIGURING:

The current version is pretty much a lightweight version that is using UNIX sockets to connect.

This [ in the initial phase ] is going to make it simple to connect and pass traffic.

It is in no way going to be secure.

This is in someways the networked version to arm-vm

Only vim-cmd is will connect to hostd running somewhere.



Looks like this is setup to connect local by UNIX socket or acrosss the network using basic TCP.  No encryption used in this version.

Local commands look like they can be of the form:

  /set mode=tcp host=127.0.0.1 port=9000
  /set mode=unix socket=/tmp/hostd.sock
  /connect tcp 127.0.0.1 9000
  /connect unix /tmp/hostd.sock

I think I will need to integrate the "config" file so that it can use security (duh) but this will be a downstream goal as getting this setup and integrated is going to take precendence.



For Windows we are storing the config file in:
C:\Users\User\\.config\vim-cmd
under the filename "config" - assuming the user name is "User"
