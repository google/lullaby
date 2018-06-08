#!/usr/bin/python

"""Gathers file paths according to patterns specified on the command line."""

import glob
import sys


def main(argv):
  files = []
  for pattern in argv[1:]:
    files.extend(glob.glob(pattern))
  print '\n'.join(files).replace('\\', '/')
  return 0

if __name__ == '__main__':
  sys.exit(main(sys.argv))
