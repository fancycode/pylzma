#!/usr/bin/python -u
#
# Python Bindings for LZMA
#
# Copyright (c) 2004-2010 by Joachim Bauch, mail@joachim-bauch.de
# 7-Zip Copyright (C) 1999-2010 Igor Pavlov
# LZMA SDK Copyright (C) 1999-2010 Igor Pavlov
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# $Id$
#
"""Helper script for creating release tarballs."""

import os, sys
import tempfile
from distutils.dir_util import remove_tree

def runCommand(cmd, *args):
    cmd = [cmd] + list(args)
    assert os.system(' '.join(cmd)) == 0, 'command failed'

def main():
    if len(sys.argv) != 2:
        print 'Syntax is "tarball version"'
        return
    
    version = sys.argv[1]
    tempdir = tempfile.mkdtemp()
    pylzma = 'pylzma-%s' % version
    
    print 'Exporting version %s from subversion...' % version
    runCommand('svn', 'export', '-q', 'http://svn.fancycode.com/repos/python/pylzma/tags/v%s' % version.replace('.', '_'), os.path.join(tempdir, pylzma))
    file(os.path.join(tempdir, pylzma, 'version.txt'), 'wb').write(version)
    olddir = os.getcwd()
    os.chdir(tempdir)
    try:
        print 'Creating tarball %s' % os.path.join(olddir, pylzma+'.tar.gz')
        runCommand('tar', 'czf', os.path.join(olddir, pylzma+'.tar.gz'), pylzma)
    finally:
        os.chdir(olddir)
    
    remove_tree(tempdir)

if __name__ == '__main__':
    main()
