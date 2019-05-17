#!/bin/bash
#
# Generates hashes of known strings using the lull::Hash function. The output
# are lines of the form <string>,<hash>.

# Extracts all the arguments to the function identified by argument 1 that can
# be found in codesearch. Optionally strips the quotes from that argument if it
# they exist. Additional criteria to codesearch can be passed in argument 2.
#
# file 1:
# ...
# HashValue kFoo = Hash("foo");
# ...
#
# file 2:
# ...
# HashValue kConstant = Hash(CONSTANT)
# ...
#
# $extract Hash
# foo
# CONSTANT
extract () {
  results=$(csearch "$1" "$2")
  while read -r line; do
    echo "$line" | sed -nr "s/.*$1\(\"?([0-9A-Za-z_:\-]*)\"?.*/\1/p"
  done <<< "$results"
}

# Contains a reasonable approximation of all of the strings that are currently
# hashed in lullaby apps. It may contain some extra strings if constants are
# passed to the below functions, but it's not worth stripping those out.
results=$((extract "lull::Hash";
           extract "Hash" "f:lullaby";
           extract "lull::ConstHash";
           extract "ConstHash" '"f:lullaby"'
           extract "HashValue.hash";
           extract "LULLABY_SETUP_TYPEID") | LC_ALL=c sort -u)

# Generate the native hashes for those strings.
blaze build //lullaby/tests:hasher
while read -r result; do
  value=$(blaze-bin/lullaby/tests/hasher "$result")
  echo "$result,$value"
done <<< "$results"
