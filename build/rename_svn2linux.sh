#!/bin/bash
#
# The linux kernel source contains some files which exist in the same directory
# as well with uppercase names as with lowercase names. 
# The contents of the files with the 'same' name is different.
#
# The following list gives a complete overview of the files concerned in linux
# kernel source 2.6.12:
#
# Directory include/linux/netfilter_ipv4:
#  ipt_CONNMARK.h    ipt_connmark.h
#  ipt_DSCP.h        ipt_dscp.h
#  ipt_ECN.h         ipt_ecn.h
#  ipt_MARK.h        ipt_mark.h
#  ipt_TCPMSS.h      ipt_tcpmss.h
#  ipt_TOS.h         ipt_tos.h
#
# Directory include/linux/netfilter_ipv6:
#  ip6t_MARK.h       ip6t_mark.h
#
# Directory net/ipv4/netfilter:
#  ipt_CONNMARK.c    ipt_connmark.c
#  ipt_DSCP.c        ipt_dscp.c
#  ipt_ECN.c         ipt_ecn.c
#  ipt_MARK.c        ipt_mark.c
#  ipt_TCPMSS.c      ipt_tcpmss.c
#  ipt_TOS.c         ipt_tos.c
#
# Directory net/ipv6/netfilter:
#  ip6t_MARK.c       ip6t_mark.c
#
# The sources from the SVN repository may be checked out on a Windows machine
# as well as on a Linux machine. Unfortunately on a Windows file system,
# two files having the same name, differing only in case, can't exist in one
# directory. Therefore a checkout of the directories listed above will always
# fail on a Windows machine.
#
# Remedy: Each file having an uppercase name has been renamed in the SVN 
# repository. The renaming scheme is: FILENAME.ext -> FILENAME.ext.off, in
# other words, the extension '.off' has been added to the name of each of these
# files.
#
# In order to be able to compile the netfilter part of the linux source, 
# these files should be renamed to their original names. This script will do 
# that for you! Actually, it will COPY those files to their original names. 
# This will prevent SVN complaining that some files have been deleted if 
# you do a commit.
#
# Please note, that if you modify one of these files (having their original 
# names), you should copy them back to their SVN names, otherwise your 
# modifications won't be checked in when you commit. Please do this manually!
#
# Nanko Verwaal, 02.08.2007

result=0

# The script should be executed from the root of the source project
root=$(pwd)

cd $root/include/linux/netfilter_ipv4
for s in CONNMARK DSCP ECN MARK TCPMSS TOS ; do
	cp -v ipt_$s.h.off ipt_$s.h
	result=$((result + $?))
done
 
cd $root/include/linux/netfilter_ipv6
for s in MARK ; do
	cp -v ip6t_$s.h.off ip6t_$s.h
	result=$((result + $?))
done
 
cd $root/net/ipv4/netfilter
for s in CONNMARK DSCP ECN MARK TCPMSS TOS ; do
	cp -v ipt_$s.c.off ipt_$s.c
	result=$((result + $?))
done
 
cd $root/net/ipv6/netfilter
for s in MARK ; do
	cp -v ip6t_$s.c.off ip6t_$s.c
	result=$((result + $?))
done

cd $root

if [ $result -eq 0 ] ; then
	echo "Renamed successfully"
else
	echo "Errors occurred during copying; couldn't copy $result files" >&2
fi

exit $result

