/*
Copyright 2017 Google Inc. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS-IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef LULLABY_MODULES_FILE_TAGGED_FILE_LOADER_H_
#define LULLABY_MODULES_FILE_TAGGED_FILE_LOADER_H_

#include <functional>
#include <string>
#include <unordered_map>

#include "fplbase/utilities.h"

// This file implements a "tagging" system that allows for asset filenames to
// change at runtime, which is necessary in certain contexts..
// A tag is a string followed by a ':' that precedes a filename to be loaded.
// I.e. "foo:path/to/a/file" has the tag "foo". Under certain situations, the
// asset may reside in "bar/path/to/a/file", while in others it may reside in
// "baz/path/to/a/file". A TaggedFileLoader can be configured to replace "foo:"
// with "bar/" or "baz/" at runtime.

namespace lull {

// File loader supporting tagged asset file paths.
class TaggedFileLoader {
 public:
  TaggedFileLoader();
  virtual ~TaggedFileLoader() {}

  // Sets the loader to be used by |LoadTaggedFile| to |loader|.
  static void SetTaggedFileLoader(TaggedFileLoader* loader);

  // Loads |filename| into |dest| using the |TaggedFileLoader|, if one has been
  // set. Clients are expected to have called |SetTaggedFileLoader| prior to
  // calling this function.
  // Returns true if loading succeeds.
  // Returns false if loading fails or the |TaggedFileLoader| was never set.
  static bool LoadTaggedFile(const char* filename, std::string* dest);

  // Apply settings configured in the global |TaggedFileLoader| to |filename|.
  // Returns true if successful. If |transformed_filename| is non-null, it will
  // hold the transformed filename.
  // Returns false if no settings were found, in which case
  // |transformed_filename| will be given the value of |filename|.
  static bool ApplySettingsToTaggedFilename(
      const char* filename, std::string* const transformed_filename);

  // Load the contents of the specified filename into the given string pointer.
  // Return true on success, false otherwise.
  using LoadFileFn = std::function<bool(const char*, std::string*)>;

  // Loads a file and returns its contents via string pointer.
  // If |filename| begins with a tag, apply settings associated with the tag set
  // up by |RegisterTag|, then delegate the actual file read to a load method
  // implemented on a platform-specific basis.
  // If |filename| contains no tag and a default tag is set via |SetDefaultTag|,
  // applye settings of the default tag. If no default tag is present, use a
  // fallback load function set via |SetFallbackLoadFn|, which defaults to
  // |fplbase::LoadFileRaw|.
  // Returns true if successful, in which case |dest| will contain the contents
  // of the file, or false if the load fails.
  bool LoadFile(const char* filename, std::string* dest);

  // Adds an alternate path to attempt to load files from. Filenames ending with
  // |suffix| will be checked for in |path| if the asset is not found in the
  // asset folder. |suffix| should start with a '.' and |path| should end with
  // a '/'.  I.e. passing in ".ttf" and "/system/fonts/" will attempt to load
  // fonts from the system directory if they don't exist in the assets
  // directory.
  // If multiple alt paths are registered for a particular suffix, they will be
  // attempted in the order they were registered until one succeeds.
  void RegisterAltPathForSuffix(const std::string& suffix,
                                const std::string& path);

  // Sets a default |tag| whose settings will be used for untagged paths.
  void SetDefaultTag(const std::string& tag);

  // Sets a fallback load function that |LoadFile| will use for untagged paths.
  // Note: this function is used only when no default tag is set.
  // Defaults to |fplbase::LoadFileRaw|.
  // Returns the previously set LoadLifeFn.
  LoadFileFn SetFallbackLoadFn(LoadFileFn fn);

  // Apply settings configured in |TaggedFileLoader| to |filename|.
  // Returns true if successful. If |transformed_filename| is non-null, it will
  // hold the transformed filename.
  // Returns false if no settings were found, in which case
  // |transformed_filename| will be given the value of |filename|.
  bool ApplySettingsToFile(const char* filename,
                           std::string* const transformed_filename);

  // Replace requests to load |from_file| to load |to_file| instead.
  // Replacement is performed before any tag settings are applied.
  void AddReplacementFile(const std::string& from_file,
                          const std::string& to_file);

 protected:
  // Apply settings configured in |TaggedFileLoader| to |filename|.
  // Returns true if successful. If |transformed_filename| is non-null, it will
  // hold the transformed filename, and |tag_used| will contain the tag that was
  // used.
  // Returns false if no settings were found, in which case
  // |transformed_filename| will be given the value of |filename| and |tag_used|
  // will be left as-is.
  bool ApplySettingsToFile(const char* filename,
                           std::string* const transformed_filename,
                           std::string* const tag_used);

  // Set up files with |tag| to have |path_prefix| prepended to the untagged
  // filename. |tag| should end with a '/'.
  // A tag is a string followed by a ':' that precedes a filename to be
  // loaded. I.e. "foo:path/to/a/file" has the tag "foo", and calling this
  // method with tag "foo" and path_prefix "bar/" would result in loading
  // "bar/path/to/a/file" in place of the original filename.
  // Platform-specific implementations may choose to expose this function if no
  // further data is needed to register a tag.
  void RegisterTag(const std::string& tag, const std::string& path_prefix);

  // Implements the platform-specific technique for loading |filename| into
  // |dest|. |tag_used| will be non-empty if a tag was applied to the filename.
  virtual bool PlatformSpecificLoadFile(const char* filename, std::string* dest,
                                        const std::string& tag_used) = 0;

 private:
  std::unordered_map<std::string, std::string> tag_settings_map_;
  std::unordered_map<std::string, std::string> replacement_map_;
  std::unordered_map<std::string, std::vector<std::string>> alt_paths_;
  std::string default_tag_;
  LoadFileFn fallback_load_fn_;

  bool ApplySettingsToFile(const std::string& tag, const char* filename,
                           std::string* const transformed_filename);
};

}  // namespace lull

#endif  // LULLABY_MODULES_FILE_TAGGED_FILE_LOADER_H_
