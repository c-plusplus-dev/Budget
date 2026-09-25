#include "Menu.h"

#ifdef __APPLE__
#import <Cocoa/Cocoa.h>
#import <objc/runtime.h>

// Global static key for associated objects
static void *CallbackKey = &CallbackKey;

@interface CallbackWrapper : NSObject
@property (nonatomic, copy) void (^callback)(void);
@end

@implementation CallbackWrapper
@end

@interface MenuHandler : NSObject
@end

@implementation MenuHandler
- (void)menuItemTriggered:(NSMenuItem*)sender {
    CallbackWrapper* wrapper = objc_getAssociatedObject(sender, CallbackKey);
    if (wrapper.callback) wrapper.callback();
}
@end

NSEventModifierFlags maskFromModifiers(int modifiers) {
    NSEventModifierFlags mask = 0;
    if (modifiers & COMMAND) mask |= NSEventModifierFlagCommand; // ⌘
    if (modifiers & SHIFT) mask |= NSEventModifierFlagShift;     // ⇧
    if (modifiers & OPTION) mask |= NSEventModifierFlagOption;   // ⌥
    if (modifiers & CONTROL) mask |= NSEventModifierFlagControl; // ⌃
    return mask;
}

// Recursive function to build submenus
NSMenuItem* CreateMenuItem(const MenuItem& item, MenuHandler* handler) {
    NSMenuItem* subItem = [[NSMenuItem alloc] initWithTitle:[NSString stringWithUTF8String:item.title.c_str()]
                                                     action:nil
                                              keyEquivalent:[NSString stringWithFormat:@"%c", item.shortcut.key]];
    subItem.target = handler;

    [subItem setKeyEquivalentModifierMask:maskFromModifiers(item.shortcut.modifiers)];

    CallbackKey = &CallbackKey;

    // If item has a submenu
    if (item.menu) { // pointer version
        NSMenu* nestedSubMenu = [[NSMenu alloc] init];
        for (const MenuItem& nestedItem : item.menu->items) {
            [nestedSubMenu addItem:CreateMenuItem(nestedItem, handler)];
        }
        [subItem setSubmenu:nestedSubMenu];
    }

    // If item has an action
    if (item.action) {
        CallbackWrapper* wrapper = [CallbackWrapper new];
        wrapper.callback = ^{
            item.action();
        };
        objc_setAssociatedObject(subItem, CallbackKey, wrapper, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        subItem.action = @selector(menuItemTriggered:);
    }

    return subItem;
}

void CreatePlatformMenu(const std::vector<Menu>& menus) {
    @autoreleasepool {
        NSMenu* mainMenu = [[NSMenu alloc] init];
        MenuHandler* handler = [[MenuHandler alloc] init];
        const Menu& firstMenu = menus[0];

        // ----- App menu (About, Quit) -----
        NSMenuItem* appMenuItem = [[NSMenuItem alloc] init];
        NSMenu* appMenu = [[NSMenu alloc] initWithTitle:@""];
        [appMenuItem setSubmenu:appMenu];

        [appMenu addItemWithTitle:[NSString stringWithFormat:@"About %@", [[NSProcessInfo processInfo] processName]]
                            action:@selector(orderFrontStandardAboutPanel:)
                     keyEquivalent:@""];
        [appMenu addItem:[NSMenuItem separatorItem]];
        if (firstMenu.mainMenu) {
            for (const MenuItem& item : firstMenu.items) {
                [appMenu addItem:CreateMenuItem(item, handler)];
            }
            [appMenu addItem:[NSMenuItem separatorItem]];
        }
        [appMenu addItemWithTitle:[NSString stringWithFormat:@"Quit %@", [[NSProcessInfo processInfo] processName]]
                            action:@selector(terminate:)
                     keyEquivalent:@"q"];

        [mainMenu addItem:appMenuItem];

        // ----- Other menus -----
        for (const Menu& menu : menus) {
            if (menu.mainMenu) continue;
            NSMenuItem* menuItem = [[NSMenuItem alloc] initWithTitle:[NSString stringWithUTF8String:menu.title.c_str()]
                                                              action:nil
                                                       keyEquivalent:@""];
            NSMenu* subMenu = [[NSMenu alloc] initWithTitle:[NSString stringWithUTF8String:menu.title.c_str()]];

            for (const MenuItem& item : menu.items) {
                [subMenu addItem:CreateMenuItem(item, handler)];
            }

            [menuItem setSubmenu:subMenu];
            [mainMenu addItem:menuItem];
        }

        [NSApp setMainMenu:mainMenu];
    }
}
#endif
