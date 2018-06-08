"""Append Lullaby-specific Entity Schema to a client Component Schema."""
from __future__ import absolute_import
import argparse
from shutil import copyfile


def main():
  parser = argparse.ArgumentParser(description='Create an entity schema fbs.')
  parser.add_argument('--infile', '-i', help='Input component fbs file.',
                      required=True)
  parser.add_argument('--appendfile', '-x', help='Entity schema file.',
                      required=True)
  parser.add_argument('--outfile', '-o', help='Output entity fbs file.',
                      required=True)
  parser.add_argument(
      '--identifier',
      help='file_identifier of the resulting flatbuffer type.',
      required=True)
  args = parser.parse_args()
  copyfile(args.infile, args.outfile)
  txt = ''
  with open(args.appendfile, 'r') as f:
    txt = f.read()
  with open(args.outfile, 'a') as f:
    f.write(txt)
    f.write('file_identifier "%s";' % args.identifier)

if __name__ == '__main__':
  main()
