#import <AppKit/NSWindow.h>
#import <AppKit/NSView.h>

void* GetNativeWindow(void *window) {
  NSWindow* ns_window = (__bridge NSWindow*)window;
  NSView* ns_view = [ns_window contentView];
  return (__bridge void*)ns_view;
}
