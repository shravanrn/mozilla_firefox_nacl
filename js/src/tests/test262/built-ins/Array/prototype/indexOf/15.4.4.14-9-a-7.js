// Copyright (c) 2012 Ecma International.  All rights reserved.
// This code is governed by the BSD license found in the LICENSE file.

/*---
es5id: 15.4.4.14-9-a-7
description: >
    Array.prototype.indexOf - properties added into own object after
    current position are visited on an Array-like object
---*/

        var arr = { length: 2 };

        Object.defineProperty(arr, "0", {
            get: function () {
                Object.defineProperty(arr, "1", {
                    get: function () {
                        return 1;
                    },
                    configurable: true
                });
                return 0;
            },
            configurable: true
        });

assert.sameValue(Array.prototype.indexOf.call(arr, 1), 1, 'Array.prototype.indexOf.call(arr, 1)');

reportCompare(0, 0);
