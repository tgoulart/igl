/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#import <MetalKit/MetalKit.h>

@interface MetalView : MTKView {
  NSEventModifierFlags _lastKnownModifierFlags;
}

- (void)setViewController:(NSViewController*)newController;
@end
