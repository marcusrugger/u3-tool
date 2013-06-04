u3-tool
=======

Tool for controlling the special features of a "U3 smart drive" USB Flash disk.


Description
===========

This is a clone of the u3-tool found on SourceForge: http://u3-tool.sourceforge.net/

The sourceforge repository is authoritative.  Bug reports should go there.


How to build
============

u3-tool is an autotools (http://en.wikipedia.org/wiki/GNU_build_system) project.

Assuming you have all the necessary development tools installed, run the following commands from a terminal window in the project's root folder.

<pre>
$ aclocal
$ autoheader
$ automake [--add-missing]
$ autoconf
$ ./configure
$ make
</pre>

Optionally, after running the above commands, you may also run 'make install'.  However, u3-tool may also be executed directly from the src folder.
