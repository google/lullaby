# `Debug`


Enables 3D drawing of lines, shapes, text, etc for debugging purposes at the
specified positions in the app world.

### How to use:


`OnInitialize() { \
// Set implementation. \
// Initialize DebugImpl.

debug_impl_.reset(new lull::DebugRenderImpl(&registry));
lull::debug::Initialize(debug_impl_.get()); }

OnAdvanceFrame() { \
// Get registry, populate views. \
// Call Debug functions

lull::debug::DrawLine(...);

// Update the systems. \
// Commit debug data and draw the scene.

debug_impl_->Begin(views, gvr::kNumEyes); lull::debug::Submit();
debug_impl_->End(); \
}`

**IMPORTANT** \
_DO NOT_ explicitly create tags as string such as: \
`std::string tag("lull.Transform");`

`lull::debug::DrawLine(tag.c_str(), ...);`

Instead _DO_ the following: \
`constexpr char* gTag = "lull.Rotation";`

`lull::debug::DrawLine(gTag, ...);`

`lull::debug::DrawLine("lull.Transform", ...);`
